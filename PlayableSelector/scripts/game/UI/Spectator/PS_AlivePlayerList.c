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
	protected ref map<SCR_Faction, PS_AliveFactionButton> m_aFactionButtons = new map<SCR_Faction, PS_AliveFactionButton>();
	protected ref array<SCR_Faction> m_aSelectedFactions = {};
	
	ref ScriptInvokerBool m_OnShowDead = new ScriptInvokerBool();
	ScriptInvokerBool GetOnShowDead()
	{
		return m_OnShowDead;
	}
	
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
		// InitList runs on EVERY spectator-menu open (OnMenuOpen -> SetSpectatorMenu). Reset prior state first,
		// or a re-open duplicates rows/buttons AND stacks another OnPlayableRegistered subscription - which is
		// exactly what broke the alive-count: a stale VISIBLE faction button left at 0 alongside a duplicate
		// (hidden) button that held the correct number, plus N recomputes per event from N stacked subscriptions.
		m_PlayableManager.GetOnPlayableRegistered().Remove(OnPlayableRegistered);
		foreach (SCR_Faction discardFaction, PS_AliveFactionButton oldButton : m_aFactionButtons)
		{
			if (oldButton)
				oldButton.GetRootWidget().RemoveFromHierarchy();
		}
		m_aFactionButtons.Clear();
		foreach (SCR_AIGroup discardGroup, PS_AlivePlayerGroup oldGroup : m_aAlivePlayerGroups)
		{
			if (oldGroup)
				oldGroup.GetRootWidget().RemoveFromHierarchy();
		}
		m_aAlivePlayerGroups.Clear();
		m_aSelectedFactions.Clear();

		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();

		// Order by faction (alphabetical) then group, so the list reads Faction A + its squads, then
		// Faction B + its squads (instead of raw registration order). Build a sortable key per playable
		// (factionKey | group name | stable index) and add them in sorted order, so both the group widgets
		// AND the faction filter buttons end up created in that order.
		array<string> sortKeys = {};
		map<string, PS_PlayableContainer> byKey = new map<string, PS_PlayableContainer>();
		for (int i = 0; i < playables.Count(); i++)
		{
			PS_PlayableContainer playable = playables[i];
			SCR_AIGroup group = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetRplId());
			string groupName = "";
			if (group)
				groupName = PS_GroupHelper.GetGroupFullName(group);
			string idx = i.ToString();
			while (idx.Length() < 4)
				idx = "0" + idx;
			string key = playable.GetFactionKey() + "|" + groupName + "|" + idx;
			sortKeys.Insert(key);
			byKey.Insert(key, playable);
		}
		sortKeys.Sort();

		foreach (string key : sortKeys)
		{
			PS_PlayableContainer playable = byKey.Get(key);
			AddPlayable(playable);

			SCR_Faction faction = playable.GetFaction();
			if (faction && !m_aSelectedFactions.Contains(faction))
			{
				m_aSelectedFactions.Insert(faction);
				AddFactionButton(faction, 0, 0); // created in sorted order; counts filled by RecomputeFactionCounts
			}
		}

		RecomputeFactionCounts();

		// Added in runtime
		m_PlayableManager.GetOnPlayableRegistered().Insert(OnPlayableRegistered);
	}

	// Recount total + alive playables per faction from the CURRENT replicated damage states and push the
	// values to the faction buttons. Order-independent and idempotent - replaces the old incremental +/-
	// counter (AddFactionCount), which drifted: it decremented for deaths fired during init (UpdateDammage
	// runs inside AddPlayable, before the buttons exist) and never restored the count on respawn. The data
	// it reads already replicates, so this needs no server round-trip.
	void RecomputeFactionCounts()
	{
		map<SCR_Faction, ref Tuple2<int, int>> tally = new map<SCR_Faction, ref Tuple2<int, int>>();
		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			SCR_Faction faction = playable.GetFaction();
			if (!faction)
				continue;
			int aliveAdd = 0;
			if (playable.GetDamageState() != EDamageState.DESTROYED)
				aliveAdd = 1;
			Tuple2<int, int> t;
			if (!tally.Find(faction, t))
				tally.Insert(faction, new Tuple2<int, int>(1, aliveAdd));
			else
			{
				t.param1 = t.param1 + 1;
				t.param2 = t.param2 + aliveAdd;
			}
		}

		// Update existing buttons; create any missing (a faction registered after init appends).
		foreach (SCR_Faction faction, Tuple2<int, int> t : tally)
		{
			bool buttonExisted = m_aFactionButtons.Contains(faction);
			// TEMP DIAGNOSTIC (alive=0 while count computes alive): is the button found/updated, or do we keep
			// re-creating it (duplicate widgets)? buttonsInMap reveals duplication. Remove once confirmed.
			PrintFormat("[PS_AliveDBG] update faction='%1' total=%2 alive=%3 buttonExisted=%4 buttonsInMap=%5",
				faction.GetFactionKey(), t.param1, t.param2, buttonExisted, m_aFactionButtons.Count());
			if (!m_aFactionButtons.Contains(faction))
			{
				// Default a newly-seen faction to "selected" so its button is created VISIBLE - AddFactionButton
				// hides buttons whose faction is not selected, which previously left a recompute-created button
				// hidden so the count never showed.
				if (!m_aSelectedFactions.Contains(faction))
					m_aSelectedFactions.Insert(faction);
				AddFactionButton(faction, t.param1, t.param2);
			}
			else
			{
				PS_AliveFactionButton button = m_aFactionButtons.Get(faction);
				button.SetCount(t.param1);
				button.SetCountAlive(t.param2);
			}
		}

		// Drop buttons for factions that no longer have any playables.
		array<SCR_Faction> stale = {};
		foreach (SCR_Faction faction, PS_AliveFactionButton button : m_aFactionButtons)
		{
			if (!tally.Contains(faction))
				stale.Insert(faction);
		}
		foreach (SCR_Faction staleFaction : stale)
		{
			m_aFactionButtons.Get(staleFaction).GetRootWidget().RemoveFromHierarchy();
			m_aFactionButtons.Remove(staleFaction);
		}
	}
	
	void AddPlayable(PS_PlayableContainer playable)
	{
		SCR_AIGroup playableGroup = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetRplId());
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

	void ~PS_AlivePlayerList()
	{
		// Best-effort unsubscribe if this instance is ever GC'd. The AUTHORITATIVE cleanup is the reset at the
		// top of InitList (runs on every menu open) - a widget component is not reliably collected the instant
		// its menu closes, so the destructor cannot be relied on as the only cleanup.
		if (GetGame() && m_PlayableManager)
		{
			PS_ScriptInvokerPlayable onRegistered = m_PlayableManager.GetOnPlayableRegistered();
			if (onRegistered)
				onRegistered.Remove(OnPlayableRegistered);
		}
	}
	
	void OnPlayableRegistered(RplId playableId, PS_PlayableContainer playable)
	{
		AddPlayable(playable);

		SCR_Faction faction = playable.GetFaction();
		if (faction && !m_aSelectedFactions.Contains(faction))
			m_aSelectedFactions.Insert(faction);
		RecomputeFactionCounts();
	}

	void OnAliveDie(PS_PlayableContainer playableContainer)
	{
		RecomputeFactionCounts();
	}

	void OnAliveRemoved(PS_PlayableContainer playableContainer)
	{
		RecomputeFactionCounts();
	}
	
	void OnAliveGroupRemoved(SCR_AIGroup group)
	{
		m_aAlivePlayerGroups.Remove(group);
	}
	
	// ETC
	bool IsShowDead()
	{
		return m_hShowDeathButton.IsToggled();
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
	
	void ShowDeadButtonClicked(SCR_ButtonBaseComponent deadButton)
	{
		m_OnShowDead.Invoke(m_hShowDeathButton.IsToggled());
	}
}