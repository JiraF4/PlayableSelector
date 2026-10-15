enum PS_EFreezePhase
{
	NOT_STARTED = 0,
	STARTUP = 1,
	ORDINARY = 2,
	FINISHED = 3
};

void PS_OnHardFreezeChangedMethod(bool active);
typedef func PS_OnHardFreezeChangedMethod;
typedef ScriptInvokerBase<PS_OnHardFreezeChangedMethod> PS_OnHardFreezeChangedInvoker;

void PS_OnFreezeTimeAdjustedMethod(float deltaMilliseconds);
typedef func PS_OnFreezeTimeAdjustedMethod;
typedef ScriptInvokerBase<PS_OnFreezeTimeAdjustedMethod> PS_OnFreezeTimeAdjustedInvoker;

[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "Управление фазами фриза и Hard Freeze")]
class PS_FreezeManagerClass : SCR_BaseGameModeComponentClass
{
};

class PS_FreezeManager : SCR_BaseGameModeComponent
{
	[Attribute("-1", UIWidgets.EditBox, "Freeze time override in ms (-1 = use GameMode attribute, 0 = disable, >0 = override)", category: "Freeze Manager")]
	protected int m_iFreezeTimeOverride;

	[Attribute("-1", UIWidgets.EditBox, "Disable time (startup freeze) override in ms (-1 = use GameMode attribute, 0 = disable, >0 = override)", category: "Freeze Manager")]
	protected int m_iDisableTimeOverride;

	[RplProp(onRplName: "OnRpl_CurrentFreezeTime")]
	protected float m_fCurrentFreezeTime;

	[RplProp(onRplName: "OnRpl_HardFreeze")]
	protected bool m_bHardFreeze;

	[RplProp(onRplName: "OnRpl_HardFreezeRemaining")]
	protected float m_fHardFreezeRemaining;

	[RplProp()]
	protected float m_fHardFreezeDuration;

	[RplProp()]
	protected int m_iEffectiveFreezeTime;

	[RplProp()]
	protected int m_iEffectiveDisableTime;

	[RplProp(onRplName: "OnRpl_FreezePhase")]
	protected PS_EFreezePhase m_eFreezePhase = PS_EFreezePhase.NOT_STARTED;

	[RplProp(onRplName: "OnRpl_StartupFreeze")]
	protected bool m_bStartupFreeze;

	protected ref PS_OnHardFreezeChangedInvoker m_OnHardFreezeChanged = new PS_OnHardFreezeChangedInvoker();
	protected ref PS_OnFreezeTimeAdjustedInvoker m_OnFreezeTimeAdjusted = new PS_OnFreezeTimeAdjustedInvoker();

	protected ref PS_VehicleHold m_VehicleHold = new PS_VehicleHold();
	protected ref PS_DamagePause m_DamagePause = new PS_DamagePause();

	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_FreezeTimeCounter m_hFreezeTimeCounter;
	protected bool m_bFreezeEndTriggered;
	protected bool m_bDayAdvanceWasOn;
	protected static PS_FreezeManager s_Instance;

	//------------------------------------------------------------------------------------------------
	static PS_FreezeManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	PS_OnHardFreezeChangedInvoker GetOnHardFreezeChanged()
	{
		return m_OnHardFreezeChanged;
	}

	//------------------------------------------------------------------------------------------------
	PS_OnFreezeTimeAdjustedInvoker GetOnFreezeTimeAdjusted()
	{
		return m_OnFreezeTimeAdjusted;
	}

	//------------------------------------------------------------------------------------------------
	bool IsHardFreezeActive()
	{
		return m_bHardFreeze;
	}

	//------------------------------------------------------------------------------------------------
	bool IsStartupFreeze()
	{
		return m_bStartupFreeze;
	}

	//------------------------------------------------------------------------------------------------
	float GetHardFreezeRemaining()
	{
		return m_fHardFreezeRemaining;
	}

	//------------------------------------------------------------------------------------------------
	float GetHardFreezeDuration()
	{
		return m_fHardFreezeDuration;
	}

	//------------------------------------------------------------------------------------------------
	float GetCurrentFreezeTime()
	{
		return m_fCurrentFreezeTime;
	}

	//------------------------------------------------------------------------------------------------
	int GetEffectiveFreezeTime()
	{
		return m_iEffectiveFreezeTime;
	}

