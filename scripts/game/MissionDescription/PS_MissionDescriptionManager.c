class PS_MissionDescriptionManagerClass : ScriptComponentClass
{

}

class PS_MissionDescriptionManager : ScriptComponent
{
	[Attribute("", UIWidgets.Object, "List of mission description layouts.")]
	ref array<ref PS_MapDescription> m_aDescriptions;
		
	override protected void OnPostInit(IEntity owner)
	{
		foreach(PS_MapDescription description: m_aDescriptions)
		{
			description.m_sFactions.Split(",", description.m_aFactions, true);
		}
	}
	
	void GetDescriptions(out array<PS_MapDescription> descriptionsOut)
	{
		foreach(PS_MapDescription description: m_aDescriptions)
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