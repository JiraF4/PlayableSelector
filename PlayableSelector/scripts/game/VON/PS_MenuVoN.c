// PS_MenuVoN — client-local press-to-talk for players without a living character.
// Ported from LiteLobby (LL_MenuVoN).
//
// The "talking device" is the player's replicated VoN proxy entity (spawned by
// PS_VoNRoomsManager — see PS_VoNProxyComponent for WHY it must be a separate,
// everywhere-replicated entity). While the player is a menu speaker (no controlled
// entity, or a corpse), the proxy's VoN component is connected to the engine VoN system
// the same way the Game Master editor connects its own — ConnectEditorToVoNSystem(playerId)
// — and the VONDirect key drives SetCapture directly. Room routing uses the proxy's virtual
// position; its radio transceiver stays muted.
//
// ShouldOwnVoice is the single owner decision; Refresh applies it to the local proxy.
// IsActive reports proxy readiness and is not an ownership decision.

class PS_MenuVoN
{
	protected static ref PS_MenuVoN s_Instance;

	protected SCR_VoNComponent m_VoNComp;
	protected BaseTransceiver m_Transceiver;
	protected bool m_bActive;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected PlayerController m_ActivePc; // controller we activated for; differs after a reconnect -> re-acquire

	// =====================================================================
	// PUBLIC API
	// =====================================================================

	static void Refresh()
	{
		// Dedicated servers have no microphone and no local player.
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (!s_Instance)
			s_Instance = new PS_MenuVoN();

		s_Instance.RefreshInternal();
	}

	static bool IsActive()
	{
		return s_Instance && s_Instance.m_bActive;
	}

	static bool ShouldOwnVoice()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return false;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() <= 0)
			return false;

		return SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()) && !IsEditorOpened();
	}

	// =====================================================================
	// OWNERSHIP RULE
	// =====================================================================

	protected void RefreshInternal()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() <= 0)
		{
			Deactivate();
			UnsubscribeEditor();
			ScheduleRetryRefresh();
			return;
		}

		GetGame().GetCallqueue().Remove(RetryRefresh);
		if (!TrySubscribeEditor())
			ScheduleRetryRefresh();

		if (m_bActive)
		{
			IEntity currentProxy = PS_VoNProxyComponent.GetProxyEntity(pc.GetPlayerId());
			SCR_VoNComponent currentVonComp;
			if (currentProxy)
				currentVonComp = SCR_VoNComponent.Cast(currentProxy.FindComponent(SCR_VoNComponent));

			if (currentVonComp != m_VoNComp || pc != m_ActivePc)
				Deactivate();
		}

		if (ShouldOwnVoice())
			Activate(pc);
		else
			Deactivate();

		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
			vonMgr.MuteProxyRadio(pc.GetPlayerId());
	}

	// An opened editor connects its own component for the same player id; two connected
	// components fight over the sender slot.
	protected static bool IsEditorOpened()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager();
		if (!editorManager)
			return false;

		return editorManager.IsOpened();
	}

	protected bool TrySubscribeEditor()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		SCR_EditorManagerEntity editorManager;
		if (core)
			editorManager = core.GetEditorManager();

		if (editorManager == m_EditorManager)
			return m_EditorManager != null;

		UnsubscribeEditor();
		if (!editorManager)
			return false;

		m_EditorManager = editorManager;
		m_EditorManager.GetOnOpened().Insert(OnEditorToggled);
		m_EditorManager.GetOnClosed().Insert(OnEditorToggled);
		return true;
	}

	protected void UnsubscribeEditor()
	{
		if (!m_EditorManager)
			return;

		m_EditorManager.GetOnOpened().Remove(OnEditorToggled);
		m_EditorManager.GetOnClosed().Remove(OnEditorToggled);
		m_EditorManager = null;
	}

	protected void OnEditorToggled()
	{
		RefreshInternal();
	}

	protected void ScheduleRetryRefresh()
	{
		GetGame().GetCallqueue().Remove(RetryRefresh);
		GetGame().GetCallqueue().CallLater(RetryRefresh, 500, false);
	}

	protected void RetryRefresh()
	{
		RefreshInternal();
	}

	protected void Activate(notnull PlayerController pc)
	{
		if (m_bActive)
			return;

		// Detach a late body/editor assignment even when this proxy is already active.
		SCR_VONController vonController = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		if (vonController && vonController.GetVONComponent())
			vonController.SetVONComponent(null);

		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(pc.GetPlayerId());
		if (!proxy)
		{
			ScheduleRetryRefresh();
			return;
		}

		SCR_VoNComponent vonComp = SCR_VoNComponent.Cast(proxy.FindComponent(SCR_VoNComponent));
		if (!vonComp)
		{
			Print("[PS_VoN] MenuVoN: no SCR_VoNComponent on the VoN proxy prefab - menu voice disabled", LogLevel.WARNING);
			return;
		}

		SCR_RadioComponent radioGadget = SCR_RadioComponent.Cast(proxy.FindComponent(SCR_RadioComponent));
		if (radioGadget)
			radioGadget.OnPostInit(proxy);

		BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
		if (!radio || radio.TransceiversCount() < 1)
		{
			Print("[PS_VoN] MenuVoN: no radio transceiver on the VoN proxy prefab - menu voice disabled", LogLevel.WARNING);
			return;
		}

		BaseTransceiver proxyTransceiver = radio.GetTransceiver(0);
		if (!proxyTransceiver)
		{
			Print("[PS_VoN] MenuVoN: no transceiver on the VoN proxy radio - menu voice disabled", LogLevel.WARNING);
			return;
		}
		
		m_VoNComp = vonComp;
		m_Transceiver = radio.GetTransceiver(0);
		//m_Transceiver.SetMuteState(true);

		m_VoNComp.ConnectEditorToVoNSystem(pc.GetPlayerId());
		// Armed on the radio for reception as well, not only while the talk key is held.
		m_VoNComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
		m_VoNComp.SetTransmitRadio(m_Transceiver);

		SCR_VoNComponent.InvalidateEditorLocCache(pc.GetPlayerId());
		ArmDirect();

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, OnTalkDown);
		inputManager.AddActionListener("VONDirect", EActionTrigger.UP, OnTalkUp);

		m_bActive = true;
		m_ActivePc = pc;
		Print("[PS_VoN] MenuVoN: activated", LogLevel.NORMAL);
	}
	protected void Deactivate()
	{
		if (!m_bActive)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			SCR_VoNComponent.InvalidateEditorLocCache(pc.GetPlayerId());

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, OnTalkDown);
		inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, OnTalkUp);

		if (m_VoNComp)
		{
			m_VoNComp.SetCapture(false);
			m_VoNComp.DisconnectEditorFromVoNSystem();
		}

		m_VoNComp = null;
		m_Transceiver = null;
		m_bActive = false;
		m_ActivePc = null;
		Print("[PS_VoN] MenuVoN: deactivated", LogLevel.NORMAL);
	}

	// =====================================================================
	// PRESS-TO-TALK
	// =====================================================================

	protected void ArmDirect()
	{
		if (!m_VoNComp)
			return;

		m_VoNComp.SetCommMethod(ECommMethod.DIRECT);
		m_VoNComp.SetTransmitRadio(null);
	}

	protected void OnTalkDown()
	{
		if (!m_bActive || !m_VoNComp)
			return;

		ArmDirect();
		m_VoNComp.SetCapture(true);
	}

	protected void OnTalkUp()
	{
		if (m_VoNComp)
			m_VoNComp.SetCapture(false);
	}
}
