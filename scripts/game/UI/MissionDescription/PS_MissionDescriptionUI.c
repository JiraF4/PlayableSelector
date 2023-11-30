class PS_MissionDescriptionUI : ScriptedWidgetComponent
{
	protected ResourceName m_rStartLayout = "{B965F43FBD01E4B5}UI/MissionDescription/MissionDescriptionListScroll.layout";
	
	protected FrameWidget m_wContentFrame;
	protected Widget m_wCurrentContent;
	protected ImageWidget m_wFolderIcon;
	protected ButtonWidget m_wMissionDescriptionHeaderButton;
	protected TextWidget m_wMissionDescriptionHeaderText;
	
	protected PS_MissionDescription m_rPreviousMapDescription;
	protected PS_MissionDescription m_rCurrentMapDescription;
	
	// -------------------- Handler events --------------------
	override void HandlerAttached(Widget w)
	{
		m_wContentFrame = FrameWidget.Cast(w.FindAnyWidget("ContentFrame"));
		m_wMissionDescriptionHeaderButton = ButtonWidget.Cast(w.FindAnyWidget("MissionDescriptionHeaderButton"));
		m_wMissionDescriptionHeaderText = TextWidget.Cast(w.FindAnyWidget("MissionDescriptionHeaderText"));
		m_wFolderIcon = ImageWidget.Cast(w.FindAnyWidget("FolderIcon"));
		if (!GetGame().InPlayMode())
			return;
		OpenDescriptionList();
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		SCR_ButtonBaseComponent missionDescriptionHeaderButtonHandler = SCR_ButtonBaseComponent.Cast(m_wMissionDescriptionHeaderButton.FindHandler(SCR_ButtonBaseComponent));
		missionDescriptionHeaderButtonHandler.m_OnClicked.Insert(MissionDescriptionHeaderButtonClicked);
	}
	
	void RemoveCurrentContent(PS_MissionDescription mapDescription)
	{
		if (m_wCurrentContent)
		{
			m_wContentFrame.RemoveChild(m_wCurrentContent);
			m_rPreviousMapDescription = mapDescription;
			m_wFolderIcon.SetVisible(true);
		}
	}
	
	void OpenDescriptionList()
	{
		RemoveCurrentContent(null);
		m_wFolderIcon.SetVisible(false);
		
		m_wCurrentContent = GetGame().GetWorkspace().CreateWidgets(m_rStartLayout);
		PS_MissionDescriptionContentUI handler = PS_MissionDescriptionContentUI.Cast(m_wCurrentContent.FindHandler(PS_MissionDescriptionContentUI));
		handler.SetMissionUI(this);
		m_wMissionDescriptionHeaderText.SetText("Mission description");
		m_wContentFrame.AddChild(m_wCurrentContent);
	}
	
	void SwitchContent(PS_MissionDescription mapDescription)
	{
		RemoveCurrentContent(mapDescription);
		
		m_rCurrentMapDescription = mapDescription;
		m_wCurrentContent = GetGame().GetWorkspace().CreateWidgets(mapDescription.GetDescriptionLayout());
		
		PS_MissionDescriptionContentUI handler = PS_MissionDescriptionContentUI.Cast(m_wCurrentContent.FindHandler(PS_MissionDescriptionContentUI));
		handler.SetMissionUI(this);
		handler.SetMissionDescription(mapDescription);
		m_wMissionDescriptionHeaderText.SetText(handler.GetTitle());
		m_wContentFrame.AddChild(m_wCurrentContent);
	}
	
	void SwitchBack()
	{
		OpenDescriptionList();
		
		/*
		if (!m_rPreviousMapDescription) {
			OpenDescriptionList();
			return;
		}
		
		RemoveCurrentContent(null);
		
		m_wContentFrame.RemoveChild(m_wCurrentContent);
		m_rCurrentLayout = m_rPreviousLayout;
		m_wCurrentContent = GetGame().GetWorkspace().CreateWidgets(m_rCurrentLayout);
		m_rPreviousLayout = "";
		
		PS_MissionDescriptionContentUI handler = PS_MissionDescriptionContentUI.Cast(m_wCurrentContent.FindHandler(PS_MissionDescriptionContentUI));
		handler.SetMissionUI(this);
		m_wMissionDescriptionHeaderText.SetText(handler.GetTitle());
		m_wContentFrame.AddChild(m_wCurrentContent);
		m_wFolderIcon.SetVisible(false);
		*/
	}
	
	
	// -------------------- Buttons events --------------------
	void MissionDescriptionHeaderButtonClicked(SCR_ButtonBaseComponent headerButton)
	{
		SwitchBack();
	}
}