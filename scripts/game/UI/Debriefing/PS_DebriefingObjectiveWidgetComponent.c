class PS_DebriefingObjectiveWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	PS_Objective m_Objective;
	
	TextWidget m_wObjectiveName;
	TextWidget m_wObjectiveScore;
	ImageWidget m_wObjectiveCompletion;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wObjectiveName = TextWidget.Cast(w.FindAnyWidget("ObjectiveName"));
		m_wObjectiveScore = TextWidget.Cast(w.FindAnyWidget("ObjectiveScore"));
		m_wObjectiveCompletion = ImageWidget.Cast(w.FindAnyWidget("ObjectiveCompletion"));
	}
	
	void SetObjective(PS_Objective objective)
	{
		m_Objective = objective;
		
		m_wObjectiveName.SetText(objective.GetTitle());
		m_wObjectiveScore.SetText(objective.GetScore().ToString());
		
		if (objective.GetCompleted())
			m_wObjectiveCompletion.LoadImageFromSet(0, m_sImageSet, "check");
		else
			m_wObjectiveCompletion.LoadImageFromSet(0, m_sImageSet, "cancel");
	}
}