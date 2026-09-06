// PS_MenuVoN — client-local press-to-talk for players without a living character.
// Ported from LiteLobby (LL_MenuVoN).
//
// The "talking device" is the player's replicated VoN proxy entity (spawned by
// PS_VoNRoomsManager — see PS_VoNProxyComponent for WHY it must be a separate,
// everywhere-replicated entity). While the player is a menu speaker (no controlled
// entity, or a corpse), the proxy's VoN component is connected to the engine VoN system
// the same way the Game Master editor connects its own — ConnectEditorToVoNSystem(playerId)
// — and the VONDirect key drives SetCapture directly. Room routing (frequency + key on the
// proxy radio) is owned by PS_VoNRoomsManager.ApplyRadioKey.
//
// Single ownership rule (Refresh): local player is a menu speaker AND the GM editor is
// closed -> device active (connected + PTT listeners); otherwise inactive. Refresh() is
// idempotent; call it from every hook that could change the answer.

class PS_MenuVoN
{
	protected static ref PS_MenuVoN s_Instance;

	protected SCR_VoNComponent m_VoNComp;
	protected BaseTransceiver m_Transceiver;
	protected bool m_bActive;
	protected bool m_bEditorSubscribed;
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

	// =====================================================================
	// OWNERSHIP RULE
	// =====================================================================

	protected void RefreshInternal()
	{
		PlayerController pc = GetGame().GetPlayerController();
		// On dedicated clients the controller exists before the engine assigns the player
		// id (ids start at 1) - connecting under id 0 would register the wrong sender. Keep
		// retrying: during slot selection no later hook fires once the id arrives.
		if (!pc || pc.GetPlayerId() <= 0)
		{
			Deactivate();
			GetGame().GetCallqueue().Remove(RetryRefresh);
			GetGame().GetCallqueue().CallLater(RetryRefresh, 500, false);
			return;
		}

		TrySubscribeEditor();

		// RECONNECT / proxy-respawn guard. PS_MenuVoN is a CLIENT singleton that survives a reconnect, but
		// the VoN proxy it bound to is destroyed and a NEW one is spawned for the new playerId. If we are
		// still "active" on a proxy that is no longer this player's current one, the binding is stale
		// (m_VoNComp points at the dead old proxy) and Activate() would early-return on m_bActive - leaving
		// a reconnected spectator able to HEAR but not SPEAK (exactly the reported bug, which only shows
		// with reconnect, impossible to hit in peer tools). Drop the stale binding so Activate() re-acquires
		// the current proxy below.
		if (m_bActive)
		{
			IEntity currentProxy = PS_VoNProxyComponent.GetProxyEntity(pc.GetPlayerId());
			SCR_VoNComponent currentVonComp;
			if (currentProxy)
				currentVonComp = SCR_VoNComponent.Cast(currentProxy.FindComponent(SCR_VoNComponent));
			// Re-acquire if the proxy changed (respawn) OR the player CONTROLLER changed (reconnect keeps the
			// same playerId and the server reuses the same proxy entity, but the controller + editor manager
			// are new - our SetVONComponent(null) and client-side editor-VoN registration were done on the
			// dead old controller and must be redone, or the reconnected client can't transmit).
			if (currentVonComp != m_VoNComp || pc != m_ActivePc)
				Deactivate();
		}

		bool shouldBeActive = SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()) && !IsEditorOpened();

		if (shouldBeActive)
			Activate(pc);
		else
			Deactivate();

