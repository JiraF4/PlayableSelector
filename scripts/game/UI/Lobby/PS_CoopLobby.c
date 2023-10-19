class PS_CoopLobby: MenuBase
{
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout";
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout";
	protected ResourceName m_sPlayerSelectorPrefab = "{B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout";
	
	protected ref map<Faction,ref array<PS_PlayableComponent>> m_sFactionPlayables = new map<Faction,ref array<PS_PlayableComponent>>();
	protected Faction m_fCurrentFaction;
	
	Widget m_wFactionList;
	protected ref array<Widget> m_aFactionListWidgets = {};
	Widget m_wRolesList;
	protected ref array<Widget> m_aCharactersListWidgets = {};
	protected ref array<Widget> m_aRolesListWidgets = {};
	Widget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	PS_LobbyLoadoutPreview m_preview;
	TextWidget m_wCounterText;
	protected Widget m_wChatPanelWidget;
	protected SCR_ChatPanel m_ChatPanel;
	
	SCR_NavigationButtonComponent m_bNavigationButtonReady;
	SCR_NavigationButtonComponent m_bNavigationButtonChat;
	SCR_NavigationButtonComponent m_bNavigationButtonClose;
	
	int startTime = 0;
	int oldPlayablesCount = 0;
	
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
		
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
		GetGame().GetCallqueue().CallLater(Fill, 0); // TODO: Fix delay
	}
	
	void UpdateCycle() // Soo many staff need to bee init, i can't track all of it...
	{
		Fill();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void UpdateMenu()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PossessingManagerComponent possessingManagerComponent = SCR_PossessingManagerComponent.GetInstance();
		foreach (Widget factionSelector: m_aFactionListWidgets)
		{
			PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[handler.GetFaction()];
			int i = 0;
			foreach (PS_PlayableComponent playable : factionPlayablesList) {
				int disconnectedPlayerId = PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetPlayablePlayer(playable.GetId());
				if (disconnectedPlayerId != -1)  i++;
			}
			handler.SetCount(i, factionPlayablesList.Count());
		}
		foreach (Widget characterWidget : m_aCharactersListWidgets)
		{
			PS_CharacterSelector characterHandler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
			characterHandler.UpdatePlayableInfo();
		}
		UpdatePlayersList();
		m_preview.UpdatePreviewInfo();
		TryClose();
	}
	
	void TryClose()
	{
		if (startTime != 0) return;
		if (!IsAllReady()) return;
		startTime = System.GetTickCount();
		CloseTimer();
	}
	
	void CloseTimer()
	{
		if (!IsAllReady()) {
			m_wCounterText.SetText("");
			startTime = 0;
			return;
		}
		
		int currentTime = System.GetTickCount();
		int seconds = (currentTime - startTime)/1000;
		string secondsStr = (seconds + 1).ToString();
		if (m_wCounterText.GetText() != secondsStr) {
			m_wCounterText.SetText(secondsStr);
			AudioSystem.PlaySound("{3119327F3EFCA9C6}Sounds/UI/Samples/Gadgets/UI_Radio_Frequency_Cycle.wav");
		}
		if (seconds == 3) Close();
		else GetGame().GetCallqueue().CallLater(CloseTimer, 100);
	}
			
	
	bool IsAllReady()
	{
		int allReady = 0;
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		if (playerIds.Count() == 0) return false;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		foreach (int playerId: playerIds)
		{
			if (PS_GameModeCoop.GetPlayerState(playerId) != PlayableControllerState.NotReady) allReady++;
		}
		
		return allReady == playerIds.Count();
	}
	
	
	override void OnMenuClose()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SetState(PlayableControllerState.Playing);
		
		GetGame().GetInputManager().RemoveActionListener("MenuSelect", EActionTrigger.DOWN, Action_Ready);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_ChatOpen);
	}
	
	void Fill()
	{
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		array<PS_PlayableComponent> playablesSorted = new array<PS_PlayableComponent>;
		if (oldPlayablesCount != playables.Count())
		{
			foreach (Widget widget: m_aFactionListWidgets)
			{
				m_wFactionList.RemoveChild(widget);
			}
			m_aFactionListWidgets.Clear();
			
			m_sFactionPlayables.Clear();
			for (int i = 0; i < playables.Count(); i++) {
				PS_PlayableComponent playable = playables.GetElement(i);
				bool inserted = false;
				for (int s = 0; s < playablesSorted.Count(); s++) {
					PS_PlayableComponent playableS = playablesSorted[s];
					if (playableS.GetId() > playable.GetId()) {
						playablesSorted.InsertAt(playable, s);
						inserted = true;
						break;
					}
				}
				if (!inserted) {
					playablesSorted.Insert(playable);
				}
			}
		}
		oldPlayablesCount = playables.Count();
		
		for (int i = 0; i < playablesSorted.Count(); i++) {
			PS_PlayableComponent playable = playablesSorted[i];
			if (playable == null) continue;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			if (!m_sFactionPlayables.Contains(faction)) {
				m_sFactionPlayables.Insert(faction, new array<PS_PlayableComponent>);
				
				Widget factionSelector = GetGame().GetWorkspace().CreateWidgets(m_sFactionSelectorPrefab);
				PS_FactionSelector handler = PS_FactionSelector.Cast(factionSelector.FindHandler(PS_FactionSelector));
				
				handler.SetFaction(faction);
				handler.m_OnClicked.Insert(FactionClick);
				
				m_aFactionListWidgets.Insert(factionSelector);
				m_wFactionList.AddChild(factionSelector);
			}
			
			array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[faction];
			if (!factionPlayablesList.Contains(playable))
				factionPlayablesList.Insert(playable);
		}
		
		UpdatePlayersList();
		UpdateMenu();
	}
	
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
		
		UpdateCharacterList();
	}
	
	protected void UpdateCharacterList()
	{
		foreach (Widget widget: m_aRolesListWidgets)
		{
			m_wRolesList.RemoveChild(widget);
		}
		m_aRolesListWidgets.Clear();
		m_aCharactersListWidgets.Clear();
		
		
		map<string, PS_RolesGroup> RolesGroups = new map<string, PS_RolesGroup>();
		array<PS_PlayableComponent> factionPlayablesList = m_sFactionPlayables[m_fCurrentFaction];
		foreach (PS_PlayableComponent playable: factionPlayablesList)
		{
			string groupName = playable.GetGroupName();
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
	protected void CharacterMouseLeave(Widget characterWidget)
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		int playableId = PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetPlayerPlayable(playerId);
		
		if (playableId == -1) {
			m_preview.SetPlayable(null);
		} else {
			map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
			m_preview.SetPlayable(playables[playableId]);
		}
	}
	
	protected void CharacterMouseEnter(Widget characterWidget)
	{
		PS_CharacterSelector handler = PS_CharacterSelector.Cast(characterWidget.FindHandler(PS_CharacterSelector));
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		int playableId = handler.GetPlayableId();
		m_preview.SetPlayable(playables[playableId]);
	}
	
	protected void CharacterClick(SCR_ButtonBaseComponent characterSelector)
	{
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
		
		if (handler.GetPlayableId() == PS_GameModeCoop.GetPlayerPlayable(playerController.GetPlayerId())
		&& playableController.GetState(playerController.GetPlayerId()) == PlayableControllerState.NotReady) {
			playableController.SetState(PlayableControllerState.Ready);
			return;
		}
		
		playableController.SetState(PlayableControllerState.NotReady);
		playableController.TakePossession(playerController.GetPlayerId(), handler.GetPlayableId());
	}
	
	protected void UpdatePlayersList()
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (Widget widget: m_aPlayersListWidgets)
		{
			m_wPlayersList.RemoveChild(widget);
		}
		m_aPlayersListWidgets.Clear();
		
		foreach (int playerId: playerIds)
		{
			Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerSelectorPrefab);
			PS_PlayerSelector handler = PS_PlayerSelector.Cast(playerSelector.FindHandler(PS_PlayerSelector));
			handler.SetPlayer(playerId);
			m_aPlayersListWidgets.Insert(playerSelector);
			m_wPlayersList.AddChild(playerSelector);
		}
	}
	
	void Action_Ready()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (playableController.GetState(playerController.GetPlayerId()) == PlayableControllerState.Ready) {
			playableController.SetState(PlayableControllerState.NotReady);
			return;
		}
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playerController.GetControlledEntity());
		if (character != null) 
		{
			playableController.SetState(PlayableControllerState.Ready);
		}
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		GetGame().GetInputManager().ActivateContext("LobbyContext", 1);
	};
	
	
	void Action_ChatOpen()
	{
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
}





























