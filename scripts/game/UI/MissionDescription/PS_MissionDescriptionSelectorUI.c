
class PS_MissionDescriptionSelectorUI : SCR_ButtonBaseComponent
{
	TextWidget m_wDescriptionName;
	
	string m_sTitle;
	ResourceName m_rContent;
	
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
	
	void SetTitle(string title)
	{
		m_sTitle = title;
		m_wDescriptionName.SetText(title);
	}
	
	void SetContent(ResourceName content)
	{
		m_rContent = content;
	}
	
	// -------------------- Buttons events --------------------
	void HeaderButtonClicked(SCR_ButtonBaseComponent headerButton)
	{
		PS_MissionDescriptionContentUI currentHandlerContent = PS_MissionDescriptionContentUI.Cast(m_wRoot.FindHandler(PS_MissionDescriptionContentUI));
		PS_MissionDescriptionUI missionDescription = currentHandlerContent.GetMissionUI();
		missionDescription.SwitchContent(m_rContent);
	}
}