class PS_MissionDescriptionContentUI : ScriptedWidgetComponent
{
	PS_MissionDescriptionUI m_hMissionDescriptionUI;
	
	[Attribute("")]
	string m_sTitle;
	
	void SetMissionUI(PS_MissionDescriptionUI missionDescriptionUI)
	{
		m_hMissionDescriptionUI = missionDescriptionUI;
	}
	
	PS_MissionDescriptionUI GetMissionUI()
	{
		return m_hMissionDescriptionUI;
	}
	
	string GetTitle()
	{
		return m_sTitle;
	}
}