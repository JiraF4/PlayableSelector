class PS_DebriefingObjectiveWidgetComponent : SCR_ScriptedWidgetComponent
{
	PS_Objective m_Objective;
	
	TextWidget m_wObjectiveName;
	TextWidget m_wObjectiveScore;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wObjectiveName = TextWidget.Cast(w.FindAnyWidget("ObjectiveName"));
		m_wObjectiveScore = TextWidget.Cast(w.FindAnyWidget("ObjectiveScore"));
	}
	
	void SetObjective(PS_Objective objective)
	{
		m_Objective = objective;
		
		m_wObjectiveName.SetText(objective.GetTitle());
		m_wObjectiveScore.SetText(objective.GetScore().ToString());
	}
}