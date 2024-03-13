modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	SpectatorMenu
}

class PS_SpectatorMenu: MenuBase
{
	protected SCR_ChatPanel m_ChatPanel;
	
	InputManager m_InputManager;
	
	// Voice chat menu
	protected Widget m_wChat;
	protected Widget m_wVoiceChatList;
	protected Widget m_wOverlayFooter;
	protected Widget m_wEarlyAccessRoot;
	protected Widget m_wAlivePlayerList;
	protected Widget m_wSidesRatio;
	protected Widget m_wSidesRatioFrame;
	protected PS_VoiceChatList m_hVoiceChatList;
	protected SCR_ButtonBaseComponent m_hVoiceChatListPinButton;
	protected PS_AlivePlayerList m_hAlivePlayerList;
	protected SCR_ButtonBaseComponent m_hAlivePlayerListPinButton;
	
	SCR_InputButtonComponent m_bNavigationSwitchSpectatorUI;

	protected ResourceName m_rPlayableIconPath = "{EE28CD1CBCAED99D}UI/Spectator/SpectatorPlayableIcon.layout";
	protected FrameWidget m_wIconsFrame;
	
	protected FrameWidget m_wMapFrame;
	
	protected SCR_MapEntity m_MapEntity;
	
	protected ref map<PS_SpectatorLabel, PS_SpectatorLabel> m_mIconsList = new map<PS_SpectatorLabel, PS_SpectatorLabel>();
		
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu();
	}
	
	void SetCameraCharacter(IEntity characterEntity)
	{
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetCharacterEntity(characterEntity);
	}
	
	override void OnMenuOpen()
	{
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		m_hVoiceChatListPinButton = SCR_ButtonBaseComponent.Cast(m_wVoiceChatList.FindAnyWidget("PinButton").FindHandler(SCR_ButtonBaseComponent));
		
		m_wAlivePlayerList = GetRootWidget().FindAnyWidget("AlivePlayersList");
		m_hAlivePlayerList = PS_AlivePlayerList.Cast(m_wAlivePlayerList.FindHandler(PS_AlivePlayerList));
		m_hAlivePlayerListPinButton = SCR_ButtonBaseComponent.Cast(m_wAlivePlayerList.FindAnyWidget("PinButton").FindHandler(SCR_ButtonBaseComponent));
		
		m_hAlivePlayerList.SetSpectatorMenu(this);
		m_wOverlayFooter = GetRootWidget().FindAnyWidget("OverlayFooter");
		m_wEarlyAccessRoot = GetRootWidget().FindAnyWidget("EarlyAccessRoot");
		m_wIconsFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"));
		m_wMapFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("MapFrame"));
		m_wSidesRatioFrame = GetRootWidget().FindAnyWidget("SidesRatioFrame");
		m_wSidesRatio = GetRootWidget().FindAnyWidget("SidesRatio");
		
		m_bNavigationSwitchSpectatorUI = SCR_InputButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationSwitchSpectatorUI").FindHandler(SCR_InputButtonComponent));
		m_bNavigationSwitchSpectatorUI.m_OnClicked.Insert(Action_SwitchSpectatorUI);
		
		super.OnMenuOpen();
		InitChat();
		
		GetGame().GetCallqueue().CallLater(UpdateCycle, 0);
		GetGame().GetCallqueue().CallLater(RoomSwitchToGlobal, 0);
	}
	
	void RoomSwitchToGlobal()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.MoveToVoNRoom(playerController.GetPlayerId(), "", "#PS-VoNRoom_Global");
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hAlivePlayerList.HardUpdate();
	}
	
	void InitChat()
	{
		// Check if this menu has chat
		m_wChat = GetRootWidget().FindAnyWidget("ChatPanel");
		if (!m_wChat)
			return;
		
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChat.FindHandler(SCR_ChatPanel));
	}
	
	override void OnMenuInit()
	{
		super.OnMenuInit();
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			
		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			m_InputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			m_InputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, Action_SwitchSpectatorUI);
			m_InputManager.AddActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			m_InputManager.AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
#ifdef WORKBENCH
			m_InputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
		}
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			m_InputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			m_InputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			m_InputManager.RemoveActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.RemoveActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.RemoveActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, Action_SwitchSpectatorUI);
			m_InputManager.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			m_InputManager.RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
