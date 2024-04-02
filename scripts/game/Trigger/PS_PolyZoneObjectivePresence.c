class PS_PolyZoneObjectivePresenceClass : PS_PolyZoneObjectiveTriggerClass
{
	
}

class PS_PolyZoneObjectivePresence : PS_PolyZoneObjectiveTrigger
{
	int m_iCount;
	
	[Attribute("1")]
	int m_iNeedCount;
	[Attribute("0")]
	bool m_bOneTime;
	
	void UpdateObjectives()
	{
		if (m_bAfterGame)
			return;
		if (m_iCount >= m_iNeedCount && m_bOneTime)
			EnablePeriodicQueries(false);
		foreach (PS_Objective objective : m_aObjectives)
		{
			if (!m_bOneTime || !objective.GetCompleted())
				objective.SetCompleted(m_iCount >= m_iNeedCount);
		}
	}
	
	override protected void OnActivate(IEntity ent)
	{
		m_iCount++;
		UpdateObjectives();
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		m_iCount--;
		UpdateObjectives();
	}
}