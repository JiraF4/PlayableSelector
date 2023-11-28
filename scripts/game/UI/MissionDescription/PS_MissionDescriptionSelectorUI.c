
class PS_MissionDescriptionSelectorUI : SCR_ButtonBaseComponent
{
	TextWidget m_wDescriptionName;
	
	PS_MissionDescription m_Description;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wDescriptionName = TextWidget.Cast(w.FindAnyWidget("DescriptionName"));
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		m_OnClicked.Insert(HeaderButtonClicked);
	}
	
	void SetDescription(PS_MissionDescription description)
	{
		m_Description = description;
		m_wDescriptionName.SetText(description.GetTitle());
	}
	
	// -------------------- Buttons events --------------------
	void HeaderButtonClicked(SCR_ButtonBaseComponent headerButton)
	{
		PS_MissionDescriptionContentUI currentHandlerContent = PS_MissionDescriptionContentUI.Cast(m_wRoot.FindHandler(PS_MissionDescriptionContentUI));
		PS_MissionDescriptionUI missionDescription = currentHandlerContent.GetMissionUI();
		missionDescription.SwitchContent(m_Description);
	}
}