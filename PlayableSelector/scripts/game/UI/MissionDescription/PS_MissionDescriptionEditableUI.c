class PS_MissionDescriptionEditableUI : ScriptedWidgetComponent
{
	Widget m_wRoot;
	
	protected PS_MissionDescription m_rMissionDescription;
	
	RichTextWidget m_wRichTextEditable;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
		
		m_wRichTextEditable = RichTextWidget.Cast(w.FindAnyWidget("RichTextEditable"));
		
		GetGame().GetCallqueue().CallLater(GetDescription, 0);
	}
	void GetDescription()
	{
		PS_MissionDescriptionContentUI currentHandlerContent = PS_MissionDescriptionContentUI.Cast(m_wRoot.FindHandler(PS_MissionDescriptionContentUI));
		PS_MissionDescription missionDescription = currentHandlerContent.GetMissionDescription();
		m_rMissionDescription = missionDescription;
		
		UpdateDescription();
	}
	
	void UpdateDescription()
	{
		m_wRichTextEditable.SetText(m_rMissionDescription.GetTextData());
	}
	
}