modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	SpectatorMenu
}

class PS_SpectatorMenu: MenuBase
{
	protected SCR_ChatPanel m_ChatPanel;
	
	// Voice chat menu
	protected Widget m_wVoiceChatList;
	protected PS_VoiceChatList m_hVoiceChatList;
	protected Widget m_wAlivePlayerList;
	protected PS_AlivePlayerList m_hAlivePlayerList;
	
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu();
	}
	
	override void OnMenuOpen()
	{
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		m_wAlivePlayerList = GetRootWidget().FindAnyWidget("AlivePlayersList");
		m_hAlivePlayerList = PS_AlivePlayerList.Cast(m_wAlivePlayerList.FindHandler(PS_AlivePlayerList));
		
		super.OnMenuOpen();
		InitChat();
		
		GetGame().GetCallqueue().CallLater(UpdateCycle, 0);
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hAlivePlayerList.HardUpdate();
		m_hVoiceChatList.HardUpdate();
	}
	
	void InitChat()
	{
		// Check if this menu has chat
		Widget chatPanelWidget = GetRootWidget().FindAnyWidget("ChatPanel");
		if (!chatPanelWidget)
			return;
		
		m_ChatPanel = SCR_ChatPanel.Cast(chatPanelWidget.FindHandler(SCR_ChatPanel));
	}
	
	override void OnMenuInit()
	{
		super.OnMenuInit();
		
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			inputManager.AddActionListener("InstantVote", EActionTrigger.DOWN, GetGame().OnInstantVote);
			inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
#ifdef WORKBENCH
			inputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
		}
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			inputManager.RemoveActionListener("InstantVote", EActionTrigger.DOWN, GetGame().OnInstantVote);
			inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			inputManager.RemoveActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
			inputManager.RemoveActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
#ifdef WORKBENCH
			inputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
		}
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		//GetGame().GetInputManager().ActivateContext("SpectatorContext", 1);
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
}