class PS_DebriefingFactionReportWidgetComponent : SCR_ScriptedWidgetComponent
{
	SCR_Faction m_Faction;
	
	ImageWidget m_wFactionColor;
	TextWidget m_wFactionName;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		
	}
	
	void SetFaction(Faction faction)
	{
		m_Faction = SCR_Faction.Cast(faction);
		
		m_wFactionColor.SetColor(m_Faction.GetOutlineFactionColor());
		m_wFactionName.SetText(m_Faction.GetFactionName());
	}
}
