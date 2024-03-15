class PS_AlivePlayerGroup : SCR_ScriptedWidgetComponent
{
	// Const
	protected ResourceName m_sAlivePlayerSelectorPrefab = "{20DCB7288210C151}UI/Spectator/AlivePlayerSelector.layout";
	
	// Widgets
	protected VerticalLayoutWidget m_PlayersVerticalLayout;
	protected TextWidget m_wGroupName;
	protected ImageWidget m_wGroupFactionColor;
	
	// Cache global
	protected WorkspaceWidget m_WorkspaceWidget;
	
	// Parameters
	protected PS_AlivePlayerList m_AlivePlayerList;
	protected PS_SpectatorMenu m_SpectatorMenu;
	protected SCR_AIGroup m_AIGroup;
	
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Widgets
		m_PlayersVerticalLayout = VerticalLayoutWidget.Cast(w.FindWidget("PlayersVerticalLayout"));
		m_wGroupName = TextWidget.Cast(w.FindAnyWidget("GroupName"));
		m_wGroupFactionColor = ImageWidget.Cast(w.FindAnyWidget("GroupFactionColor"));
		
		// Cache global
		m_WorkspaceWidget = GetGame().GetWorkspace();
	}
	
	void SetSpectatorMenu(PS_SpectatorMenu spectatorMenu)
	{
		m_SpectatorMenu = spectatorMenu;
	}
	
	void SetAlivePlayerList(PS_AlivePlayerList alivePlayerList)
	{
		m_AlivePlayerList = alivePlayerList;
	}
	
	void SetAIGroup(SCR_AIGroup aiGroup)
	{
		m_AIGroup = aiGroup;
		
		// Init
		string groupName = PS_GroupHelper.GetGroupFullName(m_AIGroup);
		m_wGroupName.SetText(groupName);
		Faction faction = m_AIGroup.GetFaction();
		
		Color factionColor = faction.GetFactionColor();
		m_wGroupFactionColor.SetColor(factionColor);
	}
	
	void InsertPlayable(PS_PlayableComponent playable)
	{
		Widget alivePlayerRoot = m_WorkspaceWidget.CreateWidgets(m_sAlivePlayerSelectorPrefab, m_PlayersVerticalLayout);
		PS_AlivePlayerSelector alivePlayerSelector = PS_AlivePlayerSelector.Cast(alivePlayerRoot.FindHandler(PS_AlivePlayerSelector));
		alivePlayerSelector.SetAliveGroup(this);
		alivePlayerSelector.SetSpectatorMenu(m_SpectatorMenu);
		alivePlayerSelector.SetAlivePlayerList(m_AlivePlayerList);
		alivePlayerSelector.SetPlayable(playable.GetId());
	}
	
	void OnAliveRemoved(PS_PlayableComponent playableComponent)
	{
		if (!m_PlayersVerticalLayout.GetChildren())
		{
			m_wRoot.RemoveFromHierarchy();
			m_AlivePlayerList.OnAliveGroupRemoved(m_AIGroup);
		}
	}
}











