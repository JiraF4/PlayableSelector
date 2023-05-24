class SCR_CoopLobby: MenuBase
{
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout";
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout";
	protected ResourceName m_sPlayerSelectorPrefab = "{B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout";
	
	protected ref map<Faction,ref array<SCR_PlayableComponent>> m_sFactionPlayables = new map<Faction,ref array<SCR_PlayableComponent>>();
	protected Faction m_fCurrentFaction;
	
	Widget m_wFactionList;
	protected ref array<Widget> m_aFactionListWidgets = {};
	Widget m_wRolesList;
	protected ref array<Widget> m_aCharactersListWidgets = {};
	protected ref array<Widget> m_aRolesListWidgets = {};
	Widget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	SCR_LobbyLoadoutPreview m_preview;
	
	SCR_NavigationButtonComponent m_bNavigationButton;
	
	override void OnMenuOpen()
	{
		GetGame().GetInputManager().ActivateContext("MenuContext");
		GetGame().GetCallqueue().CallLater(Fill, 300); // TODO: Fix delay
		
		m_bNavigationButton = SCR_NavigationButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationStart").FindHandler(SCR_NavigationButtonComponent));
		
		m_bNavigationButton.m_OnClicked.Insert(Action_Ready);
		GetGame().GetInputManager().AddActionListener("MenuSelect", EActionTrigger.DOWN, Action_Ready);
	}
	
	void UpdateMenu()
	{
		foreach (Widget factionSelector: m_aFactionListWidgets)
		{
			SCR_FactionSelector handler = SCR_FactionSelector.Cast(factionSelector.FindHandler(SCR_FactionSelector));
		}
		foreach (Widget characterWidget: m_aCharactersListWidgets)
		{
			SCR_CharacterSelector characterHandler = SCR_CharacterSelector.Cast(characterWidget.FindHandler(SCR_CharacterSelector));
			characterHandler.UpdatePlayableInfo();
		}
		UpdatePlayersList();
		m_preview.UpdatePreviewInfo();
		TryClose();
	}
	
	void TryClose()
	{
		int allReady = 0;
		
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		foreach (int playerId: playerIds)
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
			SCR_PlayableControllerComponent playableController = SCR_PlayableControllerComponent.Cast(controller.FindComponent(SCR_PlayableControllerComponent));
			if (playableController.GetState(playerId) != PlayableControllerState.NotReady) allReady++;
		}
		//if (allReady == playerIds.Count()) Close();
	}
	
	
	override void OnMenuClose()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		SCR_PlayableControllerComponent playableController = SCR_PlayableControllerComponent.Cast(playerController.FindComponent(SCR_PlayableControllerComponent));
		playableController.SetState(PlayableControllerState.Playing);
	}
	
	void Fill()
	{
		m_wFactionList = GetRootWidget().FindAnyWidget("FactionList");
		m_wRolesList = GetRootWidget().FindAnyWidget("RolesList");
		m_wPlayersList = GetRootWidget().FindAnyWidget("PlayersList");
		Widget widget = GetRootWidget().FindAnyWidget("LoadoutPreviewTileController");
		m_preview = SCR_LobbyLoadoutPreview.Cast(widget.FindHandler(SCR_LobbyLoadoutPreview));
		
		map<int, SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		
		for (int i = 0; i < playables.Count(); i++) {
			SCR_PlayableComponent playable = playables.GetElement(i);
			if (playable == null) continue;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			if (!m_sFactionPlayables.Contains(faction)) 
				m_sFactionPlayables.Insert(faction, new array<SCR_PlayableComponent>);
			
			array<SCR_PlayableComponent> factionPlayablesList = m_sFactionPlayables[faction];
			factionPlayablesList.Insert(playable);
		}
		
		for (int i = 0; i < m_sFactionPlayables.Count(); i++) {
			Faction faction = m_sFactionPlayables.GetKey(i);
			array<SCR_PlayableComponent> factionPlayablesList = m_sFactionPlayables.GetElement(i);
			
			Widget factionSelector = GetGame().GetWorkspace().CreateWidgets(m_sFactionSelectorPrefab);
			SCR_FactionSelector handler = SCR_FactionSelector.Cast(factionSelector.FindHandler(SCR_FactionSelector));
			
			handler.SetFaction(faction);
			handler.SetCount(0, factionPlayablesList.Count());
			
			handler.m_OnClicked.Insert(FactionClick);
			
			m_aFactionListWidgets.Insert(factionSelector);
			m_wFactionList.AddChild(factionSelector);
		}
		
		UpdatePlayersList();
		m_preview.SetPlayable(null);
	}
	
	protected void FactionClick(SCR_ButtonBaseComponent factionSelector)
	{
		Widget factionSelectorWidget = factionSelector.GetRootWidget();
		foreach (Widget widget: m_aFactionListWidgets)
		{
			if (widget != factionSelectorWidget)
			{
				SCR_FactionSelector otherHandler = SCR_FactionSelector.Cast(widget.FindHandler(SCR_FactionSelector));
				otherHandler.SetToggled(false);
			}
		}
		
		SCR_FactionSelector handler = SCR_FactionSelector.Cast(factionSelector.GetRootWidget().FindHandler(SCR_FactionSelector));
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
		
		Widget RolesGroup = GetGame().GetWorkspace().CreateWidgets(m_sRolesGroupPrefab);
		SCR_RolesGroup handler = SCR_RolesGroup.Cast(RolesGroup.FindHandler(SCR_RolesGroup));
		m_aRolesListWidgets.Insert(RolesGroup);
		m_wRolesList.AddChild(RolesGroup);
		
		array<SCR_PlayableComponent> factionPlayablesList = m_sFactionPlayables[m_fCurrentFaction];
		foreach (SCR_PlayableComponent playable: factionPlayablesList)
		{
			Widget characterWidget = handler.AddPlayable(playable);
			SCR_CharacterSelector characterHandler = SCR_CharacterSelector.Cast(characterWidget.FindHandler(SCR_CharacterSelector));
			m_aCharactersListWidgets.Insert(characterWidget);
			characterHandler.m_OnClicked.Insert(CharacterClick);
			characterHandler.m_OnMouseEnter.Insert(CharacterMouseEnter);
			characterHandler.m_OnMouseLeave.Insert(CharacterMouseLeave);
			characterHandler.m_OnFocus.Insert(CharacterMouseEnter);
			characterHandler.m_OnFocusLost.Insert(CharacterMouseLeave);
			
			handler.SetName(playable.GetGroupName());
		}
	}
	
	protected void CharacterMouseLeave(Widget characterWidget)
	{
		m_preview.SetPlayable(null);
	}
	
	protected void CharacterMouseEnter(Widget characterWidget)
	{
		SCR_CharacterSelector handler = SCR_CharacterSelector.Cast(characterWidget.FindHandler(SCR_CharacterSelector));
		map<int, SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		int playableId = handler.GetPlayableId();
		m_preview.SetPlayable(playables[playableId]);
	}
	
	protected void CharacterClick(SCR_ButtonBaseComponent characterSelector)
	{
		
		Widget characterWidget = characterSelector.GetRootWidget();
		foreach (Widget widget: m_aCharactersListWidgets)
		{
			if (widget != characterWidget)
			{
				SCR_CharacterSelector otherHandler = SCR_CharacterSelector.Cast(widget.FindHandler(SCR_CharacterSelector));
				otherHandler.SetToggled(false);
			}
		}
		
		SCR_CharacterSelector handler = SCR_CharacterSelector.Cast(characterSelector);
		PlayerController playerController = GetGame().GetPlayerController();
		SCR_PlayableControllerComponent playableController = SCR_PlayableControllerComponent.Cast(playerController.FindComponent(SCR_PlayableControllerComponent));
		
		playableController.SetState(PlayableControllerState.NotReady);
		playableController.TakePossession(playerController.GetPlayerId(), handler.GetPlayableId());
	}
	
	protected void UpdatePlayersList()
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		
		foreach (Widget widget: m_aPlayersListWidgets)
		{
			m_wPlayersList.RemoveChild(widget);
		}
		m_aPlayersListWidgets.Clear();
		
		foreach (int playerId: playerIds)
		{
			Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerSelectorPrefab);
			SCR_PlayerSelector handler = SCR_PlayerSelector.Cast(playerSelector.FindHandler(SCR_PlayerSelector));
			handler.SetPlayer(playerId);
			m_aPlayersListWidgets.Insert(playerSelector);
			m_wPlayersList.AddChild(playerSelector);
		}
	}
	
	void Action_Ready()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		SCR_PlayableControllerComponent playableController = SCR_PlayableControllerComponent.Cast(playerController.FindComponent(SCR_PlayableControllerComponent));
		if (playableController.GetState(playerController.GetPlayerId()) == PlayableControllerState.Ready) {
			playableController.SetState(PlayableControllerState.NotReady);
		}
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playerController.GetControlledEntity());
		if (character != null) 
		{
			playableController.SetState(PlayableControllerState.Ready);
		}
	}
	
	
}





























