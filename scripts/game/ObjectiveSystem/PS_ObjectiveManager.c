//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_ObjectiveManagerClass: ScriptComponentClass
{
	
};


class PS_ObjectiveManager : ScriptComponent
{
	[Attribute()]
	ref array<ref PS_ObjectiveLevel> m_aObjectiveLavels;
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
	
	void GetObjectives(out array<PS_Objective> objectivesOut)
	{
		foreach(PS_Objective objective: m_aObjectives)
		{
			objectivesOut.Insert(objective);
		}
	}
	
	void RegisterObjective(PS_Objective objective)
	{
		bool isInserted = false;
		
		for (int i = 0; i < m_aObjectives.Count(); i++)
		{
			if (m_aObjectives[i].m_iScore < objective.m_iScore)
			{
				m_aObjectives.InsertAt(objective, i);
				isInserted = true;
				break;
			}
		}
		
		if (!isInserted)
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
	
	PS_ObjectiveLevel GetFactionScoreLevel(FactionKey factionKey)
	{
		int score = 0;
		foreach (PS_Objective objective : m_aObjectives)
		{
			if (objective.GetFactionKey() == factionKey && objective.GetCompleted())
				score += objective.GetScore();
		}
		return GetObjectiveLevelByScore(score);
	}
	
	PS_ObjectiveLevel GetObjectiveLevelByScore(int score)
	{
		foreach (PS_ObjectiveLevel objectiveLevel : m_aObjectiveLavels)
		{
			if (objectiveLevel.GetScore() >= score)
				return objectiveLevel;
		}
		return m_aObjectiveLavels[m_aObjectiveLavels.Count() - 1];
	}
};

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class PS_ObjectiveLevel
{
	[Attribute()]
	int m_iScore;
	[Attribute()]
	string m_sName;
	
	int GetScore()
	{
		return m_iScore;
	}
	
	string GetName()
	{
		return m_sName;
	}
}