class PS_ContextMenu : SCR_ScriptedWidgetComponent
{
	const static ResourceName IMAGESET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	const static ResourceName IMAGESET_PS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	const static string CONTEXT_MENU_PREFAB = "{35274D65CB0F52D6}UI/ContextMenu/ContextMenu.layout";
	const static string CONTEXT_ACTION_PREFAB = "{E773ED1D9DD8E8A8}UI/ContextMenu/ContextAction.layout";
	const static string CONTEXT_SEPARATOR_PREFAB = "{3F8E97AEB2C0E361}UI/ContextMenu/ContextActionSeparator.layout";
	static Widget s_wContextMenuWidget;
	
	// Widgets
	protected VerticalLayoutWidget m_wActionsVerticalLayout;
	protected OverlayWidget m_wNoActions;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
	
		m_wActionsVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("ActionsVerticalLayout"));
		m_wNoActions = OverlayWidget.Cast(w.FindAnyWidget("NoActions"));
		
		GetGame().GetInputManager().AddActionListener("MouseLeft", EActionTrigger.UP, CloseMenu);
		GetGame().GetInputManager().AddActionListener("MouseRight", EActionTrigger.DOWN, CloseMenu);
	}
	
	static PS_ContextMenu CreateContextMenuOnMousePosition(Widget rootWidget)
	{
		if (s_wContextMenuWidget)
			s_wContextMenuWidget.RemoveFromHierarchy();
		s_wContextMenuWidget = GetGame().GetWorkspace().CreateWidgets(CONTEXT_MENU_PREFAB, rootWidget);
		int x, y;
		WidgetManager.GetMousePos(x, y);
		x = GetGame().GetWorkspace().DPIUnscale(x);
		y = GetGame().GetWorkspace().DPIUnscale(y);
		FrameSlot.SetPos(s_wContextMenuWidget, x, y);
		PS_ContextMenu contextMenu = PS_ContextMenu.Cast(s_wContextMenuWidget.FindHandler(PS_ContextMenu));
		return contextMenu;
	}
	
	PS_ContextAction AddAction(ResourceName icon, string quad, string name, string actionName, PS_ContextActionData contextActionData)
	{
		if (m_wNoActions)
			m_wNoActions.RemoveFromHierarchy();
		if (m_wActionsVerticalLayout.GetChildren())
		{
			GetGame().GetWorkspace().CreateWidgets(CONTEXT_SEPARATOR_PREFAB, m_wActionsVerticalLayout);
		}
		Widget contextActionWidget = GetGame().GetWorkspace().CreateWidgets(CONTEXT_ACTION_PREFAB, m_wActionsVerticalLayout);
		PS_ContextAction contextAction = PS_ContextAction.Cast(contextActionWidget.FindHandler(PS_ContextAction));
		contextAction.Init(icon, quad, name, actionName, contextActionData);
		return contextAction;
	}
	
	void CloseMenu()
	{
		GetGame().GetCallqueue().Call(CloseMenuDelay);
	}
	void CloseMenuDelay()
	{
		if (s_wContextMenuWidget)
			s_wContextMenuWidget.RemoveFromHierarchy();
	}
	
	// Actions player
	void ActionPlayerSelect(int playerId)
	{
		PS_CoopLobby coopLobby = PS_CoopLobby.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (!coopLobby)
			return;
		if (coopLobby.GetSelectedPlayer() == playerId)
			return;
		
		string name = "Select player (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET, "exit", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionPlayerSelect);
	}
	void OnActionPlayerSelect(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		PS_CoopLobby coopLobby = PS_CoopLobby.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (!coopLobby)
			return;
		
		coopLobby.SetSelectedPlayer(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionKick(int playerId)
	{
		if (GetGame().GetPlayerController().GetPlayerId() == playerId)
			return;
		
		string name = "Kick (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET, "kickCommandAlt", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionKick);
	}
	void OnActionKick(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
		PS_PlayableManager.GetPlayableController().KickPlayer(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionMute(int playerId)
	{
		string name = "Mute player (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET_PS, "VoNDisabled", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionMute);
	}
	void OnActionMute(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		PermissionState mute = PermissionState.DISALLOWED;
		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		socialComp.SetMuted(contextActionDataPlayer.GetPlayerId(), true);
		PS_VoiceButton.m_OnMuteStateChanged.Invoke(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionUnmute(int playerId)
	{
		string name = "Unmute player (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET_PS, "VoNDirect", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionUnmute);
	}
	void OnActionUnmute(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		PermissionState mute = PermissionState.DISALLOWED;
		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		socialComp.SetMuted(contextActionDataPlayer.GetPlayerId(), false);
		PS_VoiceButton.m_OnMuteStateChanged.Invoke(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionUnpin(int playerId)
	{
		string name = "Unpin player (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET, "pinPlay", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionUnpin);
	}
	void OnActionUnpin(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_E_LAYER_BACK");
		PS_PlayableManager.GetInstance().GetPlayableController().UnpinPlayer(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionPin(int playerId)
	{
		string name = "Pin player (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET, "pinPlay", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionPin);
	}
	void OnActionPin(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_E_LAYER_EDIT_START");
		PS_PlayableManager.GetInstance().GetPlayableController().PinPlayer(contextActionDataPlayer.GetPlayerId());
	}
	
	PS_ScriptInvokerOnContextAction ActionVoiceKick(int playerId)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
		string name = "Kick from voice room (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET, "kickCommandAlt", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction();
	}
	
	
	// Actions playable
	PS_ScriptInvokerOnContextAction ActionFreeSlot(RplId playableId)
	{
		return AddAction(IMAGESET, "kickCommandAlt", "Free slot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionLock(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Locked", "Lock slot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionUnlock(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Unlocked", "Unlock slot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionOpenInventory(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Backpack", "Open inventory", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	void ActionRespawnInPlace(RplId playableId, int playerId)
	{
		string name = "Respawn";
		if (playerId > 0)
			name += " (" + GetPlayerName(playerId) + ")";
		return AddAction(IMAGESET_PS, "Medicine", name, "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction().Insert(OnRespawnInPlace);
	}
	void OnRespawnInPlace(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayer)
	{
		PS_PlayableControllerComponent playableController = PS_PlayableManager.GetPlayableController();
		playableController.RespawnPlayable(contextActionDataPlayer.GetPlayableId(), false);
	}
	
	// Actions spectator
	PS_ScriptInvokerOnContextAction ActionAttachTo(SCR_ChimeraCharacter character)
	{
		string name = "Attach to";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionDetachFrom(SCR_ChimeraCharacter character)
	{
		string name = "Detach from";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionLookAt(SCR_ChimeraCharacter character)
	{
		string name = "Look at";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionFirstPersonView(SCR_ChimeraCharacter character)
	{
		string name = "First person view";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	
	// Actions voice
	PS_ScriptInvokerOnContextAction ActionJoinVoice()
	{
		return AddAction(IMAGESET_PS, "RoomEnter", "Join voice room", "",
			new PS_ContextActionData()
		).GetOnOnContextAction();
	}
	
	// Util
	string GetPlayerName(int playerId)
	{
		return PS_PlayableManager.GetInstance().GetPlayerName(playerId);
	}
}