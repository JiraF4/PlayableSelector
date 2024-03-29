// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CoopLobby
}

/*
	Script generate widgets tree:
	Lobby 							- Our lobby widget
	├── FactionList 				- Contains all factions that have playable
	│ └── FactionWidget			- (m_sFactionSelectorPrefab) Set m_fCurrentFaction And update CharactersList
	├── CharactersList 				- Contains all playables from current selected faction (m_fCurrentFaction)
	│ └── GroupList 				- (m_sRolesGroupPrefab) Has no functional just group playable by group name
	│ └── CharacterWidget		- (m_sCharacterSelectorPrefab) Select playable for player
	└── PlayersList 				- Contains all CONNECTED players
		└── PlayerWidget			- (m_sPlayerSelectorPrefab) Kick or mute players.
*/

// Main lobby widget
// Menu preset: ChimeraMenuPreset.CoopLobby
// Path: {9DECCA625D345B35}UI/Lobby/CoopLobby.layout
class PS_CoopLobby : MenuBase
{
	// Const
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout"; // Handler: PS_RolesGroup
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout"; // Handler: PS_CharacterSelector
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout"; // Handler: PS_FactionSelector
	
	// Cache global
	protected InputManager m_InputManager;
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerManager m_PlayerManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iPlayerId;

	// Widgets
	protected Widget m_wRoot;
	protected FrameWidget m_wGameModeHeader;
	protected OverlayWidget m_wChatPanel;
	protected VerticalLayoutWidget m_wFactionList;
	protected VerticalLayoutWidget m_wRolesList;
	protected ScrollLayoutWidget m_wPlayersScroll;
	protected VerticalLayoutWidget m_wPlayersList;
	protected ButtonWidget m_wPreviewHideButton;
	protected TextWidget m_wPlayersCounter;
	protected FrameWidget m_wVoiceChatFrame;
	protected OverlayWidget m_wLobbyLittleInventoryItemInfo;
	protected ButtonWidget m_wPlayersSwitch;
	protected ButtonWidget m_wVoiceSwitch;
	protected FrameWidget m_wMainLoadoutPreview;
	protected OverlayWidget m_wLoadoutPreviewBody;
	protected OverlayWidget m_wPlayersBody;
	protected ItemPreviewWidget m_wPreview;
	protected ButtonWidget m_wNavigationStart;
	protected ButtonWidget m_wNavigationChat;
	protected ButtonWidget m_wNavigationClose;
	protected ScrollLayoutWidget m_wRolesScroll;
	protected OverlayWidget m_wOverlayCounter;
	protected TextWidget m_wTextCounter;
	
	// Handlers
	protected SCR_ButtonBaseComponent m_PlayersSwitchButtonComponent;
	protected SCR_ButtonBaseComponent m_VoiceSwitchButtonComponent;
	protected SCR_ButtonBaseComponent m_PreviewHideButtonComponent;
	protected PS_GameModeHeader m_GameModeHeader;
	protected SCR_ChatPanel m_ChatPanel;
	protected PS_LobbyLoadoutPreview m_LobbyLoadoutPreview;
	protected PS_PlayersList m_PlayersList;
	protected SCR_InputButtonComponent m_NavigationStart;
	protected SCR_InputButtonComponent m_NavigationChat;
	protected SCR_InputButtonComponent m_NavigationClose;
	protected PS_VoiceChatList m_VoiceChatList;
	
	// Vars
	protected ref map<SCR_Faction, PS_FactionSelector> m_mFactions = new map<SCR_Faction, PS_FactionSelector>();
	protected ref map<SCR_AIGroup, PS_RolesGroup> m_mGroups = new map<SCR_AIGroup, PS_RolesGroup>();
	protected SCR_Faction m_CurrentFaction = null;
	protected int m_iSelectedPlayer;
	
	// Footer buttons
	SCR_InputButtonComponent m_bNavigationButtonReady;
	
