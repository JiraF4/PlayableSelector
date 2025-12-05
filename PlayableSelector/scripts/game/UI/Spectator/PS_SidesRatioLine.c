class PS_SidesRatioLine : SCR_ScriptedWidgetComponent
{	
	PanelWidget m_wSideRatioLine;
	TextWidget m_wSideCount;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wSideRatioLine = PanelWidget.Cast(m_wRoot);
		m_wSideCount = TextWidget.Cast(m_wRoot.FindAnyWidget("SideCount"));
	}
	
	void SetFaction(FactionKey factionKey)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		
		Color color = faction.GetFactionColor();
		Color colorOuter = faction.GetOutlineFactionColor();
		
		m_wSideRatioLine.SetColor(color);
		m_wSideCount.SetColor(colorOuter);
	}
	
	void SetCount(int count)
	{
		m_wSideCount.SetText(count.ToString());
		
		HorizontalLayoutSlot.SetFillWeight(m_wRoot, count);
	}
}