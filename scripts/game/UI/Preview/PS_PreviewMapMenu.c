// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	PreviewMapMenu
}

//------------------------------------------------------------------------------------------------
//! Fullscreen map menu
class PS_PreviewMapMenu: ChimeraMenuBase
{	
	protected SCR_MapEntity m_MapEntity;	
	protected SCR_ChatPanel m_ChatPanel;
	
	protected Widget m_wVoiceChatList;
	protected PS_VoiceChatList m_hVoiceChatList;
	
	protected Widget m_wGameModeHeader;
	protected PS_GameModeHeader m_hGameModeHeader;
	
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
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		
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
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configComp.GetGadgetMapConfig(), GetRootWidget());
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
		
	}
	
	override void OnMenuClose()
	{
		if (m_MapEntity)
			m_MapEntity.CloseMap();
		
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
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
		playableController.LobbyVoNRadioEnable();
	}
	void Action_LobbyVoNChannelOff()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNDisable();
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
	
};