	//------------------------------------------------------------------------------------------------
	int GetEffectiveDisableTime()
	{
		return m_iEffectiveDisableTime;
	}

	//------------------------------------------------------------------------------------------------
	PS_EFreezePhase GetFreezePhase()
	{
		return m_eFreezePhase;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFreezeTimeEnd()
	{
		return m_eFreezePhase == PS_EFreezePhase.FINISHED;
	}

	//------------------------------------------------------------------------------------------------
	bool IsDisableTimeEnd()
	{
		return m_eFreezePhase == PS_EFreezePhase.ORDINARY || m_eFreezePhase == PS_EFreezePhase.FINISHED;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFreezeSafetyActive()
	{
		if (!m_GameModeCoop)
			m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());

		if (!m_GameModeCoop)
			return false;

		return m_GameModeCoop.IsFreezeTimeShootingForbiden() && m_eFreezePhase == PS_EFreezePhase.ORDINARY;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFreezeEndTriggered()
	{
		return m_bFreezeEndTriggered;
	}

	//------------------------------------------------------------------------------------------------
	void ClearHUDReference()
	{
		m_hFreezeTimeCounter = null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		s_Instance = this;
		m_GameModeCoop = PS_GameModeCoop.Cast(owner);

		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
		GetGame().GetCallqueue().Remove(UpdateHardFreezeTimer_S);

		if (m_VehicleHold)
		{
			m_VehicleHold.Release_S();
			m_VehicleHold.Clear_S();
		}

		if (m_DamagePause)
		{
			m_DamagePause.ResumeAll_S();
			m_DamagePause.Clear_S();
		}

		RestoreDayAdvance_S();

		if (m_hFreezeTimeCounter)
		{
			Widget root = m_hFreezeTimeCounter.GetRootWidget();
			if (root)
				root.RemoveFromHierarchy();
			m_hFreezeTimeCounter = null;
		}

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (m_bHardFreeze && Replication.IsServer() && m_VehicleHold)
		{
			m_VehicleHold.Tick_S();
		}
	}

	//------------------------------------------------------------------------------------------------
	void StartFreeze_S()
	{
		if (!Replication.IsServer())
			return;

		if (!m_GameModeCoop)
			m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());

		int freezeTime = 0;
		if (m_iFreezeTimeOverride > 0)
			freezeTime = m_iFreezeTimeOverride;
		else if (m_iFreezeTimeOverride == 0)
			freezeTime = 0;
		else if (m_GameModeCoop)
			freezeTime = m_GameModeCoop.GetFreezeTime();

		int disableTime = 0;
		if (m_iDisableTimeOverride > 0)
			disableTime = m_iDisableTimeOverride;
		else if (m_iDisableTimeOverride == 0)
			disableTime = 0;
		else if (m_GameModeCoop)
			disableTime = m_GameModeCoop.GetDisableTime();

		m_iEffectiveFreezeTime = freezeTime;
		m_iEffectiveDisableTime = disableTime;
		m_bFreezeEndTriggered = false;

		if (m_iEffectiveDisableTime > 0)
		{
			m_bStartupFreeze = true;
			m_eFreezePhase = PS_EFreezePhase.STARTUP;
			m_fCurrentFreezeTime = m_iEffectiveFreezeTime;
			StartHardFreeze_S(m_iEffectiveDisableTime / 1000);
		}
		else
		{
			m_bStartupFreeze = false;
			if (m_iEffectiveFreezeTime > 0)
			{
				m_eFreezePhase = PS_EFreezePhase.ORDINARY;
				m_fCurrentFreezeTime = m_iEffectiveFreezeTime;
				if (m_VehicleHold)
					m_VehicleHold.StopAllVehicles_S();
				GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
				GetGame().GetCallqueue().CallLater(UpdateFreezeTimer_S, 1000, true);
			}
			else
			{
				m_eFreezePhase = PS_EFreezePhase.FINISHED;
				m_fCurrentFreezeTime = 0;
				OnFreezeTimeEnded_S();
			}
		}

		UpdateHUD_C();
		Replication.BumpMe();
	}


	//------------------------------------------------------------------------------------------------
	void StartHardFreeze_S(int seconds)
	{
		if (!Replication.IsServer())
			return;

		seconds = Math.Clamp(seconds, 0, 86400);

		if (seconds <= 0)
		{
			EndHardFreeze_S();
			return;
		}

		if (m_bHardFreeze)
		{
			m_fHardFreezeRemaining += seconds;
			m_fHardFreezeRemaining = Math.Clamp(m_fHardFreezeRemaining, 0, 86400);
			m_fHardFreezeDuration += seconds;
			UpdateHUD_C();
			Replication.BumpMe();
			return;
		}

		StopDayAdvance_S();

		m_bHardFreeze = true;
		m_fHardFreezeRemaining = seconds;
		m_fHardFreezeDuration = seconds;

		if (m_VehicleHold)
			m_VehicleHold.PinAll_S();

		if (m_DamagePause)
			m_DamagePause.PauseAll_S();

		GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
		GetGame().GetCallqueue().Remove(UpdateHardFreezeTimer_S);
		GetGame().GetCallqueue().CallLater(UpdateHardFreezeTimer_S, 1000, true);

		m_OnHardFreezeChanged.Invoke(true);

		if (!m_bStartupFreeze)
			SendHardFreezeNotification(true);

		UpdateHUD_C();
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void EndHardFreeze_S()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bHardFreeze)
			return;

		bool wasMidGamePause = !m_bStartupFreeze;
		m_bStartupFreeze = false;

		GetGame().GetCallqueue().Remove(UpdateHardFreezeTimer_S);
		m_bHardFreeze = false;
		m_fHardFreezeRemaining = 0;
		m_fHardFreezeDuration = 0;

		if (m_VehicleHold)
			m_VehicleHold.Release_S();

		if (m_DamagePause)
			m_DamagePause.ResumeAll_S();

		RestoreDayAdvance_S();

		m_OnHardFreezeChanged.Invoke(false);

		if (wasMidGamePause)
			SendHardFreezeNotification(false);

		if (m_eFreezePhase == PS_EFreezePhase.STARTUP)
		{
			m_eFreezePhase = PS_EFreezePhase.ORDINARY;
			if (m_fCurrentFreezeTime > 0)
			{
				GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
				GetGame().GetCallqueue().CallLater(UpdateFreezeTimer_S, 1000, true);
			}
			else
			{
				OnFreezeTimeEnded_S();
			}
		}
		else if (m_eFreezePhase == PS_EFreezePhase.ORDINARY)
		{
			if (m_fCurrentFreezeTime > 0)
			{
				GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
				GetGame().GetCallqueue().CallLater(UpdateFreezeTimer_S, 1000, true);
			}
			else
			{
				OnFreezeTimeEnded_S();
			}
		}

		UpdateHUD_C();
		Replication.BumpMe();
	}


