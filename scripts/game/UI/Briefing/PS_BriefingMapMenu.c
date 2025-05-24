// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	BriefingMapMenu
}

//------------------------------------------------------------------------------------------------
//! Fullscreen map menu
class PS_BriefingMapMenu: ChimeraMenuBase
{	
	protected ResourceName m_rCurrentPlayableMapMarker = "{52CA8FF5F56C6F31}UI/Map/ManualMapMarkerBase.layout";
	PS_ManualMarkerComponent m_hPlayableMarkerComponent;
	protected float m_fPlayerMarkerSize = 15;
	
	protected SCR_MapEntity m_MapEntity;	
	protected SCR_ChatPanel m_ChatPanel;
	
	protected Widget m_wMissionDescription;
	protected PS_MissionDescriptionUI m_hMissionDescription;
	
	protected Widget m_wVoiceChatList;
	protected PS_VoiceChatList m_hVoiceChatList;
	
	protected Widget m_wGameModeHeader;
	protected PS_GameModeHeader m_hGameModeHeader;
	
	protected Widget m_wSteps;
	
	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{	
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		
		m_wGameModeHeader = GetRootWidget().FindAnyWidget("GameModeHeader");
		m_hGameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		
		m_wMissionDescription = GetRootWidget().FindAnyWidget("MissionDescriptionFrame");
		m_hMissionDescription = PS_MissionDescriptionUI.Cast(m_wGameModeHeader.FindHandler(PS_MissionDescriptionUI));
		
		m_wSteps = GetRootWidget().FindAnyWidget("StepsFrame");
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		//m_wSteps.SetVisible(gameMode.GetState() == SCR_EGameModeState.BRIEFING);
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("SwitchVoiceChat", EActionTrigger.DOWN, Action_SwitchVoiceChat);
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager.GetPlayableByPlayer(playerController.GetPlayerId()) != RplId.Invalid())
			GetRootWidget().FindAnyWidget("PlayableNotSelectedOverlay").SetVisible(false);
		
		GetGame().GetCallqueue().CallLater(UpdateCycle, 0);
	}
	void OpenMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrap, 0); // Need two frames
	}
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{4B34A766C096D12C}Configs/Map/MapLobby.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChange, 0);
	}
	void OpenMapWrapZoomChange()
	{
		// What i do with my life...
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChangeWrap, 0); // Another two frames
	}
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();
		
		/* Starting position marker on briefing
		// Create marker for playable on briefing
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.BRIEFING)
		{		
			Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
			Widget playableMarker = Widget.Cast(GetGame().GetWorkspace().CreateWidgets(m_rCurrentPlayableMapMarker, mapFrame));
			m_hPlayableMarkerComponent = PS_ManualMarkerComponent.Cast(playableMarker.FindHandler(PS_ManualMarkerComponent));
			m_hPlayableMarkerComponent.SetImage("{B9E14AFE75AEBB19}UI/Textures/Icons/icons_mapMarkersUI_drawing/icons_DR_mapMarkersUI.imageset", "AR_icon_spawn_point");
			m_hPlayableMarkerComponent.SetImageGlow("","");
			m_hPlayableMarkerComponent.SetColor(Color.ORANGE);
			m_hPlayableMarkerComponent.SetDescription("#PS-Briefing_YourPlace");
			m_hPlayableMarkerComponent.OnMouseLeave(null, null, 0, 0);
		}*/
	}
	
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		FactionKey factionKey = playableManager.GetPlayerFactionKey(currentPlayerId);
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
			
		if (m_hVoiceChatList.GetFactionKey() != factionKey)
			playableControllerComponent.SwitchToMenu(SCR_EGameModeState.BRIEFING);
		
		// Update playable marker
		/*
		if (m_hPlayableMarkerComponent)
		{
			
			// current player
			RplId currentPlayableId = playableManager.GetPlayableByPlayer(currentPlayerId);
			if (currentPlayableId == RplId.Invalid())
			{
				m_hPlayableMarkerComponent.SetColor(new Color(0, 0, 0, 0));
				return;
			}
			
			PS_PlayableContainer playableComponent = playableManager.GetPlayableById(currentPlayableId);
			IEntity entity = playableComponent.GetOwner();
			
			float wX, wY, screenX, screenY, screenXEnd, screenYEnd;
			vector worldPosition = entity.GetOrigin();
			wX = worldPosition[0];
			wY = worldPosition[2];
			m_MapEntity.WorldToScreen(wX, wY, screenX, screenY, true);
			m_MapEntity.WorldToScreen(wX + m_fPlayerMarkerSize, wY + m_fPlayerMarkerSize, screenXEnd, screenYEnd, true);
			float screenXD = GetGame().GetWorkspace().DPIUnscale(screenX);
			float screenYD = GetGame().GetWorkspace().DPIUnscale(screenY);
			float sizeXD = GetGame().GetWorkspace().DPIUnscale(screenXEnd - screenX);
			float sizeYD = GetGame().GetWorkspace().DPIUnscale(screenY - screenYEnd);
			sizeYD *= m_hPlayableMarkerComponent.GetYScale();
			m_hPlayableMarkerComponent.SetSlot(screenXD, screenYD, sizeXD, sizeYD, 0.0);
			m_hPlayableMarkerComponent.SetColor(new Color(1, 1, 1, 1));
		}
		*/
	}
	
	override void OnMenuClose()
	{
		if (m_MapEntity)
			m_MapEntity.CloseMap();
		
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("SwitchVoiceChat", EActionTrigger.DOWN, Action_SwitchVoiceChat);
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hGameModeHeader.Update();
	}
	
	// -------------------------------------- Actions --------------------------------------
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 0);
	}
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	// -------------------------------------- VoN --------------------------------------
	void Action_LobbyVoNOn()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNEnable();
	}
	void Action_LobbyVoNOff()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNDisable();
	}
	// Channel
	void Action_LobbyVoNChannelOn()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNFactionEnable();
	}

	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
	
	void Action_SwitchVoiceChat()
	{
		m_wVoiceChatList.SetVisible(!m_wVoiceChatList.IsVisible());
	}
};