modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	SpectatorMenu
}

class PS_SpectatorMenu: MenuBase
{
	protected SCR_ChatPanel m_ChatPanel;
	
	// Voice chat menu
	protected Widget m_wChat;
	protected Widget m_wVoiceChatList;
	protected Widget m_wOverlayFooter;
	protected Widget m_wEarlyAccessRoot;
	protected Widget m_wAlivePlayerList;
	protected PS_VoiceChatList m_hVoiceChatList;
	protected PS_AlivePlayerList m_hAlivePlayerList;
	
	SCR_InputButtonComponent m_bNavigationSwitchSpectatorUI;

	protected ResourceName m_rPlayableIconPath = "{EE28CD1CBCAED99D}UI/Spectator/SpectatorPlayableIcon.layout";
	protected FrameWidget m_wIconsFrame;
	
	protected ref map<RplId, PS_SpectatorPlayableIcon> m_mIconsList = new map<RplId, PS_SpectatorPlayableIcon>();
	
	
	
	
	//string IconsFrame = "IconsFrame";
	
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
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		m_wAlivePlayerList = GetRootWidget().FindAnyWidget("AlivePlayersList");
		m_hAlivePlayerList = PS_AlivePlayerList.Cast(m_wAlivePlayerList.FindHandler(PS_AlivePlayerList));
		m_wOverlayFooter = GetRootWidget().FindAnyWidget("OverlayFooter");
		m_wEarlyAccessRoot = GetRootWidget().FindAnyWidget("EarlyAccessRoot");
		m_wIconsFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"));
		
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
		m_hVoiceChatList.HardUpdate();
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
		
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			inputManager.AddActionListener("SwitchSpectatorUI", EActionTrigger.UP, Action_SwitchSpectatorUI);
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
			inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			inputManager.RemoveActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
			inputManager.RemoveActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
			inputManager.RemoveActionListener("SwitchSpectatorUI", EActionTrigger.UP, Action_SwitchSpectatorUI);
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
		
		UpdateIcons();
		//GetGame().GetInputManager().ActivateContext("SpectatorContext", 1);
	}
	
	void UpdateIcons()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance(); 
		foreach (RplId id, PS_PlayableComponent playableComponent : playableManager.m_aPlayables)
		{
			if (!m_mIconsList.Contains(id))
			{
				Widget playableMarker = GetGame().GetWorkspace().CreateWidgets(m_rPlayableIconPath, m_wIconsFrame);
				PS_SpectatorPlayableIcon spectatorPlayableIcon = PS_SpectatorPlayableIcon.Cast(playableMarker.FindHandler(PS_SpectatorPlayableIcon));
				
				spectatorPlayableIcon.SetCharacter(playableComponent);
				
				m_mIconsList[id] = spectatorPlayableIcon;
			}
		}
		foreach (RplId id, PS_SpectatorPlayableIcon spectatorPlayableIcon : m_mIconsList)
		{
			if (playableManager.m_aPlayables.Contains(id))
			{
				spectatorPlayableIcon.Update();
			}
			else {
				m_wIconsFrame.RemoveChild(m_mIconsList[id].GetRootWidget());
				m_mIconsList.Remove(id);
			}
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
		} else {
			m_wChat.SetVisible(true);
			m_wVoiceChatList.SetVisible(true);
			m_wOverlayFooter.SetVisible(true);
			m_wEarlyAccessRoot.SetVisible(true);
			m_wAlivePlayerList.SetVisible(true);
			m_wIconsFrame.SetVisible(true);
		}
	}
}