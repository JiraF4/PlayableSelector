class PS_AliveFactionButton : SCR_ButtonBaseComponent
{
	ImageWidget m_wBackgroundFaction;
	TextWidget m_wSideCount;
	
	SCR_Faction m_Faction;
	int m_iCount;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wBackgroundFaction = ImageWidget.Cast(m_wRoot.FindAnyWidget("BackgroundFaction"));
		m_wSideCount = TextWidget.Cast(m_wRoot.FindAnyWidget("SideCount"));
	}
	
	SCR_Faction GetFaction()
	{
		return m_Faction;
	}
	
	void SetFaction(SCR_Faction faction)
	{
		m_Faction = faction;
		
		Color color = m_Faction.GetFactionColor();
		Color colorOuter = m_Faction.GetOutlineFactionColor();
		
		m_wBackgroundFaction.SetColor(colorOuter);
	}
	
	void SetCount(int count)
	{
		m_iCount = count;
		m_wSideCount.SetText(m_iCount.ToString());
	}
	
	int GetCount()
	{
		return m_iCount;
	}
}