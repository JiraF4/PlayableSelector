class PS_PolyZoneObjectiveTriggerClass : SCR_BaseTriggerEntityClass
{

}

class PS_PolyZoneObjectiveTrigger : SCR_BaseTriggerEntity
{
	PS_PolyZone m_PolyZone;
	ref array<PS_Objective> m_aObjectives = {};

	[Attribute()]
	ref array<string> m_aObjectiveNames;

	override void OnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			EnablePeriodicQueries(false);
		m_PolyZone = PS_PolyZone.Cast(owner.GetParent().FindComponent(PS_PolyZone));
		GetGame().GetCallqueue().Call(LinkObjectives);
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
		if (!m_PolyZone.IsInsidePolygon(ent.GetOrigin()))
			return false;
		
		return true;
	}
}
