modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	DebriefingMenu
}

//------------------------------------------------------------------------------------------------
class PS_DebriefingMenu : ChimeraMenuBase
{
	static const ResourceName m_sFactionReportLayout = "{2987421913A58DF6}UI/Debriefing/FactionReportFrame.layout";
	
	protected HorizontalLayoutWidget m_wBodyHorizontalLayout;
	
	protected Widget m_wGameModeHeader;
	protected PS_GameModeHeader m_hGameModeHeader;
		
	ref array<PS_DebriefingFactionReportWidgetComponent> m_aFactionReports = {};

	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{
		m_wBodyHorizontalLayout = HorizontalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("BodyHorizontalLayout"));
		
		m_wGameModeHeader = GetRootWidget().FindAnyWidget("GameModeHeader");
		m_hGameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		
		FillFactions();
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hGameModeHeader.Update();
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