	override void OnMenuOpen()
	{
		if (!PS_WaitScreen.m_bWaitEnded) {
			Close();
			return;
		}
			
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		// Cache global
		m_InputManager = GetGame().GetInputManager();
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_iPlayerId = m_PlayerController.GetPlayerId();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		m_iSelectedPlayer = m_iPlayerId;
		
		// Widgets
		m_wRoot = GetRootWidget();
		m_wGameModeHeader = FrameWidget.Cast(m_wRoot.FindAnyWidget("GameModeHeader"));
		m_wChatPanel = OverlayWidget.Cast(m_wRoot.FindAnyWidget("ChatPanel"));
		m_wFactionList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("FactionList"));
		m_wRolesList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesList"));
		m_wPlayersScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("PlayersScroll"));
		m_wPreviewHideButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewHideButton"));
		m_wPlayersCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersCounter"));
		m_wVoiceChatFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("VoiceChatFrame"));
		m_wLobbyLittleInventoryItemInfo = OverlayWidget.Cast(m_wRoot.FindAnyWidget("LobbyLittleInventoryItemInfo"));
		m_wPlayersSwitch = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PlayersSwitch"));
		m_wVoiceSwitch = ButtonWidget.Cast(m_wRoot.FindAnyWidget("VoiceSwitch"));
		m_wMainLoadoutPreview = FrameWidget.Cast(m_wRoot.FindAnyWidget("MainLoadoutPreview"));
		m_wLoadoutPreviewBody = OverlayWidget.Cast(m_wRoot.FindAnyWidget("LoadoutPreviewBody"));
		m_wPlayersBody = OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayersBody"));
		m_wPreview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("Preview"));
		m_wNavigationStart = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationStart"));
		m_wRolesScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesScroll"));
		m_wNavigationChat = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationChat"));
		m_wNavigationClose = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationClose"));
		m_wOverlayCounter = OverlayWidget.Cast(m_wRoot.FindAnyWidget("OverlayCounter"));
		m_wTextCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("TextCounter"));
		
		// Handlers
		m_GameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChatPanel.FindHandler(SCR_ChatPanel));
		m_PreviewHideButtonComponent = SCR_ButtonBaseComponent.Cast(m_wPreviewHideButton.FindHandler(SCR_ButtonBaseComponent));
		m_PlayersSwitchButtonComponent = SCR_ButtonBaseComponent.Cast(m_wPlayersSwitch.FindHandler(SCR_ButtonBaseComponent));
		m_VoiceSwitchButtonComponent = SCR_ButtonBaseComponent.Cast(m_wVoiceSwitch.FindHandler(SCR_ButtonBaseComponent));
		m_NavigationChat = SCR_InputButtonComponent.Cast(m_wNavigationChat.FindHandler(SCR_InputButtonComponent));
		m_NavigationClose = SCR_InputButtonComponent.Cast(m_wNavigationClose.FindHandler(SCR_InputButtonComponent));
		m_LobbyLoadoutPreview = PS_LobbyLoadoutPreview.Cast(m_wMainLoadoutPreview.FindHandler(PS_LobbyLoadoutPreview));
		m_PlayersList = PS_PlayersList.Cast(m_wPlayersBody.FindHandler(PS_PlayersList));
		m_NavigationStart = SCR_InputButtonComponent.Cast(m_wNavigationStart.FindHandler(SCR_InputButtonComponent));
		m_VoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatFrame.FindHandler(PS_VoiceChatList));
		
		FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(m_iPlayerId);
		m_CurrentFaction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));
		m_VoiceChatList.SwitchFaction(factionKey);
		
		// Buttons
		m_PreviewHideButtonComponent.m_OnClicked.Insert(OnClickedPreviewHide);
		m_PlayersSwitchButtonComponent.m_OnClicked.Insert(OnClickedPlayersSwitch);
		m_VoiceSwitchButtonComponent.m_OnClicked.Insert(OnClickedVoiceSwitch);
		m_NavigationStart.m_OnActivated.Insert(Action_Ready);
		m_NavigationChat.m_OnActivated.Insert(Action_ChatOpen);
		m_NavigationClose.m_OnActivated.Insert(Action_Exit);
		
		// Events
		m_PlayableManager.GetOnFactionChange().Insert(UpdatePlayerFaction);
		m_PlayableManager.GetOnStartTimerCounterChanged().Insert(OnStartTimerCounterChanged);
		m_PlayableManager.GetOnPlayerConnected().Insert(OnPlayerConnected);
		m_PlayableManager.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		
		// Actions
		if (m_GameModeCoop.GetState() == SCR_EGameModeState.SLOTSELECTION)
		{
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
			m_InputManager.AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		}
		
		m_LobbyLoadoutPreview.SetItemInfoWidget(m_wLobbyLittleInventoryItemInfo);
		
		// Init
		Init();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		m_ChatPanel.OnUpdateChat(tDelta);
		
		// Force refreash
		if (m_wLoadoutPreviewBody.IsVisible())
			m_wPreview.SetRefresh(1, 1);
		
		m_GameModeHeader.TryUpdate();
	};
	
	override void OnMenuClose()
	{
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
			m_InputManager.RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		}
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnFactionChange().Remove(UpdatePlayerFaction);
			m_PlayableManager.GetOnStartTimerCounterChanged().Remove(OnStartTimerCounterChanged);
			m_PlayableManager.GetOnPlayerConnected().Remove(OnPlayerConnected);
			m_PlayableManager.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	void Init()
	{
		InitPlayables();
		InitPlayers();
		
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}
	
	void InitPlayables()
	{
		array<PS_PlayableComponent> playables = m_PlayableManager.GetPlayablesSorted();
		map<SCR_Faction, ref Tuple2<int, int>> factions = new map<SCR_Faction, ref Tuple2<int, int>>();
		
		foreach (PS_PlayableComponent playable : playables)
		{
			AddPlayable(playable);
			
			int playerId = m_PlayableManager.GetPlayerByPlayable(playable.GetId());
			int playerAdded = 0;
			if (playerId >= 0)
				playerAdded = 1;
			
			FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
			SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
			if (!factions.Contains(faction))
				factions.Insert(faction, new Tuple2<int, int>(playerAdded, 1));
			else
			{
				Tuple2<int, int> tuple = factions.Get(faction);
				tuple.param1 += playerAdded;
				tuple.param2 += 1;
			}
		}
		
		foreach (SCR_Faction faction, Tuple2<int, int> count : factions)
		{
			AddFaction(faction, count.param1, count.param2);
		}
		
		// Added in runtime
		m_PlayableManager.GetOnPlayableRegistered().Insert(OnPlayableRegistered);
	}
	
	void InitPlayers()
	{
		m_PlayersList.SetLobbyMenu(this);
		m_PlayersList.InitPlayers();
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Add
	void AddFaction(SCR_Faction faction, int count, int maxCount)
	{
		Widget factionSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sFactionSelectorPrefab, m_wFactionList);
		PS_FactionSelector factionSelector = PS_FactionSelector.Cast(factionSelectorRoot.FindHandler(PS_FactionSelector));
		factionSelector.SetFaction(faction);
		factionSelector.SetCount(count);
		factionSelector.SetMaxCount(maxCount);
		factionSelector.SetCoopLobby(this);
		factionSelector.SetToggled(m_CurrentFaction == faction);
		m_mFactions.Insert(faction, factionSelector);
	}
	
	void AddPlayable(PS_PlayableComponent playable)
	{
		SCR_AIGroup playableGroup = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetId());
		PS_RolesGroup rolesGroup;
		if (!m_mGroups.Contains(playableGroup))
		{
			Widget rolesGroupRoot = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, m_wRolesList);
			rolesGroup = PS_RolesGroup.Cast(rolesGroupRoot.FindHandler(PS_RolesGroup));
			rolesGroup.SetLobbyMenu(this);
			rolesGroup.SetAIGroup(playableGroup);
			m_mGroups.Insert(playableGroup, rolesGroup);
		
			FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
			SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
			rolesGroupRoot.SetVisible(m_CurrentFaction == faction);
		}
		else rolesGroup = m_mGroups.Get(playableGroup);
		rolesGroup.InsertPlayable(playable);
	}
	
	void AddPlayer(int playerId)
	{
		
	}
	
	void AddFactionCount(SCR_Faction faction, int added, int addedMax, int addedLocked = 0)
	{
		if (!m_mFactions.Contains(faction))
			AddFaction(faction, 0, 0);
		PS_FactionSelector factionSelector = m_mFactions.Get(faction);
		int count = factionSelector.GetCount();
		int maxCount = factionSelector.GetMaxCount();
		int lockedCount = factionSelector.GetLockedCount();
		factionSelector.SetCount(count + added);
		factionSelector.SetMaxCount(maxCount + addedMax);
		factionSelector.SetLockedCount(lockedCount + addedLocked);
		if ((maxCount + addedMax) < 1)
		{
			factionSelector.GetRootWidget().RemoveFromHierarchy();
			m_mFactions.Remove(faction);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClickedPreviewHide(SCR_ButtonBaseComponent factionButton)
	{
		m_wLoadoutPreviewBody.SetVisible(!m_wLoadoutPreviewBody.IsVisible());
	}
	
	void OnClickedPlayersSwitch(SCR_ButtonBaseComponent factionButton)
	{
		m_wVoiceChatFrame.SetVisible(false);
		m_wPlayersScroll.SetVisible(true);
		m_VoiceSwitchButtonComponent.SetToggled(false);
	}
	
	void OnClickedVoiceSwitch(SCR_ButtonBaseComponent factionButton)
	{
		m_wVoiceChatFrame.SetVisible(true);
		m_wPlayersScroll.SetVisible(false);
		m_PlayersSwitchButtonComponent.SetToggled(false);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Events
	void OnPlayableRegistered(RplId playableId, PS_PlayableComponent playable)
	{
		AddPlayable(playable);
	}
	
	void OnRolesGroupRemoved(PS_RolesGroup rolesGroup)
	{
		
	}
	
	void OnPlayableRemoved(PS_PlayableComponent playable)
	{
		FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
		SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
		
		int playerId = m_PlayableManager.GetPlayerByPlayable(playable.GetId());
		int playerAdded = 0;
		int lockedAdded = 0;
		if (playerId >= 0)
			playerAdded = -1;
		if (playerId == -2)
			lockedAdded = -1;
		
		AddFactionCount(faction, playerAdded, -1, lockedAdded);
	}
	
	void OnPlayerRemoved(int playerId)
	{
		if (playerId != m_iSelectedPlayer)
			return;
		
		m_iSelectedPlayer = m_iPlayerId;
		m_PlayersList.SetSelectedPlayer(m_iPlayerId);
	}
	
	void SwitchCurrentFaction(SCR_Faction faction)
	{
		if (m_CurrentFaction != faction)
		{
			if (m_CurrentFaction)
				m_mFactions.Get(m_CurrentFaction).SetToggled(false);
			m_CurrentFaction = faction;
		}
		else
			m_CurrentFaction = null;
		
		foreach (SCR_AIGroup aiGroup, PS_RolesGroup rolesGroup : m_mGroups)
		{
			SCR_Faction groupFaction = SCR_Faction.Cast(aiGroup.GetFaction());
			bool factionSelected = m_CurrentFaction == groupFaction;
			rolesGroup.GetRootWidget().SetVisible(factionSelected);
		}
		
		m_wRolesScroll.SetSliderPos(0, 0);
	}
	
	void SetPreviewPlayable(PS_PlayableComponent playable)
	{
		if (!playable)
		{
			RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iPlayerId);
			playable = m_PlayableManager.GetPlayableById(playableId);
		}
		m_LobbyLoadoutPreview.SetPlayable(playable);
	}
	
	void UpdatePlayerFaction(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		if (playerId != m_iPlayerId)
			return;
		
		SwitchVoiceChatFaction(factionKey);
	}
	
	void SwitchVoiceChatFaction(FactionKey factionKey)
	{
		m_VoiceChatList.SwitchFaction(factionKey);
	}
	
	
	void OnPlayerConnected(int playerId)
	{
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}
	
	void OnPlayerDisconnected(int playerId)
	{
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}
	
	void OnStartTimerCounterChanged(int timer)
	{
		if (timer == -1)
		{
			m_wOverlayCounter.SetVisible(false);
		} else {
			m_wOverlayCounter.SetVisible(true);
			m_wTextCounter.SetText(timer.ToString());
			AudioSystem.PlaySound("{3119327F3EFCA9C6}Sounds/UI/Samples/Gadgets/UI_Radio_Frequency_Cycle.wav");
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetSelectedPlayer(int playerId)
	{
		m_iSelectedPlayer = playerId;
		m_PlayersList.SetSelectedPlayer(playerId);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Get
	int GetSelectedPlayer()
	{
		return m_iSelectedPlayer;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Actions
	void Action_Ready()
	{
		PS_EPlayableControllerState currentState = m_PlayableManager.GetPlayerState(m_iPlayerId);
		RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iPlayerId);
		if (currentState != PS_EPlayableControllerState.Ready && playableId != RplId.Invalid()) m_PlayableControllerComponent.SetPlayerState(m_iPlayerId, PS_EPlayableControllerState.Ready);
		if (currentState == PS_EPlayableControllerState.Ready) m_PlayableControllerComponent.SetPlayerState(m_iPlayerId, PS_EPlayableControllerState.NotReady);
	}
	
	// Direct
	void Action_LobbyVoNOn()
	{
		m_PlayableControllerComponent.LobbyVoNEnable();
	}
	void Action_LobbyVoNOff()
	{
		m_PlayableControllerComponent.LobbyVoNDisable();
	}
	// Channel
	void Action_LobbyVoNChannelOn()
	{
		m_PlayableControllerComponent.LobbyVoNRadioEnable();
	}
	void Action_LobbyVoNChannelOff()
	{
		m_PlayableControllerComponent.LobbyVoNDisable();
	}
	
	void Action_ChatOpen()
	{
		// Delay is esential, if any character already controlled
		GetGame().GetCallqueue().CallLater(ChatWrap, 0);
	}
	void ChatWrap()
	{
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameMode.GetState();
		if (gameModeState == SCR_EGameModeState.GAME)
		{
			Close();
			return;
		}
		
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}





























