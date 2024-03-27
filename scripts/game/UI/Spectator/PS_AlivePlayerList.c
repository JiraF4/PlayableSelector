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
	protected ButtonWidget m_wShowDeathButton;
	
	// Handlers
	protected SCR_ButtonBaseComponent m_hShowDeathButton;
	
	// Parameters
	protected PS_SpectatorMenu m_mSpectatorMenu;
	
	// Vars
	protected ref map<SCR_AIGroup, PS_AlivePlayerGroup> m_aAlivePlayerGroups = new map<SCR_AIGroup, PS_AlivePlayerGroup>();
	protected ref map<Faction, PS_AliveFactionButton> m_aFactionButtons = new map<Faction, PS_AliveFactionButton>();
	protected ref array<Faction> m_aSelectedFactions = {};
	
	ref ScriptInvokerBool m_OnShowDead = new ScriptInvokerBool();
	ScriptInvokerBool GetOnShowDead()
		return m_OnShowDead;
	
	override void HandlerAttached(Widget w)
	{
		// Widgets
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("AlivePlayersList"));
		m_wHorizontalLayoutFactions = HorizontalLayoutWidget.Cast(w.FindAnyWidget("HorizontalLayoutFactions"));
		m_wAlivePlayersListScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("AlivePlayersListScroll"));
		m_wShowDeathButton = ButtonWidget.Cast(w.FindAnyWidget("ShowDeathButton"));
		
		// Handlers
		m_hShowDeathButton = SCR_ButtonBaseComponent.Cast(m_wShowDeathButton.FindHandler(SCR_ButtonBaseComponent));
		
		// Cache global
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_WorkspaceWidget = GetGame().GetWorkspace();
		
		// Buttons
		m_hShowDeathButton.m_OnClicked.Insert(ShowDeadButtonClicked);
	}
	
	void InitList()
	{
		array<PS_PlayableComponent> playables = m_PlayableManager.GetPlayablesSorted();
		map<SCR_Faction, ref Tuple2<int, int>> factions = new map<SCR_Faction, ref Tuple2<int, int>>();
		
		foreach (PS_PlayableComponent playable : playables)
		{
			AddPlayable(playable);
			
			FactionAffiliationComponent factionAffiliationComponent = playable.GetFactionAffiliationComponent();
			SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
			int alive = 0;
			SCR_CharacterDamageManagerComponent characterDamageManagerComponent = playable.GetCharacterDamageManagerComponent();
			if (characterDamageManagerComponent.GetState() != EDamageState.DESTROYED)
				alive = 1;
			if (!factions.Contains(faction))
			{
				factions.Insert(faction, new Tuple2<int, int>(1, alive));
			}
			else
			{
				Tuple2<int, int> factionCount = factions.Get(faction);
				factionCount.param1++;
				factionCount.param2 += alive;
			}
		}
		
		foreach (SCR_Faction faction, Tuple2<int, int> factionCount : factions)
		{
			m_aSelectedFactions.Insert(faction);
			AddFactionButton(faction, factionCount.param1, factionCount.param2);
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
	
	void AddFactionButton(SCR_Faction faction, int count, int countAlive)
	{
		Widget aliveFactionRoot = m_WorkspaceWidget.CreateWidgets(m_sAliveFactionButtonPrefab, m_wHorizontalLayoutFactions);
		bool factionSelected = m_aSelectedFactions.Contains(faction);
		aliveFactionRoot.SetVisible(factionSelected);
		PS_AliveFactionButton aliveFactionButton = PS_AliveFactionButton.Cast(aliveFactionRoot.FindHandler(PS_AliveFactionButton));
		aliveFactionButton.SetFaction(faction);
		aliveFactionButton.SetCount(count);
		aliveFactionButton.SetCountAlive(countAlive);
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
		SCR_CharacterDamageManagerComponent characterDamageManagerComponent = playable.GetCharacterDamageManagerComponent();
		int addAlive = 0;
		if (characterDamageManagerComponent.GetState() != EDamageState.DESTROYED)
			addAlive = 1;
		AddFactionCount(faction, 1, addAlive);	
	}
	
	void AddFactionCount(SCR_Faction faction, int added, int addedAlive)
	{
		if (!m_aFactionButtons.Contains(faction))
			AddFactionButton(faction, 0, 0);
		PS_AliveFactionButton aliveFactionButton = m_aFactionButtons.Get(faction);
		int count = aliveFactionButton.GetCount();
		int countAlive = aliveFactionButton.GetCountAlive();
		aliveFactionButton.SetCount(count + added);
		aliveFactionButton.SetCountAlive(countAlive + addedAlive);
		if ((count + added) == 0)
		{
			aliveFactionButton.GetRootWidget().RemoveFromHierarchy();
			m_aFactionButtons.Remove(faction);
		}
	}
	
	void OnAliveDie(PS_PlayableComponent playableComponent)
	{
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
		AddFactionCount(faction, 0, -1);
	}
	
	void OnAliveRemoved(PS_PlayableComponent playableComponent)
	{
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		SCR_Faction faction = SCR_Faction.Cast(factionAffiliationComponent.GetDefaultAffiliatedFaction());
		SCR_CharacterDamageManagerComponent characterDamageManagerComponent = playableComponent.GetCharacterDamageManagerComponent();
		int removeAlive = 0;
		if (characterDamageManagerComponent.GetState() == EDamageState.DESTROYED)
			removeAlive = -1;
		AddFactionCount(faction, -1, removeAlive);
	}
	
	void OnAliveGroupRemoved(SCR_AIGroup group)
	{
		m_aAlivePlayerGroups.Remove(group);
	}
	
	// ETC
	bool IsShowDead()
		return m_hShowDeathButton.IsToggled();
	
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
	
	void ShowDeadButtonClicked(SCR_ButtonBaseComponent deadButton)
	{
		m_OnShowDead.Invoke(m_hShowDeathButton.IsToggled());
	}
}