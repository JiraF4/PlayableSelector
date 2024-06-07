class PS_FactionReadyListWidgetComponent : SCR_ScriptedWidgetComponent
{
	static const string BUTTON_RESOURCE_PATH = "{45A0B294E14FA840}UI/FactionReady/FactionReadyHideableButton.layout";
	
	WorkspaceWidget m_wWorkspace;
	PlayerManager m_PlayerManager;
	PS_PlayableManager m_PlayableManager;
	HorizontalLayoutWidget m_wFactionReadyFactions;
	ref map<FactionKey, PS_FactionReadyWidgetComponent> m_mFactionWidgets = new map<FactionKey, PS_FactionReadyWidgetComponent>();
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wWorkspace = GetGame().GetWorkspace();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerManager = GetGame().GetPlayerManager();
		m_wFactionReadyFactions = HorizontalLayoutWidget.Cast(w.FindAnyWidget("FactionReadyFactions"));
		
		if (m_PlayableManager)
			m_PlayableManager.m_eOnFactionChange.Insert(FillFactions);
		
		FillFactions(0, "", "");	
	}
	
	//------------------------------------------------------------------------------------------------
	void FillFactions(int _playerId, FactionKey _factionKey, FactionKey _factionKeyOld)
	{
		if (!m_PlayableManager)
			return;
		
		foreach (FactionKey factionKey, PS_FactionReadyWidgetComponent factionReadyWidgetComponent : m_mFactionWidgets)
		{
			factionReadyWidgetComponent.GetRootWidget().RemoveFromHierarchy();
		}
		m_mFactionWidgets.Clear();
		
		array<int> players = {};
		m_PlayerManager.GetPlayers(players);
		foreach (int playerId : players)
		{
			FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(playerId);
			if (factionKey == "")
				continue;
			
			if (!m_mFactionWidgets.Contains(factionKey))
			{
				Widget factionWidget = m_wWorkspace.CreateWidgets(BUTTON_RESOURCE_PATH, m_wFactionReadyFactions);
				PS_FactionReadyWidgetComponent factionReadyWidgetComponent = PS_FactionReadyWidgetComponent.Cast(factionWidget.FindHandler(PS_FactionReadyWidgetComponent));
				factionReadyWidgetComponent.Init(this, factionKey);
				m_mFactionWidgets.Insert(factionKey, factionReadyWidgetComponent);
			}
		}
		
		m_wRoot.SetVisible(!m_mFactionWidgets.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_PlayableManager)
			m_PlayableManager.m_eOnFactionChange.Remove(FillFactions);
	}
}
