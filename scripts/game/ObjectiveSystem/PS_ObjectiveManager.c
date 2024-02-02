//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_ObjectiveManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_ObjectiveManager : ScriptComponent
{
	ref array<PS_Objective> m_aObjectives = new array<PS_Objective>();
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_ObjectiveManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_ObjectiveManager.Cast(gameMode.FindComponent(PS_ObjectiveManager));
		else
			return null;
	}
	
	void RegisterObjective(PS_Objective objective)
	{
		m_aObjectives.Insert(objective);
	}
	
	void UnRegisterObjective(PS_Objective objective)
	{
		m_aObjectives.RemoveItem(objective);
	}
	
	void GetObjectivesByFactionKey(FactionKey factionKey, out notnull array<PS_Objective> outObjectives)
	{
		foreach (PS_Objective objective : m_aObjectives)
		{
			if (objective.GetFactionKey() == factionKey)
				outObjectives.Insert(objective);
		}
	}
};