	//------------------------------------------------------------------------------------------------
	void AdjustHardFreeze_S(int seconds)
	{
		if (!Replication.IsServer())
			return;

		StartHardFreeze_S(seconds);
	}

	//------------------------------------------------------------------------------------------------
	void FreezeTimerAdvance_S(int seconds)
	{
		if (!Replication.IsServer())
			return;

		if (m_eFreezePhase != PS_EFreezePhase.ORDINARY && m_eFreezePhase != PS_EFreezePhase.STARTUP)
			return;

		seconds = Math.Clamp(seconds, -86400, 86400);
		if (seconds == 0)
			return;

		float deltaMs = seconds * 1000;
		m_fCurrentFreezeTime += deltaMs;

		if (m_GameModeCoop)
			m_GameModeCoop.FreezeTimerAdvance_Notify();

		m_OnFreezeTimeAdjusted.Invoke(deltaMs);

		if (m_fCurrentFreezeTime <= 0)
		{
			m_fCurrentFreezeTime = 0;
			if (!m_bHardFreeze)
				OnFreezeTimeEnded_S();
		}

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void FreezeTimerEnd_S()
	{
		if (!Replication.IsServer())
			return;

		if (m_eFreezePhase == PS_EFreezePhase.FINISHED || m_bFreezeEndTriggered)
			return;

		m_bFreezeEndTriggered = true;

		float oldFreezeTime = m_fCurrentFreezeTime;
		m_fCurrentFreezeTime = 5000;

		if (m_GameModeCoop)
			m_GameModeCoop.FreezeTimerEnd_Notify();

		float deltaMs = 5000 - oldFreezeTime;
		m_OnFreezeTimeAdjusted.Invoke(deltaMs);

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateHardFreezeTimer_S()
	{
		if (!m_bHardFreeze)
			return;

		m_fHardFreezeRemaining -= 1.0;
		if (m_fHardFreezeRemaining <= 0)
		{
			EndHardFreeze_S();
		}
		else
		{
			Replication.BumpMe();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFreezeTimer_S()
	{
		if (m_bHardFreeze)
			return;

		m_fCurrentFreezeTime -= 1000;
		if (m_fCurrentFreezeTime <= 0)
		{
			m_fCurrentFreezeTime = 0;
			GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
			OnFreezeTimeEnded_S();
		}
		else
		{
			Replication.BumpMe();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFreezeTimeEnded_S()
	{
		if (m_eFreezePhase == PS_EFreezePhase.FINISHED)
			return;

		m_eFreezePhase = PS_EFreezePhase.FINISHED;
		m_fCurrentFreezeTime = 0;

		GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);

		if (m_GameModeCoop)
			m_GameModeCoop.OnFreezeTimeEnded_Internal();

		UpdateHUD_C();
		Replication.BumpMe();
	}


	//------------------------------------------------------------------------------------------------
	void SetFreezeTime_S(int freezeTime)
	{
		if (!Replication.IsServer())
			return;

		m_iEffectiveFreezeTime = freezeTime;
		Replication.BumpMe();
	}

	void UpdateHUD_C()
	{
		if (System.IsConsoleApp())
			return;

		if (!m_bHardFreeze && (m_eFreezePhase == PS_EFreezePhase.FINISHED || m_eFreezePhase == PS_EFreezePhase.NOT_STARTED))
		{
			if (m_hFreezeTimeCounter)
			{
				Widget root = m_hFreezeTimeCounter.GetRootWidget();
				if (root)
					root.RemoveFromHierarchy();
				m_hFreezeTimeCounter = null;
			}
			return;
		}

		if (!m_hFreezeTimeCounter || !m_hFreezeTimeCounter.GetRootWidget())
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{EC8A548C3F53BE4F}UI/layouts/FreezeTime/FreezeTimeCounter.layout");
			if (widget)
				m_hFreezeTimeCounter = PS_FreezeTimeCounter.Cast(widget.FindHandler(PS_FreezeTimeCounter));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRpl_HardFreeze()
	{
		m_OnHardFreezeChanged.Invoke(m_bHardFreeze);
		UpdateHUD_C();
	}

	protected void OnRpl_CurrentFreezeTime() {}
	protected void OnRpl_HardFreezeRemaining()
	{
		UpdateHUD_C();
	}
	protected void OnRpl_FreezePhase()
	{
		UpdateHUD_C();
	}
	protected void OnRpl_StartupFreeze()
	{
		UpdateHUD_C();
	}

	//------------------------------------------------------------------------------------------------
	protected void StopDayAdvance_S()
	{
		TimeAndWeatherManagerEntity timeManager = GetTimeManager();
		if (!timeManager)
			return;

		m_bDayAdvanceWasOn = timeManager.GetIsDayAutoAdvanced();
		if (m_bDayAdvanceWasOn)
			timeManager.SetIsDayAutoAdvanced(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void RestoreDayAdvance_S()
	{
		if (!m_bDayAdvanceWasOn)
			return;

		m_bDayAdvanceWasOn = false;

		TimeAndWeatherManagerEntity timeManager = GetTimeManager();
		if (timeManager)
			timeManager.SetIsDayAutoAdvanced(true);
	}

	//------------------------------------------------------------------------------------------------
	protected TimeAndWeatherManagerEntity GetTimeManager()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return null;

		return world.GetTimeAndWeatherManager();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Рассылка широковещательного системного уведомления о начале/снятии административной паузы
	 * @rpc Server -> Broadcast (Reliable)
	 * @param bStarted true = пауза активирована, false = пауза снята
	 */
	protected void SendHardFreezeNotification(bool bStarted)
	{
		if (RplSession.Mode() == RplMode.None)
			RPC_HardFreezeNotify(bStarted);
		else
			Rpc(RPC_HardFreezeNotify, bStarted);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_HardFreezeNotify(bool bStarted)
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		if (!chatPanelManager)
			return;

		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		if (!invoker)
			return;

		if (bStarted)
			invoker.Invoke(null, "#PS-HardFreeze_Started");
		else
			invoker.Invoke(null, "#PS-HardFreeze_Ended");
	}
}

