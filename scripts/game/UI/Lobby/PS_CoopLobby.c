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
	protected ref map<FactionKey,ref array<PS_PlayableComponent>> m_sFactionPlayables = new map<FactionKey,ref array<PS_PlayableComponent>>();
		
	// Admins can move other players, save selectted player id
	protected int m_iCurrentPlayer = 0;
		
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
	SCR_NavigationButtonComponent m_bNavigationButtonForceStart;
	
	// Timer start time, for game launch delay and counter animation
	int m_iStartTime = 0;
	int m_iStartTimerSeconds = 3; // Launch delay in seconds, skip if game already started
	
	// playables count on revious update, for exclude redunant widget recreation
	int m_iOldPlayablesCount = 0;
	int m_iOldPlayersCount = 0;
	FactionKey m_sOldPlayerFactionKey = "";
	
	// Global options
	ButtonWidget m_wFactionLockButton;
	ImageWidget m_wFactionLockImage;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	float m_fRecconnectTime = -1.0;
	
	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController) { // Menu open faster than player creation
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
			playableController.SwitchFromObserver();
		}
		
		m_wFactionList = GetRootWidget().FindAnyWidget("FactionList");
		m_wRolesList = GetRootWidget().FindAnyWidget("RolesList");
		m_wPlayersList = GetRootWidget().FindAnyWidget("PlayersList");
		
		Widget widget = GetRootWidget().FindAnyWidget("MainLoadoutPreview");
		m_preview = PS_LobbyLoadoutPreview.Cast(widget.FindHandler(PS_LobbyLoadoutPreview));
		m_wCounterText = TextWidget.Cast(GetRootWidget().FindAnyWidget("TextCounter"));
		m_wCounterText.SetText("");
		
		m_wChatPanelWidget = GetRootWidget().FindAnyWidget("ChatPanel");
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChatPanelWidget.FindHandler(SCR_ChatPanel));
		
		m_bNavigationButtonReady = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationStart").FindHandler(SCR_NavigationButtonComponent));
		m_bNavigationButtonChat = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationChat").FindHandler(SCR_NavigationButtonComponent));
		m_bNavigationButtonClose = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationClose").FindHandler(SCR_NavigationButtonComponent));
		m_bNavigationButtonForceStart = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationForceStart").FindHandler(SCR_NavigationButtonComponent));
		
		m_bNavigationButtonReady.m_OnClicked.Insert(Action_Ready);
		m_bNavigationButtonChat.m_OnClicked.Insert(Action_ChatOpen);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_ChatOpen);
		m_bNavigationButtonClose.m_OnClicked.Insert(Action_Exit);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		m_bNavigationButtonForceStart.m_OnClicked.Insert(Action_ForceStart);
		GetGame().GetInputManager().AddActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
		
		// Faction lock
		m_wFactionLockButton = ButtonWidget.Cast(GetRootWidget().FindAnyWidget("FactionLockButton"));
		m_wFactionLockImage = ImageWidget.Cast(GetRootWidget().FindAnyWidget("FactionLockImage"));
		SCR_ButtonBaseComponent factionLockButtonHandler = SCR_ButtonBaseComponent.Cast(m_wFactionLockButton.FindHandler(SCR_ButtonBaseComponent));
		factionLockButtonHandler.m_OnClicked.Insert(Action_FactionLock);
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
				
		GetGame().GetCallqueue().CallLater(AwaitPLayerController, 100);
	}
	
	void AwaitPLayerController()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		if (playerController && playableManager.IsReplicated()) {
			if (playerController.GetPlayerId() != 0) {
				OnMenuOpenDelay();
				return;
			}
		}
		
		GetGame().GetCallqueue().CallLater(AwaitPLayerController, 100);
	}
	
	void OnMenuOpenDelay()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager.GetPlayableByPlayer(playerController.GetPlayerId()) != RplId.Invalid() && playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.Disconected)
			m_fRecconnectTime = GetGame().GetWorld().GetWorldTime() + 500;
		
		// Start update cycle
		UpdateCycle();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	};
	
	override void OnMenuClose()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableManager.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.Playing);
		
		GetGame().GetInputManager().RemoveActionListener("MenuSelect", EActionTrigger.DOWN, Action_Ready);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_ChatOpen);
		GetGame().GetInputManager().RemoveActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
	}
	
	// Soo many staff need to bee init, i can't track all of it...
	// Just update state every 100 ms
	void UpdateCycle() 
	{
		if (m_fRecconnectTime < GetGame().GetWorld().GetWorldTime()) Fill();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	// -------------------- Update content functions --------------------
	
	// "Hard" update, insert new widgets and call updateMenu
	void Fill()
	{		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		// await player initialization
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		if (m_iCurrentPlayer == 0) m_iCurrentPlayer = playerController.GetPlayerId();
		if (playableManager.GetPlayerState(m_iCurrentPlayer) == PS_EPlayableControllerState.Disconected) m_iCurrentPlayer = playerController.GetPlayerId();
		
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
		
		// Add playables if need
		for (int i = 0; i < playablesSorted.Count(); i++) {
			PS_PlayableComponent playable = playablesSorted[i];
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			FactionKey factionKey = faction.GetFactionKey();
			
			// insert faction if not currently exist
			if (!m_sFactionPlayables.Contains(factionKey)) {
				m_sFactionPlayables.Insert(factionKey, new array<PS_PlayableComponent>);
				
				Widget factionSelector = GetGame().GetWorkspace().CreateWidgets(m_sFactionSelectorPrefab);
				PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
				
				handler.SetFaction(faction);
				handler.m_OnClicked.Insert(FactionClick);
				
				m_aFactionListWidgets.Insert(factionSelector);
				m_wFactionList.AddChild(factionSelector);
			}
			
			// insert playable to faction
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[factionKey];
			if (!factionPlayablesList.Contains(playable))
				factionPlayablesList.Insert(playable);
		}
		
		// ReFill players if need
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		FactionKey curentFactionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
		
		if (m_iOldPlayersCount != playerIds.Count()) ReFillPlayersList();
		if (m_iOldPlayablesCount != playables.Count() || m_sOldPlayerFactionKey != curentFactionKey) ReFillCharacterList();
				
		m_iOldPlayersCount = playerIds.Count();
		m_iOldPlayablesCount = playables.Count();
		m_sOldPlayerFactionKey = curentFactionKey;
		
		UpdateMenu();
	}
	
	// "Soft" update, just change values do not create any new widgets
	void UpdateMenu()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		// Join button rename
		PlayerController thisPlayerController = GetGame().GetPlayerController();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_bNavigationButtonForceStart.SetVisible(playerRole == EPlayerRole.ADMINISTRATOR && gameMode.GetState() == SCR_EGameModeState.PREGAME, false);
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME) m_bNavigationButtonReady.SetLabel("#PS-Lobby_Ready");
		else {
			if (playableManager.GetPlayableByPlayer(thisPlayerController.GetPlayerId()) != RplId.Invalid()) m_bNavigationButtonReady.SetLabel("#PS-Lobby_Join");
			else m_bNavigationButtonReady.SetLabel("#PS-Lobby_JoinAsObserver");
		}
		
		// Faction lock state
		if (!gameMode.IsFactionLockMode()) m_wFactionLockImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
		else m_wFactionLockImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
		m_wFactionLockButton.SetVisible(playerRole == EPlayerRole.ADMINISTRATOR);
		
		// calculate players count for every faction
		foreach (Widget factionSelector: m_aFactionListWidgets)
		{
			PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
			
			int factionPLayersCount = 0;
			array<int> playerIds = new array<int>();
			GetGame().GetPlayerManager().GetPlayers(playerIds);
			foreach (int playerId: playerIds)
			{
				if (playableManager.GetPlayerFactionKey(playerId) == handler.GetFaction().GetFactionKey())
					factionPLayersCount = factionPLayersCount + 1;
			}
			
			if (playableManager.GetPlayerFactionKey(thisPlayerController.GetPlayerId()) == handler.GetFaction().GetFactionKey()) {
				if (!handler.IsToggled()) handler.SetToggled(true);
			} else if (handler.IsToggled()) handler.SetToggled(false);
			
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[handler.GetFaction().GetFactionKey()];
			int i = 0;
			foreach (PS_PlayableComponent playable : factionPlayablesList) {
				int playerId = playableManager.GetPlayerByPlayable(playable.GetId());
				if (playerId != -1)  i++;
			}
			handler.SetCount(factionPLayersCount, i, factionPlayablesList.Count());
		}
		
		// update every character widget
		foreach (Widget characterWidget : m_aCharactersListWidgets)
		{
			PS_CharacterSelector characterHandler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
			if (playableManager.GetPlayerByPlayable(characterHandler.GetPlayableId()) == m_iCurrentPlayer) {
				if (!characterHandler.IsToggled()) characterHandler.SetToggled(true);
			} else if (characterHandler.IsToggled()) characterHandler.SetToggled(false);
			characterHandler.UpdatePlayableInfo();
		}
		
		// update every player widget
		foreach (Widget playerWidget : m_aPlayersListWidgets)
		{
			PS_PlayerSelector playerHandler = PS_PlayerSelector.Cast(playerWidget.FindHandler(PS_PlayerSelector));
			if (playerHandler.GetPlayerId() == m_iCurrentPlayer) {
				if (!playerHandler.IsToggled()) playerHandler.SetToggled(true);
			} else if (playerHandler.IsToggled()) playerHandler.SetToggled(false);
			playerHandler.UpdatePlayerInfo();
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
		PlayerController playerController = GetGame().GetPlayerController();
		FactionKey currentFactionKey = "";
		if (playerController) currentFactionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
		if (!m_sFactionPlayables.Contains(currentFactionKey)) return;
		array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[currentFactionKey];
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
			handler.m_OnClicked.Insert(PlayerClick);
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
			playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.NotReady);
			return;
		}
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (playableManager.GetPlayableByPlayer(playerController.GetPlayerId()) != RplId.Invalid() || gameMode.GetState() == SCR_EGameModeState.GAME) 
		{
			playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.Ready);
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
	
	void Action_ForceStart()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.ForceGameStart();
	}
	
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
	
	void Action_FactionLock()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.FactionLockSwitch();
	}
	
	// -------------------- Widgets events --------------------
	// If faction clicked, switch focus from previous faction and update characters list to selected faction
	// ----- Faction -----
	protected void FactionClick(SCR_ButtonBaseComponent factionSelector)
	{
		PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.GetRootWidget().FindHandler(PS_FactionSelector));
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		FactionKey currentFactionKey = playableManager.GetPlayerFactionKey(currentPlayerController.GetPlayerId());
		if (gameMode.IsFactionLockMode() && currentPlayerRole != EPlayerRole.ADMINISTRATOR && currentFactionKey != "") return;	
		if (playableManager.GetPlayerPin(playerController.GetPlayerId()) && currentPlayerRole != EPlayerRole.ADMINISTRATOR) return;
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
		
		FactionKey factionKey = handler.GetFaction().GetFactionKey();
		if (playableManager.GetPlayerFactionKey(playerController.GetPlayerId()) == handler.GetFaction().GetFactionKey()) factionKey = "";
		
		// Change player faction
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.ChangeFactionKey(playerController.GetPlayerId(), factionKey);
		playableController.SetPlayerPlayable(playerController.GetPlayerId(), RplId.Invalid()); // Reset character
		playableController.MoveToVoNRoom(playerController.GetPlayerId(), factionKey, "");
	}
	
	// ----- Players -----
	protected void PlayerClick(SCR_ButtonBaseComponent playerSelector)
	{	
		// If not admin you can change only herself
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = GetGame().GetPlayerController();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
		
		PS_PlayerSelector handler = PS_PlayerSelector.Cast(playerSelector);
		m_iCurrentPlayer = handler.GetPlayerId();
		
		UpdateMenu();
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
		if (!handler) return;
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		int playableId = handler.GetPlayableId();
		m_preview.SetPlayable(playables[playableId]);
	}
	
	// Select playable from widget for player, or change player state if current playable already selected
	protected void CharacterClick(SCR_ButtonBaseComponent characterSelector)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_CharacterSelector handler = PS_CharacterSelector.Cast(characterSelector);
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR && playableManager.GetPlayerPin(m_iCurrentPlayer)) return;
		
		playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.NotReady);
		if ( handler.GetPlayableId() == playableManager.GetPlayableByPlayer(playerController.GetPlayerId())
		  && m_iCurrentPlayer == playerController.GetPlayerId()
		) {
			playableController.SetPlayerPlayable(playerController.GetPlayerId(), RplId.Invalid());
			playableController.MoveToVoNRoom(playerController.GetPlayerId(), playableManager.GetPlayerFactionKey(playerController.GetPlayerId()), "");
			return;
		}
		
		
		if (playableManager.GetPlayerByPlayable(handler.GetPlayableId()) != -1) return;
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
		
		// apply changes to SELECTED player
		
		PS_PlayableComponent playable = playableManager.GetPlayableById(handler.GetPlayableId());
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playable.GetOwner());
		playableController.ChangeFactionKey(m_iCurrentPlayer, playableCharacter.GetFaction().GetFactionKey());
		playableController.SetPlayerState(m_iCurrentPlayer, PS_EPlayableControllerState.NotReady);
		playableController.SetPlayerPlayable(m_iCurrentPlayer, handler.GetPlayableId());
		playableController.MoveToVoNRoom(playerController.GetPlayerId(), playableManager.GetPlayerFactionKey(playerController.GetPlayerId()), playableManager.GetGroupNameByPlayable(handler.GetPlayableId()));
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
		
		// If game already started, close lobby immediately
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (seconds >= m_iStartTimerSeconds || gameMode.GetState() == SCR_EGameModeState.GAME) {
			PlayerController playerController = GetGame().GetPlayerController();
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
			playableController.ApplyPlayable();
			m_wCounterText.SetText("");
			// Getting controll take some time, this prevent zero coord blinking. Not the best way though...
			GetGame().GetCallqueue().CallLater(Close, 100); 
			return;
		}
		else GetGame().GetCallqueue().CallLater(CloseTimer, 100);
		
		string secondsStr = (seconds + 1).ToString();
		if (m_wCounterText.GetText() != secondsStr) {
			m_wCounterText.SetText(secondsStr);
			AudioSystem.PlaySound("{3119327F3EFCA9C6}Sounds/UI/Samples/Gadgets/UI_Radio_Frequency_Cycle.wav");
		}
	}
			
	// check all player for ready state
	bool IsAllReady()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// PlayerManager sync take some time sooo check herself separeted before
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return false; // it may not exist befory synk completed
		if (playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.NotReady) return false;
		if (playableManager.GetPlayerState(playerController.GetPlayerId()) == PS_EPlayableControllerState.Playing) return false;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.GAME) return true;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		int allReady = 0;
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		if (playerIds.Count() == 0) return false;
		bool adminExist = false;
		foreach (int playerId: playerIds)
		{
			EPlayerRole playerRole = playerManager.GetPlayerRoles(playerId);
			if (playerRole == EPlayerRole.ADMINISTRATOR) adminExist = true;
			if (playableManager.GetPlayerState(playerId) != PS_EPlayableControllerState.NotReady) allReady++;
		}
		
		// Check for admin existance if need.
		if (!adminExist && gameMode.IsAdminMode()) return false;
		
		return allReady == playerIds.Count();
	}
	
	
}





























