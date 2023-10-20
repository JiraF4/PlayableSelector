// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CoopLobby
}

// Main lobby widget 
// Menu preset: ChimeraMenuPreset.CoopLobby
// Path: {9DECCA625D345B35}UI/Lobby/CoopLobby.layout
class PS_CoopLobby: MenuBase
{
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout"; // Handler: PS_RolesGroup
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout"; // Handler: PS_CharacterSelector
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout"; // Handler: PS_FactionSelector
	protected ResourceName m_sPlayerSelectorPrefab = "{B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout"; // Handler: PS_PlayerSelector
	
	/*
		Script generate widgets tree:
		Lobby 							- Our lobby widget
		├── FactionList 				- Contains all factions that have playable
		│   └── FactionWidget			- (m_sFactionSelectorPrefab) Set m_fCurrentFaction And update CharactersList
		├── CharactersList 				- Contains all playables from current selected faction (m_fCurrentFaction)
		│   └── GroupList 				- (m_sRolesGroupPrefab) Has no functional just group playable by group name
		│       └── CharacterWidget		- (m_sCharacterSelectorPrefab) Select playable for player
		└── PlayersList 				- Contains all CONNECTED players
		    └── PlayerWidget			- (m_sPlayerSelectorPrefab) Right now do nothing just display. TODO: kick, mute
	*/
	
	
	// faction sorted playables, for fast search, also they sorted by RplId for consistent order
	protected ref map<Faction,ref array<PS_PlayableComponent>> m_sFactionPlayables = new map<Faction,ref array<PS_PlayableComponent>>();
	
	// Current selected from widget faction
	protected Faction m_fCurrentFaction;
	
	// Factions collection widget
	Widget m_wFactionList;
	protected ref array<Widget> m_aFactionListWidgets = {};
	
	// Roles collection widget
	Widget m_wRolesList;
	protected ref array<Widget> m_aCharactersListWidgets = {};
	protected ref array<Widget> m_aRolesListWidgets = {};
	
	// Players collection widget
	Widget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	
	// Lobby widgets
	PS_LobbyLoadoutPreview m_preview; // Display selected playable info
	TextWidget m_wCounterText; // Counter for start timer animation
	protected Widget m_wChatPanelWidget; // Defaul SCR chat panel, initial chanel broken for some reason, but you still may select global chanel.
	protected SCR_ChatPanel m_ChatPanel;
	
	// Footer buttons
	SCR_NavigationButtonComponent m_bNavigationButtonReady;
	SCR_NavigationButtonComponent m_bNavigationButtonChat;
	SCR_NavigationButtonComponent m_bNavigationButtonClose;
	
	// Timer start time, for game launch delay and counter animation
	int m_iStartTime = 0;
	int m_iStartTimerSeconds = 3; // Launch delay in seconds, skip if game already started
	
	// playables count on revious update, for exclude redunant widget recreation
	int m_iOldPlayablesCount = 0;
	
	
	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{
		m_wFactionList = GetRootWidget().FindAnyWidget("FactionList");
		m_wRolesList = GetRootWidget().FindAnyWidget("RolesList");
		m_wPlayersList = GetRootWidget().FindAnyWidget("PlayersList");
		
		Widget widget = GetRootWidget().FindAnyWidget("VLWoadoutPreview");
		m_preview = PS_LobbyLoadoutPreview.Cast(widget.FindHandler(PS_LobbyLoadoutPreview));
		m_wCounterText = TextWidget.Cast(GetRootWidget().FindAnyWidget("TextCounter"));
		m_wCounterText.SetText("");
		
		m_wChatPanelWidget = GetRootWidget().FindAnyWidget("ChatPanel");
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChatPanelWidget.FindHandler(SCR_ChatPanel));
		
		m_bNavigationButtonReady = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationStart").FindHandler(SCR_NavigationButtonComponent));
		m_bNavigationButtonChat = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationChat").FindHandler(SCR_NavigationButtonComponent));
		m_bNavigationButtonClose = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationClose").FindHandler(SCR_NavigationButtonComponent));
		
