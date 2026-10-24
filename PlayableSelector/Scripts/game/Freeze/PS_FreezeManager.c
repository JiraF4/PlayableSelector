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

void PS_OnFreezePhaseChangedMethod(PS_EFreezePhase oldPhase, PS_EFreezePhase newPhase);
typedef func PS_OnFreezePhaseChangedMethod;
typedef ScriptInvokerBase<PS_OnFreezePhaseChangedMethod> PS_OnFreezePhaseChangedInvoker;

void PS_OnFreezeEndedMethod();
typedef func PS_OnFreezeEndedMethod;
typedef ScriptInvokerBase<PS_OnFreezeEndedMethod> PS_OnFreezeEndedInvoker;

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

	[Attribute("-1", UIWidgets.EditBox, "Shooting forbidden override (-1 = use GameMode attribute, 0 = allow shooting, 1 = forbid shooting)", category: "Freeze Manager")]
	protected int m_iShootingForbiddenOverride;

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

	[RplProp()]
	protected bool m_bEffectiveShootingForbidden;

	[RplProp(onRplName: "OnRpl_FreezePhase")]
	protected PS_EFreezePhase m_eFreezePhase = PS_EFreezePhase.NOT_STARTED;

	[RplProp(onRplName: "OnRpl_StartupFreeze")]
	protected bool m_bStartupFreeze;

	protected ref PS_OnHardFreezeChangedInvoker m_OnHardFreezeChanged = new PS_OnHardFreezeChangedInvoker();
	protected ref PS_OnFreezeTimeAdjustedInvoker m_OnFreezeTimeAdjusted = new PS_OnFreezeTimeAdjustedInvoker();
	protected ref PS_OnFreezePhaseChangedInvoker m_OnFreezePhaseChanged = new PS_OnFreezePhaseChangedInvoker();
	protected ref PS_OnFreezeEndedInvoker m_OnFreezeEnded = new PS_OnFreezeEndedInvoker();

	protected ref PS_VehicleHold m_VehicleHold = new PS_VehicleHold();
	protected ref PS_DamagePause m_DamagePause = new PS_DamagePause();

	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_FreezeTimeCounter m_hFreezeTimeCounter;
	protected PS_EFreezePhase m_eLastNotifiedFreezePhase = PS_EFreezePhase.NOT_STARTED;
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
	/**
	 * @brief Вызывается на сервере и клиентах (через OnRpl_FreezePhase) только при фактической смене фазы (oldPhase != newPhase).
	 */
	PS_OnFreezePhaseChangedInvoker GetOnFreezePhaseChanged()
	{
		return m_OnFreezePhaseChanged;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Вызывается однократно за период фриза на сервере и клиентах при переходе в PS_EFreezePhase.FINISHED.
	 */
	PS_OnFreezeEndedInvoker GetOnFreezeEnded()
	{
		return m_OnFreezeEnded;
	}

	//------------------------------------------------------------------------------------------------
	bool IsEffectiveShootingForbidden()
	{
		return m_bEffectiveShootingForbidden;
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
		return m_bEffectiveShootingForbidden && m_eFreezePhase == PS_EFreezePhase.ORDINARY;
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

		if (Replication.IsServer())
			ResolveEffectiveShootingForbidden_S();

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
	protected void ResolveEffectiveShootingForbidden_S()
	{
		if (!Replication.IsServer())
			return;

		if (!m_GameModeCoop)
			m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());

		bool shootingForbidden = false;
		if (m_iShootingForbiddenOverride > 0)
			shootingForbidden = true;
		else if (m_iShootingForbiddenOverride == 0)
			shootingForbidden = false;
		else if (m_GameModeCoop)
			shootingForbidden = m_GameModeCoop.IsFreezeTimeShootingForbiden();

		if (m_bEffectiveShootingForbidden != shootingForbidden)
		{
			m_bEffectiveShootingForbidden = shootingForbidden;
			Replication.BumpMe();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFreezePhase_S(PS_EFreezePhase newPhase)
	{
		if (!Replication.IsServer())
			return;

		if (newPhase == m_eFreezePhase)
			return;

		PS_EFreezePhase oldPhase = m_eFreezePhase;
		m_eFreezePhase = newPhase;
		m_eLastNotifiedFreezePhase = newPhase;

		m_OnFreezePhaseChanged.Invoke(oldPhase, newPhase);
		if (newPhase == PS_EFreezePhase.FINISHED)
			m_OnFreezeEnded.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Сбрасывает состояние зон у всех игроков и удаляет сущности SCR_EditorRestrictionZoneEntity. Вызывается только на сервере.
	 */
	void RemoveRestrictedZones_S()
	{
		if (!Replication.IsServer())
			return;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		SCR_PlayersRestrictionZoneManagerComponent restrictionZoneManager = SCR_PlayersRestrictionZoneManagerComponent.Cast(gameMode.FindComponent(SCR_PlayersRestrictionZoneManagerComponent));
		if (!restrictionZoneManager)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> playerIds = new array<int>();
			playerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
			{
				restrictionZoneManager.ResetPlayerZoneData(playerId);
			}
		}

		set<SCR_EditorRestrictionZoneEntity> zones = restrictionZoneManager.GetZones();
		if (!zones)
			return;

		for (int i = 0; i < zones.Count(); i++)
		{
			SCR_EditorRestrictionZoneEntity zone = zones.Get(i);
			if (zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
	}

	//------------------------------------------------------------------------------------------------
	void StartFreeze_S()
	{
		if (!Replication.IsServer())
			return;

		if (!m_GameModeCoop)
			m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());

		ResolveEffectiveShootingForbidden_S();

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
			SetFreezePhase_S(PS_EFreezePhase.STARTUP);
			m_fCurrentFreezeTime = m_iEffectiveFreezeTime;
			StartHardFreeze_S(m_iEffectiveDisableTime / 1000);
		}
		else
		{
			m_bStartupFreeze = false;
			if (m_iEffectiveFreezeTime > 0)
			{
				SetFreezePhase_S(PS_EFreezePhase.ORDINARY);
				m_fCurrentFreezeTime = m_iEffectiveFreezeTime;
				if (m_VehicleHold)
					m_VehicleHold.StopAllVehicles_S();
				GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);
				GetGame().GetCallqueue().CallLater(UpdateFreezeTimer_S, 1000, true);
			}
			else
			{
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

		seconds = Math.Clamp(seconds, -86400, 86400);

		if (!m_bHardFreeze)
		{
			if (seconds <= 0)
				return;

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
				HardFreeze_Notify(true);

			UpdateHUD_C();
			Replication.BumpMe();
			return;
		}

		if (m_fHardFreezeRemaining <= 5.0)
		{
			if (seconds == 0 || m_fHardFreezeRemaining + seconds <= 0.0)
			{
				EndHardFreeze_S();
				return;
			}

			m_fHardFreezeRemaining += seconds;
			m_fHardFreezeRemaining = Math.Clamp(m_fHardFreezeRemaining, 0.0, 86400.0);
			if (seconds > 0)
				m_fHardFreezeDuration = Math.Max(m_fHardFreezeDuration + seconds, m_fHardFreezeRemaining);
			UpdateHUD_C();
			Replication.BumpMe();
			return;
		}

		if (seconds == 0 || (seconds < 0 && m_fHardFreezeRemaining + seconds <= 5.0))
		{
			m_fHardFreezeRemaining = 5.0;
			if (!m_bStartupFreeze)
				SendSystemMessage_S("#PS-HardFreeze_EndingSoon");
			UpdateHUD_C();
			Replication.BumpMe();
			return;
		}

		m_fHardFreezeRemaining += seconds;
		m_fHardFreezeRemaining = Math.Clamp(m_fHardFreezeRemaining, 0.0, 86400.0);
		if (seconds > 0)
			m_fHardFreezeDuration = Math.Max(m_fHardFreezeDuration + seconds, m_fHardFreezeRemaining);
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
			HardFreeze_Notify(false);

		if (m_eFreezePhase == PS_EFreezePhase.STARTUP)
		{
			if (m_fCurrentFreezeTime > 0)
			{
				SetFreezePhase_S(PS_EFreezePhase.ORDINARY);
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
		float targetFreezeTime = m_fCurrentFreezeTime + deltaMs;

		if (seconds < 0 && targetFreezeTime <= 5000.0 && !m_bHardFreeze)
		{
			if (m_fCurrentFreezeTime > 5000.0)
			{
				FreezeTimerEnd_S();
				return;
			}

			if (!m_bFreezeEndTriggered)
			{
				m_bFreezeEndTriggered = true;
				SendSystemMessage_S("#PS-Freeze_Time_ForceEnd");
			}

			float remainingDeltaMs = -m_fCurrentFreezeTime;
			m_fCurrentFreezeTime = 0;
			m_OnFreezeTimeAdjusted.Invoke(remainingDeltaMs);
			OnFreezeTimeEnded_S();
			return;
		}

		m_fCurrentFreezeTime = targetFreezeTime;

		if (m_fCurrentFreezeTime > 5000.0)
			m_bFreezeEndTriggered = false;

		SendSystemMessage_S("#PS-Freeze_Time_Advanced");

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

		SendSystemMessage_S("#PS-Freeze_Time_ForceEnd");

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

		m_fCurrentFreezeTime = 0;
		GetGame().GetCallqueue().Remove(UpdateFreezeTimer_S);

		RemoveRestrictedZones_S();
		if (!m_bFreezeEndTriggered)
			SendSystemMessage_S("#PS-Freeze_End");

		SetFreezePhase_S(PS_EFreezePhase.FINISHED);

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
		PS_EFreezePhase oldPhase = m_eLastNotifiedFreezePhase;
		m_eLastNotifiedFreezePhase = m_eFreezePhase;

		if (oldPhase != m_eFreezePhase)
		{
			m_OnFreezePhaseChanged.Invoke(oldPhase, m_eFreezePhase);
			if (m_eFreezePhase == PS_EFreezePhase.FINISHED)
				m_OnFreezeEnded.Invoke();
		}

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
	protected void SendSystemMessage_S(string messageKey)
	{
		if (!Replication.IsServer())
			return;

		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		if (!chatPanelManager)
			return;

		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		if (!invoker)
			return;

		invoker.Invoke(null, messageKey);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Рассылка системного smsg-уведомления о начале/снятии административной паузы
	 * @param bStarted true = пауза активирована, false = пауза снята
	 */
	protected void HardFreeze_Notify(bool bStarted)
	{
		if (bStarted)
			SendSystemMessage_S("#PS-HardFreeze_Started");
		else
			SendSystemMessage_S("#PS-HardFreeze_Ended");
	}
}

