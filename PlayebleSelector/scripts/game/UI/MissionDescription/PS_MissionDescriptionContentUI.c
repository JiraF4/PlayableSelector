class PS_MissionDescriptionContentUI : ScriptedWidgetComponent
{
	PS_MissionDescriptionUI m_hMissionDescriptionUI;
	PS_MissionDescription m_MapDescription;
	
	void SetMissionUI(PS_MissionDescriptionUI missionDescriptionUI)
	{
		m_hMissionDescriptionUI = missionDescriptionUI;
	}
	
	void SetMissionDescription(PS_MissionDescription description)
	{
		m_MapDescription = description;
	}
	
	PS_MissionDescriptionUI GetMissionUI()
	{
		return m_hMissionDescriptionUI;
	}
	
	PS_MissionDescription GetMissionDescription()
	{
		return m_MapDescription;
	}
	
	string GetTitle()
	{
		return m_MapDescription.GetTitle();
	}
}