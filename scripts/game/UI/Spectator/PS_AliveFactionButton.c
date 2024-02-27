class PS_AliveFactionButton : SCR_ButtonBaseComponent
{
	ImageWidget m_wBackgroundFaction;
	TextWidget m_wSideCount;
	
	FactionKey m_sFactionKey;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wBackgroundFaction = ImageWidget.Cast(m_wRoot.FindAnyWidget("BackgroundFaction"));
		m_wSideCount = TextWidget.Cast(m_wRoot.FindAnyWidget("SideCount"));
	}
	
	void SetFaction(FactionKey factionKey)
	{
		m_sFactionKey = factionKey;
		
		FactionManager factionManager = GetGame().GetFactionManager();
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		
		Color color = faction.GetFactionColor();
		Color colorOuter = faction.GetOutlineFactionColor();
		
		m_wBackgroundFaction.SetColor(colorOuter);
	}
	
	void SetCount(int count)
	{
		m_wSideCount.SetText(count.ToString());
	}
	
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}
}