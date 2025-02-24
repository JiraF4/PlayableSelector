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
	protected ButtonWidget m_wHeader;
	protected TextWidget m_wHeaderText;
	
	protected int height = 32;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
	
		m_wActionsVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("ActionsVerticalLayout"));
		m_wNoActions = OverlayWidget.Cast(w.FindAnyWidget("NoActions"));
		m_wHeaderText = TextWidget.Cast(w.FindAnyWidget("HeaderText"));
		m_wHeader = ButtonWidget.Cast(w.FindAnyWidget("Header"));
		
		GetGame().GetInputManager().AddActionListener("MouseLeft", EActionTrigger.UP, CloseMenu);
		GetGame().GetInputManager().AddActionListener("MouseRight", EActionTrigger.DOWN, CloseMenu);
	}
	
	static PS_ContextMenu CreateContextMenuOnMousePosition(Widget rootWidget, string contextName)
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
		if (contextName != "")
			contextMenu.m_wHeaderText.SetText(contextName);
		else
		{
			contextMenu.m_wHeader.SetVisible(false);
			contextMenu.height -= 24;
		}
		GetGame().GetCallqueue().Call(contextMenu.MoveFromOffscreen);
		return contextMenu;
	}
	
	void MoveFromOffscreen()
	{
		int x = FrameSlot.GetPosX(m_wRoot);
		int y = FrameSlot.GetPosY(m_wRoot);
		int width = 268;
		
		float screenWidth, screenHeight;
		GetGame().GetWorkspace().GetScreenSize(screenWidth, screenHeight);
		screenWidth = GetGame().GetWorkspace().DPIUnscale(screenWidth);
		screenHeight = GetGame().GetWorkspace().DPIUnscale(screenHeight);
		
		if (x + width > screenWidth)
			x = x - width;
		if (y + height > screenHeight)
			y = y - height;
		
		FrameSlot.SetPos(m_wRoot, x, y);
	}
	
	PS_ContextAction AddAction(ResourceName icon, string quad, string name, string actionName, PS_ContextActionData contextActionData)
	{
		if (m_wNoActions)
			m_wNoActions.RemoveFromHierarchy();
		if (m_wActionsVerticalLayout.GetChildren().IsVisible())
		{
			GetGame().GetWorkspace().CreateWidgets(CONTEXT_SEPARATOR_PREFAB, m_wActionsVerticalLayout);
		}
		Widget contextActionWidget = GetGame().GetWorkspace().CreateWidgets(CONTEXT_ACTION_PREFAB, m_wActionsVerticalLayout);
		PS_ContextAction contextAction = PS_ContextAction.Cast(contextActionWidget.FindHandler(PS_ContextAction));
		contextAction.Init(icon, quad, name, actionName, contextActionData);
		height += 24;
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
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PS_CoopLobby coopLobby = PS_CoopLobby.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (!coopLobby)
			return;
		if (coopLobby.GetSelectedPlayer() == playerId)
			return;
		
		string name = "#PS-ContextAction_SelectPlayer";
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
		
		string name = "#PS-ContextAction_Kick";
		return AddAction(IMAGESET, "kickCommandAlt", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionKick);
	}
	void OnActionKick(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
		PS_PlayableManager.GetPlayableController().KickPlayer(contextActionDataPlayer.GetPlayerId());
	}
	
	void ActionGetArmaId(int playerId)
	{
		string name = "#PS-ContextAction_CopyArmaId";
		return AddAction(IMAGESET_PS, "Arma", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnActionCopyArmaId);
	}
	void OnActionCopyArmaId(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		string playerUUID = GetGame().GetBackendApi().GetPlayerUID(contextActionDataPlayer.GetPlayerId());
		System.ExportToClipboard(playerUUID);
	}
	
	void ActionMute(int playerId)
	{
		string name = "#PS-ContextAction_MutePlayer";
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
		string name = "#PS-ContextAction_UnmutePlayer";
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
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_UnpinPlayer";
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
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_PinPlayer";
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
		string name = "#PS-ContextAction_KickFromVoiceRoom";
		return AddAction(IMAGESET, "kickCommandAlt", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction();
	}
	
	void ActionDirectMessage(int playerId)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_DirectMessage";
		AddAction(IMAGESET, "whisper", name, "",
			new PS_ContextActionDataPlayer(playerId)
		).GetOnOnContextAction().Insert(OnContextDirectMessage);
	}
	void OnContextDirectMessage(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		int playerId = contextActionDataPlayer.GetPlayerId();
		MenuBase menuBase = GetGame().GetMenuManager().GetTopMenu();
		
		Widget chat = menuBase.GetRootWidget().FindAnyWidget("ChatPanel");
		if (!chat)
			chat = GetGame().GetWorkspace().FindAnyWidget("ChatPanel");
		if (!chat)
			return;
		
		SCR_ChatPanel chatPanel = SCR_ChatPanel.Cast(chat.FindHandler(SCR_ChatPanel));
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(chatPanel);
		EditBoxWidget editBoxWidget = chatPanel.PS_GetWidgets().m_MessageEditBox;
		editBoxWidget.SetText("/dmsg " + playerId.ToString() + " ");
	}
	
	// Actions playable
	PS_ScriptInvokerOnContextAction ActionFreeSlot(RplId playableId)
	{
		return AddAction(IMAGESET, "kickCommandAlt", "#PS-ContextAction_FreeSlot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionLock(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Locked", "#PS-ContextAction_LockSlot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionUnlock(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Unlocked", "#PS-ContextAction_UnlockSlot", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionOpenInventory(RplId playableId)
	{
		return AddAction(IMAGESET_PS, "Backpack", "#PS-ContextAction_OpenInventory", "",
			new PS_ContextActionDataPlayable(playableId)
		).GetOnOnContextAction();
	}
	void ActionRespawnInPlace(RplId playableId, int playerId)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_Respawn";
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
		string name = "#PS-ContextAction_AttachTo";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionDetachFrom(SCR_ChimeraCharacter character)
	{
		string name = "#PS-ContextAction_DetachFrom";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionLookAt(SCR_ChimeraCharacter character)
	{
		string name = "#PS-ContextAction_LookAt";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	PS_ScriptInvokerOnContextAction ActionFirstPersonView(SCR_ChimeraCharacter character)
	{
		string name = "#PS-ContextAction_FirstPersonView";
		return AddAction(IMAGESET, "battleye_enabled", name, "",
			new PS_ContextActionDataCharacter(character)
		).GetOnOnContextAction();
	}
	void ActionCreatePrefab(vector position)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_CreatePrefab";
		return AddAction(IMAGESET_PS, "Prefab", name, "",
			new PS_ContextActionDataPosition(position)
		).GetOnOnContextAction().Insert(OnCreatePrefab);
	}
	void OnCreatePrefab(PS_ContextAction contextAction, PS_ContextActionDataPosition contextActionDataPosition)
	{
		MenuBase menuBase = GetGame().GetMenuManager().GetTopMenu();
		
		Widget chat = menuBase.GetRootWidget().FindAnyWidget("ChatPanel");
		if (!chat)
			chat = GetGame().GetWorkspace().FindAnyWidget("ChatPanel");
		if (!chat)
			return;
		vector position = contextActionDataPosition.GetPosition();
		
		string clipBoard = System.ImportFromClipboard();
		string GUID = SCR_ConfigHelper.GetGUID(clipBoard, false);
		if (GUID != "")
		{
			PlayerController playerController = GetGame().GetPlayerController();
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
			if (!playableController)
				return;
			
			playableController.SpawnPrefab(GUID, position);
			return;
		}
		
		return;
		
		SCR_ChatPanel chatPanel = SCR_ChatPanel.Cast(chat.FindHandler(SCR_ChatPanel));
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(chatPanel);
		EditBoxWidget editBoxWidget = chatPanel.PS_GetWidgets().m_MessageEditBox;
		string positionStr = position.ToString();
		positionStr.Replace(" ", "|");
		
		editBoxWidget.SetText("/spp " + positionStr + " ");
	}
	void ActionCreateAdministrator(vector position)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		string name = "#PS-ContextAction_CreateAdministrator";
		return AddAction(IMAGESET_PS, "Prefab", name, "",
			new PS_ContextActionDataPosition(position)
		).GetOnOnContextAction().Insert(OnCreateAdministrator);
	}
	void OnCreateAdministrator(PS_ContextAction contextAction, PS_ContextActionDataPosition contextActionDataPosition)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		playableController.SpawnAdministrator(contextActionDataPosition.GetPosition());
	}
	
	// Actions voice
	PS_ScriptInvokerOnContextAction ActionJoinVoice()
	{
		return AddAction(IMAGESET_PS, "RoomEnter", "#PS-ContextAction_JoinVoiceRoom", "",
			new PS_ContextActionData()
		).GetOnOnContextAction();
	}
	
	// Util
	string GetPlayerName(int playerId)
	{
		return PS_PlayableManager.GetInstance().GetPlayerName(playerId);
	}
}