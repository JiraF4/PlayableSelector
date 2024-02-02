modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	DebriefingMenu
}

//------------------------------------------------------------------------------------------------
class PS_DebriefingMenu : ChimeraMenuBase
{
	static const ResourceName m_sFactionReportLayout = "{2987421913A58DF6}UI/Debriefing/FactionReportFrame.layout";
	
	protected HorizontalLayoutWidget m_wBodyHorizontalLayout;
		
	ref array<PS_DebriefingFactionReportWidgetComponent> m_aFactionReports = {};

	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{
		m_wBodyHorizontalLayout = HorizontalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("BodyHorizontalLayout"));
		
		FillFactions();
	}

	override void OnMenuInit()
	{

	}

	override void OnMenuUpdate(float tDelta)
	{

	}

	override void OnMenuClose()
	{

	}
	
	void FillFactions()
	{
		array<Faction> outFactions = {};
		GetGame().GetFactionManager().GetFactionsList(outFactions);
		foreach (Faction faction : outFactions)
		{
			Widget factionReportWidget = GetGame().GetWorkspace().CreateWidgets(m_sFactionReportLayout, m_wBodyHorizontalLayout);
			PS_DebriefingFactionReportWidgetComponent factionReport = PS_DebriefingFactionReportWidgetComponent.Cast(factionReportWidget.FindHandler(PS_DebriefingFactionReportWidgetComponent));
			m_aFactionReports.Insert(factionReport);
			factionReport.SetFaction(faction);
		}
	}
}