		// Keep the LOCAL proxy radio tuning in sync with speaker-ness (room while a menu
		// speaker, parked while alive) - the server does the same for its authoritative copy.
		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
			vonMgr.ApplyRadioKey(pc.GetPlayerId());
	}

	// Yield to the editor: an opened GM editor connects ITS VoN component for the same
	// player id - two connected components for one player fight over the sender slot.
	protected bool IsEditorOpened()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager();
		if (!editorManager)
			return false;

		return editorManager.IsOpened();
	}

	// The local editor manager is created asynchronously after connect - keep trying from
	// Refresh() until it exists, then subscribe exactly once.
	protected void TrySubscribeEditor()
	{
		if (m_bEditorSubscribed)
			return;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager();
		if (!editorManager)
			return;

		editorManager.GetOnOpened().Insert(OnEditorToggled);
		editorManager.GetOnClosed().Insert(OnEditorToggled);
		m_bEditorSubscribed = true;
	}

	protected void OnEditorToggled()
	{
		RefreshInternal();
	}

	protected void RetryRefresh()
	{
		RefreshInternal();
	}

	// =====================================================================
	// ACTIVATION
	// =====================================================================

	protected void Activate(notnull PlayerController pc)
	{
		if (m_bActive)
			return;

		// The menu device owns the voice while active - detach whatever VoN component
		// SCR_VONController holds. The controller's own VONDirect handlers capture on THEIR
		// component whenever one is set: the disconnected editor component after closing GM
		// without a controlled entity, or the CORPSE's component after death (the direct-speech
		// path is not life-state gated - T would capture inaudible proximity speech on the
		// corpse instead of transmitting through the menu radio: icon shows, nobody hears).
		// Vanilla re-sets the component on possession and on editor open, so this is self-healing.
		SCR_VONController vonController = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		if (vonController && vonController.GetVONComponent())
			vonController.SetVONComponent(null);

		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(pc.GetPlayerId());
		if (!proxy)
		{
			// Server spawns the proxy at connect - it may still be streaming in. Retry.
			GetGame().GetCallqueue().Remove(RetryRefresh);
			GetGame().GetCallqueue().CallLater(RetryRefresh, 500, false);
			return;
		}

		SCR_VoNComponent vonComp = SCR_VoNComponent.Cast(proxy.FindComponent(SCR_VoNComponent));
		if (!vonComp)
		{
			Print("[PS_VoN] MenuVoN: no SCR_VoNComponent on the VoN proxy prefab - menu voice disabled", LogLevel.WARNING);
			return;
		}

		// Same forced gadget init the editor manager does on open - on the owning client too.
		SCR_RadioComponent radioGadget = SCR_RadioComponent.Cast(proxy.FindComponent(SCR_RadioComponent));
		if (radioGadget)
			radioGadget.OnPostInit(proxy);

		BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
		if (!radio || radio.TransceiversCount() < 1)
		{
			Print("[PS_VoN] MenuVoN: no radio transceiver on the VoN proxy prefab - menu voice disabled", LogLevel.WARNING);
			return;
		}

		m_VoNComp = vonComp;
		m_Transceiver = radio.GetTransceiver(0);

		// Register as a non-character sender. Without this the engine never even starts
		// capture for the component (no OnCapture, no transmission) - there is no plain-radio
		// path for players without a controlled entity.
		m_VoNComp.ConnectEditorToVoNSystem(pc.GetPlayerId());

		// Arm the component on the radio for RECEPTION too, not only while the key is held.
		m_VoNComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
		m_VoNComp.SetTransmitRadio(m_Transceiver);

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, OnTalkDown);
		inputManager.AddActionListener("VONDirect", EActionTrigger.UP, OnTalkUp);

		m_bActive = true;
		m_ActivePc = pc; // remember the controller so a reconnect (new controller) forces a re-acquire
		// Keep the VON controller registered + its input context active even while we control a dead corpse,
		// so VONDirect keeps reaching OnTalkDown (see PS_M_SCR_VONController). Without this a died-with-a-slot
		// spectator can hear but not speak.
		if (vonController)
			vonController.PS_RefreshSystemState();
		Print("[PS_VoN] MenuVoN: activated", LogLevel.NORMAL);
	}

	protected void Deactivate()
	{
		if (!m_bActive)
			return;

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
		// Let the VON controller revert to vanilla update-system registration now the menu device is off.
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
		{
			SCR_VONController vonController = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
			if (vonController)
				vonController.PS_RefreshSystemState();
		}
		Print("[PS_VoN] MenuVoN: deactivated", LogLevel.NORMAL);
	}

	// =====================================================================
	// RECEIVE RE-ARM (follow the channel)
	// =====================================================================

	// Re-point the LOCAL player's menu-voice RECEPTION at their proxy transceiver after a room change.
	// Reception is armed once in Activate() (SetTransmitRadio) and otherwise stays on whatever room was active
	// then - so after moving rooms a player keeps HEARING the old (often the Global connect) channel while their
	// transmit correctly follows the new one. PS_VoNRoomsManager.ApplyRadioKeyNow calls this for the local
	// player right after it re-tunes the transceiver, so listening follows the channel the same way talking does.
	static void PS_ReapplyReceive(BaseTransceiver tsv)
	{
		if (s_Instance)
			s_Instance.ReapplyReceiveInternal(tsv);
	}
	protected void ReapplyReceiveInternal(BaseTransceiver tsv)
	{
		if (!m_bActive || !m_VoNComp || !tsv)
			return;
		// Safety: only re-arm while still bound to the local player's CURRENT proxy. After a reconnect/respawn
		// the binding can be stale (old, destroyed proxy); RefreshInternal's reconnect guard re-acquires that
		// case, so skip here rather than touch a dead component.
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(pc.GetPlayerId());
		if (!proxy)
			return;
		SCR_VoNComponent currentVonComp = SCR_VoNComponent.Cast(proxy.FindComponent(SCR_VoNComponent));
		if (currentVonComp != m_VoNComp)
			return;
		m_Transceiver = tsv; // keep in sync with the proxy's freshly-tuned transceiver
		m_VoNComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
		m_VoNComp.SetTransmitRadio(tsv);
	}

	// =====================================================================
	// PRESS-TO-TALK
	// =====================================================================

	protected void OnTalkDown()
	{
		if (!m_bActive || !m_VoNComp || !m_Transceiver)
			return;

		// Radio-only transmission: everyone whose menu radio shares our frequency
		// (= same voice channel) hears it.
		m_VoNComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
		m_VoNComp.SetTransmitRadio(m_Transceiver);
		m_VoNComp.SetCapture(true);
	}

	protected void OnTalkUp()
	{
		if (m_VoNComp)
			m_VoNComp.SetCapture(false);
	}
}
