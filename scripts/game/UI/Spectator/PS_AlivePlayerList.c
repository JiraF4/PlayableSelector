// Widget displays info about alive players in game.
// Path: {18D3CF175C9AA974}UI/Spectator/AlivePlayersList.layout

class PS_AlivePlayerList : ScriptedWidgetComponent
{
	protected ResourceName m_sAlivePlayerSelectorPrefab = "{20DCB7288210C151}UI/Spectator/AlivePlayerSelector.layout";
	protected ResourceName m_sAliveFactionButtonPrefab = "{4959360D4111DACC}UI/Spectator/AliveFactionButton.layout";
	
	protected VerticalLayoutWidget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	protected ref array<PS_AlivePlayerSelector> m_aPlayersAliveList = {};
	
	protected HorizontalLayoutWidget m_wHorizontalLayoutFactions;
	protected ref array<Widget> m_aFactionButtonsWidgets = {};
	protected ref map<FactionKey, PS_AliveFactionButton> m_aFactionButtons = new map<FactionKey, PS_AliveFactionButton>;
	
	PS_SpectatorMenu m_mSpectatorMenu;
	
	FactionKey m_sSelectedFaction;
	
	override void HandlerAttached(Widget w)
	{
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("AlivePlayersList"));
		m_wHorizontalLayoutFactions = HorizontalLayoutWidget.Cast(w.FindAnyWidget("HorizontalLayoutFactions"));
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		
		array<FactionKey> factions = {};
		foreach (RplId id, PS_PlayableComponent playableComponent: playables)
		{
			FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(playableComponent.GetOwner().FindComponent(FactionAffiliationComponent));
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			if (!factions.Contains(factionKey))
				factions.Insert(factionKey);
		}
		
		foreach (FactionKey factionKey : factions)
		{
			Widget factionButtonWidget = GetGame().GetWorkspace().CreateWidgets(m_sAliveFactionButtonPrefab, m_wHorizontalLayoutFactions);
			PS_AliveFactionButton aliveFactionButton = PS_AliveFactionButton.Cast(factionButtonWidget.FindHandler(PS_AliveFactionButton));
			
			m_aFactionButtonsWidgets.Insert(factionButtonWidget);
			m_aFactionButtons.Insert(factionKey, aliveFactionButton);
			
			aliveFactionButton.SetFaction(factionKey);
			aliveFactionButton.m_OnClicked.Insert(FactionButtonClicked);
		}
	}
	
	private int m_iOldPlayersCount = 0;
	
	// -------------------- Update content functions --------------------
	void HardUpdate()
	{
		array<int> alivePlayers = new array<int>();
		GetAlivePlayers(alivePlayers);
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (m_sSelectedFaction != "")
		{
			for (int i = 0; i < alivePlayers.Count(); i++)
			{
				RplId playableId = playableManager.GetPlayableByPlayer(alivePlayers[i]);
				PS_PlayableComponent playable = playableManager.GetPlayableById(playableId);
				FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(playable.GetOwner().FindComponent(FactionAffiliationComponent));
				if (factionAffiliationComponent.GetDefaultAffiliatedFaction().GetFactionKey() != m_sSelectedFaction)
				{
					alivePlayers.Remove(i);
					i--;
				}
			}
		}
		
		if (m_iOldPlayersCount != alivePlayers.Count())
		{
			// Clear old widgets
			foreach (Widget widget: m_aPlayersListWidgets)
			{
				m_wPlayersList.RemoveChild(widget);
			}
			m_aPlayersAliveList.Clear();
			m_aPlayersListWidgets.Clear();
			
			// Add new widgets
			foreach (int playerId: alivePlayers)
			{
				Widget alivePlayerWidget = GetGame().GetWorkspace().CreateWidgets(m_sAlivePlayerSelectorPrefab);
				PS_AlivePlayerSelector alivePlayerSelector = PS_AlivePlayerSelector.Cast(alivePlayerWidget.FindHandler(PS_AlivePlayerSelector));
				
				m_wPlayersList.AddChild(alivePlayerWidget);
				m_aPlayersListWidgets.Insert(alivePlayerWidget);
				m_aPlayersAliveList.Insert(alivePlayerSelector);
				
				alivePlayerSelector.SetPlayer(playerId);
				alivePlayerSelector.SetSpectatorMenu(m_mSpectatorMenu);
			}
		}
		m_iOldPlayersCount = alivePlayers.Count();
		
		SoftUpdate();
	}
	
	void SoftUpdate()
	{
		foreach (PS_AlivePlayerSelector alivePlayerSelector: m_aPlayersAliveList)
		{
			alivePlayerSelector.UpdateInfo();
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		map<FactionKey, int> factions = new map<FactionKey, int>();
		array<int> alivePlayers = new array<int>();
		GetAlivePlayers(alivePlayers);
		foreach (int playerId: alivePlayers)
		{
			FactionKey factionKey = playableManager.GetPlayerFactionKey(playerId);
			if (factions.Contains(factionKey))
				factions[factionKey] = factions[factionKey] + 1;
			else
				factions[factionKey] = 1;
		}
		
		foreach (FactionKey factionKey, PS_AliveFactionButton aliveFactionButton : m_aFactionButtons)
		{
			if (factions.Contains(factionKey))
				aliveFactionButton.SetCount(factions[factionKey]);
			else
				aliveFactionButton.SetCount(0);
		}
	}
	
	void SetSpectatorMenu(PS_SpectatorMenu spectatorMenu)
	{
		m_mSpectatorMenu = spectatorMenu;
	}
	
	void GetAlivePlayers(out array<int> outPlayersArray)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		array<PS_PlayableComponent> playablesSorted = playableManager.GetPlayablesSorted();
		for (int i = 0; i < playablesSorted.Count(); i++) {
			PS_PlayableComponent playable = playablesSorted[i];
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			if (!character)
				continue;
			if (!character.GetDamageManager().IsDestroyed()) 
			{
				int playerId = playableManager.GetPlayerByPlayable(playable.GetId());
				if (playerId > 0 && !outPlayersArray.Contains(playerId))
					outPlayersArray.Insert(playerId);
			}
		}
	}
	
	
	// -------------------- Buttons events --------------------
	void FactionButtonClicked(SCR_ButtonBaseComponent playerButton)
	{
		foreach (FactionKey factionKey, PS_AliveFactionButton aliveFactionButton : m_aFactionButtons)
		{
			aliveFactionButton.SetToggled(false);
		}
		
		PS_AliveFactionButton aliveFactionButton = PS_AliveFactionButton.Cast(playerButton);
		
		FactionKey factionKey = aliveFactionButton.GetFactionKey();
		if (factionKey == m_sSelectedFaction)
			m_sSelectedFaction = "";
		else
		{
			aliveFactionButton.SetToggled(true);
			m_sSelectedFaction = factionKey;
		}
		m_iOldPlayersCount = -1;
		HardUpdate();
	}
}