		m_bNavigationButtonReady.m_OnClicked.Insert(Action_Ready);
		m_bNavigationButtonChat.m_OnClicked.Insert(Action_ChatOpen);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_ChatOpen);
		m_bNavigationButtonClose.m_OnClicked.Insert(Action_Exit);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		
		// Start update cycle
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
		GetGame().GetCallqueue().CallLater(Fill, 0);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		GetGame().GetInputManager().ActivateContext("LobbyContext", 1);
	};
	
	override void OnMenuClose()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableManager.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.Playing);
		
		GetGame().GetInputManager().RemoveActionListener("MenuSelect", EActionTrigger.DOWN, Action_Ready);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_ChatOpen);
	}
	
	// Soo many staff need to bee init, i can't track all of it...
	// Just update state every 100 ms
	void UpdateCycle() 
	{
		Fill();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	
	// -------------------- Update content functions --------------------
	
	// "Hard" update, insert new widgets and call updateMenu
	void Fill()
	{		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		
		array<PS_PlayableComponent> playablesSorted = new array<PS_PlayableComponent>;
		if (m_iOldPlayablesCount != playables.Count()) // exclude redundant updates, may case problems if one frame delete and insert entity
		{
			// Remove old widgets, this reset focus
			foreach (Widget widget: m_aFactionListWidgets)
			{
				m_wFactionList.RemoveChild(widget);
			}
			m_aFactionListWidgets.Clear();
			
			// sort playables by RplId
			for (int i = 0; i < playables.Count(); i++) {
				PS_PlayableComponent playable = playables.GetElement(i);
				bool isInserted = false;
				for (int s = 0; s < playablesSorted.Count(); s++) {
					PS_PlayableComponent playableS = playablesSorted[s];
					if (playableS.GetId() > playable.GetId()) {
						playablesSorted.InsertAt(playable, s);
						isInserted = true;
						break;
					}
				}
				if (!isInserted) {
					playablesSorted.Insert(playable);
				}
			}
			
			// clear faction playable map
			m_sFactionPlayables.Clear();
		}
		m_iOldPlayablesCount = playables.Count();
		
		// Add playables if need
		for (int i = 0; i < playablesSorted.Count(); i++) {
			PS_PlayableComponent playable = playablesSorted[i];
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			
			// insert faction if not currently exist
			if (!m_sFactionPlayables.Contains(faction)) {
				m_sFactionPlayables.Insert(faction, new array<PS_PlayableComponent>);
				
				Widget factionSelector = GetGame().GetWorkspace().CreateWidgets(m_sFactionSelectorPrefab);
				PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
				
				handler.SetFaction(faction);
				handler.m_OnClicked.Insert(FactionClick);
				
				m_aFactionListWidgets.Insert(factionSelector);
				m_wFactionList.AddChild(factionSelector);
			}
			
			// insert playable to faction
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[faction];
			if (!factionPlayablesList.Contains(playable))
				factionPlayablesList.Insert(playable);
		}
		
		// Update 
		ReFillPlayersList(); // ReFill players TODO: add soft update
		UpdateMenu();
	}
	
	// "Soft" update, just change values do not create any new widgets
	void UpdateMenu()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		// calculate players count for every faction
		foreach (Widget factionSelector: m_aFactionListWidgets)
		{
			PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[handler.GetFaction()];
			int i = 0;
			foreach (PS_PlayableComponent playable : factionPlayablesList) {
				int playerId = playableManager.GetPlayerByPlayable(playable.GetId());
				if (playerId != -1)  i++;
			}
			handler.SetCount(i, factionPlayablesList.Count());
		}
		
		// update every character widget
		foreach (Widget characterWidget : m_aCharactersListWidgets)
		{
			PS_CharacterSelector characterHandler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
			characterHandler.UpdatePlayableInfo();
		}
		
		m_preview.UpdatePreviewInfo();
		TryClose(); // Start close timer if all players ready
	}
	
	// Clear current characters widgets and insert new from current selectcted faction
	protected void ReFillCharacterList()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// Clear old widgets
		foreach (Widget widget: m_aRolesListWidgets)
		{
			m_wRolesList.RemoveChild(widget);
		}
		m_aRolesListWidgets.Clear();
		m_aCharactersListWidgets.Clear();
		
		// Add new widgets
		map<string, PS_RolesGroup> RolesGroups = new map<string, PS_RolesGroup>();
		array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[m_fCurrentFaction];
		foreach (PS_PlayableComponent playable: factionPlayablesList)
		{
			string groupName = playableManager.GetGroupNameByPlayable(playable.GetId());
			if (!RolesGroups.Contains(groupName))	{
				Widget RolesGroup = GetGame().GetWorkspace().CreateWidgets(m_sRolesGroupPrefab);
				PS_RolesGroup rolesGroupHandler = PS_RolesGroup.Cast(RolesGroup.FindHandler(PS_RolesGroup));
				m_aRolesListWidgets.Insert(RolesGroup);
				m_wRolesList.AddChild(RolesGroup);
				RolesGroups[groupName] = rolesGroupHandler;
				rolesGroupHandler.SetName(groupName);
			}
			PS_RolesGroup rolesGroupHandler = RolesGroups[groupName];
			
			Widget characterWidget = rolesGroupHandler.AddPlayable(playable);
			PS_CharacterSelector characterHandler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
			m_aCharactersListWidgets.Insert(characterWidget);
			characterHandler.m_OnClicked.Insert(CharacterClick);
			characterHandler.m_OnMouseEnter.Insert(CharacterMouseEnter);
			characterHandler.m_OnMouseLeave.Insert(CharacterMouseLeave);
			characterHandler.m_OnFocus.Insert(CharacterMouseEnter);
			characterHandler.m_OnFocusLost.Insert(CharacterMouseLeave);
		}
	}
	
	// Clear current players widgets and add all CONNECTED players
	protected void ReFillPlayersList()
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds); // Get CONNECTED players, not ALL.
		
		// Clear old widgets
		foreach (Widget widget: m_aPlayersListWidgets)
		{
			m_wPlayersList.RemoveChild(widget);
		}
		m_aPlayersListWidgets.Clear();
		
		// Add new widgets
		foreach (int playerId: playerIds)
		{
			Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerSelectorPrefab);
			PS_PlayerSelector handler = PS_PlayerSelector.Cast(playerSelector.FindHandler(PS_PlayerSelector));
			handler.SetPlayer(playerId);
			m_aPlayersListWidgets.Insert(playerSelector);
			m_wPlayersList.AddChild(playerSelector);
		}
	}
	
	
	// ----- Actions -----
	// Change player state
	void Action_Ready()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.Ready) {
			playableController.SetPlayerState(PS_EPlayableControllerState.NotReady);
			return;
		}
		if (playableManager.GetPlayableByPlayer(playerController.GetPlayerId()) != RplId.Invalid()) 
		{
			playableController.SetPlayerState(PS_EPlayableControllerState.Ready);
		}
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
		GameStateTransitions.RequestGameplayEndTransition();
		Close();
	}
	
	// -------------------- Widgets events --------------------
	// If faction clicked, switch focus from previous faction and update characters list to selected faction
	// ----- Faction -----
	protected void FactionClick(SCR_ButtonBaseComponent factionSelector)
	{
		if (!factionSelector.IsToggled())
		{
			factionSelector.SetToggled(true);			
		}
		
		Widget factionSelectorWidget = factionSelector.GetRootWidget();
		foreach (Widget widget: m_aFactionListWidgets)
		{
			if (widget != factionSelectorWidget)
			{
				PS_FactionSelector otherHandler = PS_FactionSelector.Cast(widget.FindHandler(PS_FactionSelector));
				otherHandler.SetToggled(false);
			}
		}
		
		PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.GetRootWidget().FindHandler(PS_FactionSelector));
		m_fCurrentFaction = handler.GetFaction();
		
		ReFillCharacterList();
	}
	

	// ----- Character -----
	// Reset preview character to selected playable or null
	protected void CharacterMouseLeave(Widget characterWidget)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		int playableId = playableManager.GetPlayableByPlayer(playerId);
		
		if (playableId == -1) {
			m_preview.SetPlayable(null);
		} else {
			map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
			m_preview.SetPlayable(playables[playableId]);
		}
	}
	
	// Set preview character to hovered playable
	protected void CharacterMouseEnter(Widget characterWidget)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_CharacterSelector handler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		int playableId = handler.GetPlayableId();
		m_preview.SetPlayable(playables[playableId]);
	}
	
	// Select playable from widget for player, or change player state if current playable already selected
	protected void CharacterClick(SCR_ButtonBaseComponent characterSelector)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!characterSelector.IsToggled())
		{
			characterSelector.SetToggled(true);			
		}
			
		
		Widget characterWidget = characterSelector.GetRootWidget();
		foreach (Widget widget: m_aCharactersListWidgets)
		{
			if (widget != characterWidget)
			{
				PS_CharacterSelector otherHandler = PS_CharacterSelector.Cast(widget.FindHandler(PS_CharacterSelector));
				otherHandler.SetToggled(false);
			}
		}
		
		PS_CharacterSelector handler = PS_CharacterSelector.Cast(characterSelector);
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		if (handler.GetPlayableId() == playableManager.GetPlayableByPlayer(playerController.GetPlayerId())
		&& playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.NotReady) {
			playableController.SetPlayerState(PS_EPlayableControllerState.Ready);
			return;
		}
		
		playableController.SetPlayerState(PS_EPlayableControllerState.NotReady);
		playableController.SetPlayerPlayable(handler.GetPlayableId());
	}	
	
	// -------------------- Extra lobby functions --------------------
	// Check players ready and start timer if not started
	void TryClose()
	{
		if (m_iStartTime != 0) return;
		if (!IsAllReady()) return;
		m_iStartTime = System.GetTickCount();
		CloseTimer();
	}
	
	// Godlike counter animation
	void CloseTimer()
	{
		// If someon not ready, stop timer
		if (!IsAllReady()) {
			m_wCounterText.SetText("");
			m_iStartTime = 0;
			return;
		}
		
		int currentTime = System.GetTickCount();
		int seconds = (currentTime - m_iStartTime)/1000;
		string secondsStr = (seconds + 1).ToString();
		if (m_wCounterText.GetText() != secondsStr) {
			m_wCounterText.SetText(secondsStr);
			AudioSystem.PlaySound("{3119327F3EFCA9C6}Sounds/UI/Samples/Gadgets/UI_Radio_Frequency_Cycle.wav");
		}
		
		// If game already started, close lobby immediately
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (seconds >= m_iStartTimerSeconds || gameMode.GetState() == SCR_EGameModeState.GAME) {
			PlayerController playerController = GetGame().GetPlayerController();
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
			playableController.ApplyPlayable();
			// Getting controll take some time, this prevent zero coord blinking. Not the best way though...
			GetGame().GetCallqueue().CallLater(Close, 100); 
			return;
		}
		else GetGame().GetCallqueue().CallLater(CloseTimer, 100);
	}
			
	// check all player for ready state
	bool IsAllReady()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// PlayerManager sync take some time sooo check herself separeted before
		PlayerController playerController = GetGame().GetPlayerController();
		if (playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.NotReady) return false;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		int allReady = 0;
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		if (playerIds.Count() == 0) return false;
		foreach (int playerId: playerIds)
		{
			if (playableManager.GetPlayerState(playerId) != PS_EPlayableControllerState.NotReady) allReady++;
		}
		
		return allReady == playerIds.Count();
	}
}





























