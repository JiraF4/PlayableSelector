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
	protected SCR_MapEntity m_MapEntity;	
	protected SCR_ChatPanel m_ChatPanel;
	
	protected Widget m_wVoiceChatList;
	protected PS_VoiceChatList m_hVoiceChatList;
	
	protected Widget m_wGameModeHeader;
	protected PS_GameModeHeader m_hGameModeHeader;
	
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{	
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Callback_OnChatToggleAction);
		
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		
		m_wGameModeHeader = GetRootWidget().FindAnyWidget("GameModeHeader");
		m_hGameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		
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
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hVoiceChatList.HardUpdate();
		m_hGameModeHeader.Update();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_MapEntity)
			m_MapEntity.CloseMap();
		
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Callback_OnChatToggleAction);
	}
		
	//------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	//------------------------------------------------------------------------------------------------
	void Callback_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		GetGame().GetCallqueue().CallLater(OpenChat, 0);
	}
	
	void OpenChat()
	{
		if (!m_ChatPanel.IsOpen())
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
};