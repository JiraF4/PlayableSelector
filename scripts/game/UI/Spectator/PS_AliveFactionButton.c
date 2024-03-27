class PS_AliveFactionButton : SCR_ButtonBaseComponent
{
	ImageWidget m_wBackgroundFaction;
	TextWidget m_wSideCount;
	
	SCR_Faction m_Faction;
	int m_iCount;
	int m_iCountAlive;
	
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
		UpdateCount();
	}
	
	int GetCount()
	{
		return m_iCount;
	}
	
	void SetCountAlive(int countAlive)
	{
		m_iCountAlive = countAlive;
		UpdateCount();
	}
	
	int GetCountAlive()
	{
		return m_iCountAlive;
	}
	
	void UpdateCount()
	{
		m_wSideCount.SetText(m_iCountAlive.ToString() + "/" + m_iCount.ToString());
	}
}