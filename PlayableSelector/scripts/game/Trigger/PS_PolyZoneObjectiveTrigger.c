class PS_PolyZoneObjectiveTriggerClass : PS_PolyZoneTriggerClass
{

}

class PS_PolyZoneObjectiveTrigger : PS_PolyZoneTrigger
{
	ref array<PS_Objective> m_aObjectives = {};

	[Attribute()]
	ref array<string> m_aObjectiveNames;

	[Attribute("0")]
	bool m_bReverseObjective;
	
	protected bool m_bAfterGame;
	protected bool m_bGameModeHooked;
	
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		
		if (!Replication.IsServer())
			EnablePeriodicQueries(false);
		
		HookGameMode();
		
		GetGame().GetCallqueue().Call(LinkObjectives);
	}
	
	/**
	 * @brief Subscribes the trigger to PS_GameModeCoop state events.
	 * @note The game mode may be absent (editor/sandbox scene worlds) or not yet
	 * registered while InitEntities runs OnInit; the subscription is retried once
	 * from the deferred LinkObjectives() and skipped silently if the game mode
	 * never appears, so the trigger keeps working without the hooks.
	 */
	protected void HookGameMode()
	{
		if (m_bGameModeHooked)
			return;
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameModeCoop)
			return;
		
		m_bGameModeHooked = true;
		gameModeCoop.GetOnGameStateChange().Insert(OnGameStateChange);
		gameModeCoop.GetOnOnlyOneFactionAlive().Insert(OnOnlyOneFactionAlive);
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
		HookGameMode();
		
		IEntity parent = GetParent();
		if (parent)
		{
			IEntity child = parent.GetChildren();
			while (child)
			{
				PS_Objective objective = PS_Objective.Cast(child);
				if (objective)
					m_aObjectives.Insert(objective);
				child = child.GetSibling();
			}
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
