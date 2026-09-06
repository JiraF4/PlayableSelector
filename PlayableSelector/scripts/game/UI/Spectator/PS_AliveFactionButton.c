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
		// Re-acquire defensively: if HandlerAttached ran before the child widget existed, m_wSideCount would
		// be null and the count would never paint (one suspect for "alive always shows 0").
		if (!m_wSideCount)
			m_wSideCount = TextWidget.Cast(m_wRoot.FindAnyWidget("SideCount"));
		if (m_wSideCount)
			m_wSideCount.SetText(m_iCountAlive.ToString());

		// TEMP DIAGNOSTIC (alive=0 while count computes alive): does SetCountAlive reach a VALID, VISIBLE text
		// widget, and what does it actually read back after SetText? If textNow shows the right value but the
		// screen shows 0, the visible box is a different/duplicate widget instance. Remove once confirmed.
		FactionKey dbgKey = "?";
		if (m_Faction)
			dbgKey = m_Faction.GetFactionKey();
		string dbgText = "<noWidget>";
		if (m_wSideCount)
			dbgText = m_wSideCount.GetText();
		PrintFormat("[PS_AliveDBG-btn] faction=%1 setAlive=%2 widgetValid=%3 textNow='%4' rootVisible=%5",
			dbgKey, m_iCountAlive, m_wSideCount != null, dbgText, m_wRoot.IsVisible());
	}
}