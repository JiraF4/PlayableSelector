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
	protected ref array<Widget> m_aRolesListWidgets = {};
	Widget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};

	override void OnMenuOpen()
	{
		GetGame().GetCallqueue().CallLater(Fill, 210); // TODO: Fix delay
	}
	
	void Fill()
	{
		m_wFactionList = GetRootWidget().FindAnyWidget("FactionList");
		m_wRolesList = GetRootWidget().FindAnyWidget("RolesList");
		m_wPlayersList = GetRootWidget().FindAnyWidget("PlayersList");
		
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
	}
	
	protected void FactionClick(SCR_ButtonBaseComponent factionSelector)
	{
		foreach (Widget widget: m_aFactionListWidgets)
		{
			if (widget != factionSelector)
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
		
		Widget RolesGroup = GetGame().GetWorkspace().CreateWidgets(m_sRolesGroupPrefab);
		SCR_RolesGroup handler = SCR_RolesGroup.Cast(RolesGroup.FindHandler(SCR_RolesGroup));
		m_aRolesListWidgets.Insert(RolesGroup);
		m_wRolesList.AddChild(RolesGroup);
		
		array<SCR_PlayableComponent> factionPlayablesList = m_sFactionPlayables[m_fCurrentFaction];
		foreach (SCR_PlayableComponent playable: factionPlayablesList)
		{
			handler.AddPlayable(playable);
		
			handler.SetName(playable.GetGroupName());
		}
	}
	
}





