#ifdef WORKBENCH
			m_InputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
		}
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		if (m_MapEntity.IsOpen())
			m_InputManager.ActivateContext("MapContext");
		
		UpdateIcons();
		
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		while (cursorWidget)
		{
			
			if (cursorWidget == m_wAlivePlayerList)
			{
				break;
			}
			
			if (cursorWidget == m_wVoiceChatList)
			{
				break;
			}
			
			cursorWidget = cursorWidget.GetParent();
		}
		
		float alivePlayerListX = FrameSlot.GetPosX(m_wAlivePlayerList);
		float voiceChatListX = FrameSlot.GetPosX(m_wVoiceChatList);
		if (cursorWidget == m_wAlivePlayerList || m_hAlivePlayerListPinButton.IsToggled())
		{
			alivePlayerListX += tDelta * 1200.0;
			if (alivePlayerListX > 0)
				alivePlayerListX = 0;
		}
		else
		{
			alivePlayerListX -= tDelta * 1200.0;
			if (alivePlayerListX < -310)
				alivePlayerListX = -310;
		}
		FrameSlot.SetPosX(m_wAlivePlayerList, alivePlayerListX);
		
		if (cursorWidget == m_wVoiceChatList || m_hVoiceChatListPinButton.IsToggled())
		{
			voiceChatListX -= tDelta * 1200.0;
			if (voiceChatListX < -320)
				voiceChatListX = -320;
		}
		else
		{
			voiceChatListX += tDelta * 1200.0;
			if (voiceChatListX > -10)
				voiceChatListX = -10;
		}
		FrameSlot.SetPosX(m_wVoiceChatList, voiceChatListX);
		
		/* VoN Magic
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.VoNPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin() - vector.Up * 1.7;
		*/
		
		//GetGame().Getm_InputManager().ActivateContext("SpectatorContext", 1);
	}
	
	void UpdateIcons()
	{
		PS_SpectatorLabelsManager spectatorLabelsManager = PS_SpectatorLabelsManager.GetInstance();
		foreach (PS_SpectatorLabel spectatorLabel : spectatorLabelsManager.m_aSpectatorLabels)
		{
			if (!m_mIconsList.Contains(spectatorLabel))
			{
				spectatorLabel.CreateLabel(m_wIconsFrame);
				m_mIconsList.Insert(spectatorLabel, spectatorLabel);
			}
		}
		foreach (PS_SpectatorLabel spectatorLabel, PS_SpectatorLabel spectatorLabel2 : m_mIconsList)
		{
			if (!spectatorLabel)
				continue;
			spectatorLabel.UpdateLabel();
			spectatorLabel.UpdateMarker();
		}
	}
	
	void ChatToggle()
	{
		if (!m_ChatPanel)
			return;
		
		if (!m_ChatPanel.IsOpen())
			GetGame().GetCallqueue().CallLater(ChatWrap, 0);
	}
	void ChatWrap()
	{
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	void Action_LobbyVoNOn()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNRadioEnable();
	}
	
	void Action_LobbyVoNOff()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNDisable();
	}
	
	void Action_SwitchSpectatorUI()
	{
		if (m_wVoiceChatList.IsVisible())
		{
			m_wChat.SetVisible(false);
			m_wVoiceChatList.SetVisible(false);
			m_wOverlayFooter.SetVisible(false);
			m_wEarlyAccessRoot.SetVisible(false);
			m_wAlivePlayerList.SetVisible(false);
			m_wIconsFrame.SetVisible(false);
			m_wSidesRatioFrame.SetVisible(false);
		} else {
			m_wChat.SetVisible(true);
			m_wVoiceChatList.SetVisible(true);
			m_wOverlayFooter.SetVisible(true);
			m_wEarlyAccessRoot.SetVisible(true);
			m_wAlivePlayerList.SetVisible(true);
			m_wIconsFrame.SetVisible(true);
			m_wSidesRatioFrame.SetVisible(true);
		}
	}
	
	void Action_ToggleMap()
	{
		if (!m_MapEntity.IsOpen()) OpenMap();
		else CloseMap();
	}
	
	void Action_ManualCameraTeleport()
	{
		MoveCamera(GetCursorWorldPos());
	}
	
	void MoveCamera(vector worldPosition)
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
			if (teleportComponent)
			{
				teleportComponent.TeleportCamera(worldPosition, true, false);
			}
		}
	}
	
	vector GetCursorWorldPos()
	{
		ArmaReforgerScripted game = GetGame();
		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		
		vector worldPos = "0 0 0";
		
		// If map is open return map cursor world position
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			float worldX, worldY;
			mapEntity.GetMapCursorWorldPosition(worldX, worldY);
			worldPos[0] = worldX;
			worldPos[2] = worldY;
			worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
			
			return worldPos;
		}
		
		
		vector cursorPos = GetCursorPos();
		vector outDir;
		vector startPos = workspace.ProjScreenToWorld(cursorPos[0], cursorPos[1], outDir, world, -1);
		outDir *= 1000.0;
	
		autoptr TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = startPos + outDir;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN; 
		
		float traceDis = world.TraceMove(trace, null);
		worldPos = startPos + outDir * traceDis;
		
		return worldPos;
	}
	
	vector GetCursorPos()
	{
		int cursorX, cursorY;
		if (m_InputManager && m_InputManager.IsUsingMouseAndKeyboard())
		{
			WidgetManager.GetMousePos(cursorX, cursorY);
			return Vector(GetGame().GetWorkspace().DPIUnscale(cursorX), GetGame().GetWorkspace().DPIUnscale(cursorY), 0);
		} else {
			return Vector(GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetWidth()/2.0), GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetHeight()/2.0), 0);
		}
	}
	
	void OpenMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(false);
		
		m_wMapFrame.SetVisible(true);
		
		// WUT?
		SCR_WidgetHelper.RemoveAllChildren(GetRootWidget().FindAnyWidget("ToolMenuHoriz"));
		
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configComp.GetEditorMapConfig(), m_wMapFrame);
		m_MapEntity.OpenMap(mapConfigFullscreen);
	}
	
	void CloseMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(true);
		
		m_wMapFrame.SetVisible(false);
		
		m_MapEntity.CloseMap();
	}
}