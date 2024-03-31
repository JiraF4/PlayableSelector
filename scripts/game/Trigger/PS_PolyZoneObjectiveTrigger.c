class PS_PolyZoneObjectiveTriggerClass : PS_PolyZoneTriggerClass
{

}

class PS_PolyZoneObjectiveTrigger : PS_PolyZoneTrigger
{
	ref array<PS_Objective> m_aObjectives = {};

	[Attribute()]
	ref array<string> m_aObjectiveNames;

	protected bool m_bAfterGame;
	
	override void OnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			EnablePeriodicQueries(false);
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.GetOnGameStateChange().Insert(OnGameStateChange);
		gameModeCoop.GetOnOnlyOneFactionAlive().Insert(OnOnlyOneFactionAlive);
		
		GetGame().GetCallqueue().Call(LinkObjectives);
	}
	
	void OnOnlyOneFactionAlive(FactionKey aliveFaction)
	{
		foreach (PS_Objective objective : m_aObjectives)
		{
			FactionKey factionKey = objective.GetFactionKey();
			objective.SetCompleted(factionKey == aliveFaction);
		}
	}
	
	void OnGameStateChange(SCR_EGameModeState state)
	{
		if (state == SCR_EGameModeState.DEBRIEFING)
			m_bAfterGame = true;
	}

	void LinkObjectives()
	{
		IEntity child = GetParent().GetChildren();
		while (child)
		{
			PS_Objective objective = PS_Objective.Cast(child);
			if (objective)
				m_aObjectives.Insert(objective);
			child = child.GetSibling();
		}
		foreach (string objectiveName : m_aObjectiveNames)
		{
			PS_Objective objective = PS_Objective.Cast(GetWorld().FindEntityByName(objectiveName));
			if (!objective)
				Print("Missing trigger objective: " + objectiveName, LogLevel.ERROR);
			else if (!m_aObjectives.Contains(objective))
				m_aObjectives.Insert(objective);
		}
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!super.ScriptedEntityFilterForQuery(ent))
			return false;
		
		return true;
	}
}
