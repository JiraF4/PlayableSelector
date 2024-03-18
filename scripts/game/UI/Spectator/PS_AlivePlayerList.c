// Widget displays info about alive players in game.
// Path: {18D3CF175C9AA974}UI/Spectator/AlivePlayersList.layout

class PS_AlivePlayerList : ScriptedWidgetComponent
{
	// Const
	protected ResourceName m_sAliveGroupPrefab = "{71DA7743CE51DABF}UI/Spectator/AlivePlayerGroup.layout";
	protected ResourceName m_sAliveFactionButtonPrefab = "{4959360D4111DACC}UI/Spectator/AliveFactionButton.layout";
	
	// Cache global
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected WorkspaceWidget m_WorkspaceWidget;
	
	// Widgets
	protected VerticalLayoutWidget m_wPlayersList;
	protected HorizontalLayoutWidget m_wHorizontalLayoutFactions;
	protected ScrollLayoutWidget m_wAlivePlayersListScroll;
	
	// Parameters
	protected PS_SpectatorMenu m_mSpectatorMenu;
	
	// Vars
	protected ref map<SCR_AIGroup, PS_AlivePlayerGroup> m_aAlivePlayerGroups = new map<SCR_AIGroup, PS_AlivePlayerGroup>();
	protected ref map<Faction, PS_AliveFactionButton> m_aFactionButtons = new map<Faction, PS_AliveFactionButton>();
	protected ref array<Faction> m_aSelectedFactions = {};
	
	override void HandlerAttached(Widget w)
	{
		// Widgets
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("AlivePlayersList"));
		m_wHorizontalLayoutFactions = HorizontalLayoutWidget.Cast(w.FindAnyWidget("HorizontalLayoutFactions"));
		m_wAlivePlayersListScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("AlivePlayersListScroll"));
		
		// Cache global
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_WorkspaceWidget = GetGame().GetWorkspace();
	}
	
	void InitList()
	{
		array<PS_PlayableComponent> playables = m_PlayableManager.GetPlayablesSorted();
		map<SCR_Faction, int> factions = new map<SCR_Faction, int>();
		
		foreach (PS_PlayableComponent playable : playables)
		{
			AddPlayable(playable);
			
			FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
			SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
			if (!factions.Contains(faction))
				factions.Insert(faction, 1);
			else
				factions.Set(faction, factions.Get(faction) + 1);
		}
		
		foreach (SCR_Faction faction, int count : factions)
		{
			m_aSelectedFactions.Insert(faction);
			AddFactionButton(faction, count);
		}
		
		// Added in runtime
		m_PlayableManager.GetOnPlayableRegistered().Insert(OnPlayableRegistered);
	}
	
	void AddPlayable(PS_PlayableComponent playable)
	{
		SCR_AIGroup playableGroup = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetId());
		PS_AlivePlayerGroup alivePlayerGroup;
		if (!m_aAlivePlayerGroups.Contains(playableGroup))
		{
			Widget aliveGroupRoot = m_WorkspaceWidget.CreateWidgets(m_sAliveGroupPrefab, m_wPlayersList);
			alivePlayerGroup = PS_AlivePlayerGroup.Cast(aliveGroupRoot.FindHandler(PS_AlivePlayerGroup));
			alivePlayerGroup.SetAIGroup(playableGroup);	
			alivePlayerGroup.SetSpectatorMenu(m_mSpectatorMenu);
			alivePlayerGroup.SetAlivePlayerList(this);
			m_aAlivePlayerGroups.Insert(playableGroup, alivePlayerGroup);
		}
		else alivePlayerGroup = m_aAlivePlayerGroups.Get(playableGroup);
		alivePlayerGroup.InsertPlayable(playable);
	}
	
	void AddFactionButton(SCR_Faction faction, int count)
	{
		Widget aliveFactionRoot = m_WorkspaceWidget.CreateWidgets(m_sAliveFactionButtonPrefab, m_wHorizontalLayoutFactions);
		bool factionSelected = m_aSelectedFactions.Contains(faction);
		aliveFactionRoot.SetVisible(factionSelected);
		PS_AliveFactionButton aliveFactionButton = PS_AliveFactionButton.Cast(aliveFactionRoot.FindHandler(PS_AliveFactionButton));
		aliveFactionButton.SetFaction(faction);
		aliveFactionButton.SetCount(count);
		aliveFactionButton.m_OnClicked.Insert(FactionButtonClicked);
		m_aFactionButtons.Insert(faction, aliveFactionButton);
	}
	
	void SetSpectatorMenu(PS_SpectatorMenu spectatorMenu)
	{
		m_mSpectatorMenu = spectatorMenu;
		
		InitList();
	}
	
	void OnPlayableRegistered(RplId playableId, PS_PlayableComponent playable)
	{
		AddPlayable(playable);
		
		FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
		SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
		AddFactionCount(faction, 1);	
	}
	
	void AddFactionCount(SCR_Faction faction, int added)
	{
		if (!m_aFactionButtons.Contains(faction))
			AddFactionButton(faction, 0);
		PS_AliveFactionButton aliveFactionButton = m_aFactionButtons.Get(faction);
		int count = aliveFactionButton.GetCount();
		aliveFactionButton.SetCount(count + added);
		if (count <= 1)
		{
			aliveFactionButton.GetRootWidget().RemoveFromHierarchy();
			m_aFactionButtons.Remove(faction);
		}
	}
	
	void OnAliveRemoved(PS_PlayableComponent playableComponent)
	{
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
		AddFactionCount(faction, -1);
	}
	
	void OnAliveGroupRemoved(SCR_AIGroup group)
	{
		m_aAlivePlayerGroups.Remove(group);
	}
	
	// -------------------- Buttons events --------------------
	void FactionButtonClicked(SCR_ButtonBaseComponent playerButton)
	{
		m_wAlivePlayersListScroll.SetSliderPos(0, 0);
		
		PS_AliveFactionButton aliveFactionButton = PS_AliveFactionButton.Cast(playerButton);
		SCR_Faction faction = aliveFactionButton.GetFaction();
		
		if (m_aSelectedFactions.Contains(faction))
			m_aSelectedFactions.RemoveItem(faction);
		else
			m_aSelectedFactions.Insert(faction);
			
		foreach (SCR_AIGroup aiGroup, PS_AlivePlayerGroup alivePlayerGroup : m_aAlivePlayerGroups)
		{
			SCR_Faction goupFaction = SCR_Faction.Cast(aiGroup.GetFaction());
			bool factionSelected = m_aSelectedFactions.Contains(goupFaction);
			alivePlayerGroup.GetRootWidget().SetVisible(factionSelected);
		}
	}
}