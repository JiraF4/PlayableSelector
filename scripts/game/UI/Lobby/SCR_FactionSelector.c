class SCR_FactionSelector : SCR_ButtonBaseComponent
{
	protected int m_maxCount;
	protected int m_currentCount;
	protected Faction m_faction;
	
	ImageWidget m_wFactionFlag;
	TextWidget m_wFactionName;
	TextWidget m_wFactionCounter;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("FactionFlag"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
		m_wFactionCounter = TextWidget.Cast(w.FindAnyWidget("FactionCounter"));
	}
	
	void SetFaction(Faction faction)
	{
		m_faction = faction;
		
		m_wFactionName.SetText(m_faction.GetFactionName());
		m_wFactionFlag.LoadImageTexture(0, SCR_Faction.Cast(m_faction).GetFactionFlag());
	}
	
	void SetCount(int current, int max)
	{
		m_wFactionCounter.SetText(current.ToString() + " / " + max.ToString());
	}
	
	Faction GetFaction()
	{
		return m_faction;
	}
	
}