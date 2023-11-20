class PS_MissionDescriptionManagerClass : ScriptComponentClass
{
	
}

class PS_MissionDescriptionManager : ScriptComponent
{
	ref array<PS_MissionDescription> m_aDescriptions = new array<PS_MissionDescription>();
	
	void RegisterDescription(PS_MissionDescription mapDescription)
	{
		if (!m_aDescriptions.Contains(mapDescription))
			m_aDescriptions.Insert(mapDescription);
	}
	void UnregisterDescription(PS_MissionDescription mapDescription)
	{
		m_aDescriptions.RemoveItem(mapDescription);
	}
		
	override protected void OnPostInit(IEntity owner)
	{
		foreach(PS_MissionDescription description: m_aDescriptions)
		{
			//description.m_sFactions.Split(",", description.m_aFactions, true);
		}
	}
	
	void GetDescriptions(out array<PS_MissionDescription> descriptionsOut)
	{
		foreach(PS_MissionDescription description: m_aDescriptions)
		{
			descriptionsOut.Insert(description);
		}
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_MissionDescriptionManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_MissionDescriptionManager.Cast(gameMode.FindComponent(PS_MissionDescriptionManager));
		else
			return null;
	}
}