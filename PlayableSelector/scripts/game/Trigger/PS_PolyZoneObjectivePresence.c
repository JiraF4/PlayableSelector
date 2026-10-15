class PS_PolyZoneObjectivePresenceClass : PS_PolyZoneObjectiveTriggerClass
{
	
}

class PS_PolyZoneObjectivePresence : PS_PolyZoneObjectiveTrigger
{
	protected int m_iCount;
	protected bool m_bWasInHardFreeze;
	protected int m_iLastEvaluatedCount = -1;
	protected bool m_bInitialized;
	
	[Attribute("1")]
	int m_iNeedCount;
	[Attribute("0")]
	bool m_bOneTime;
	
	override protected void OnQueryFinished(bool bIsEmpty)
	{
		super.OnQueryFinished(bIsEmpty);
		if (!Replication.IsServer() || m_bAfterGame)
			return;

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			m_bWasInHardFreeze = true;
			return;
		}

		bool objectivesReady;
		if (m_aObjectives)
		{
			foreach (PS_Objective objective : m_aObjectives)
			{
				if (objective)
				{
					objectivesReady = true;
					break;
				}
			}
		}
		// LinkObjectives runs through the call queue; an earlier query must remain retryable.
		if (!objectivesReady)
		{
			m_bInitialized = false;
			return;
		}

		bool thawTransition = m_bWasInHardFreeze;
		m_bWasInHardFreeze = false;
		array<IEntity> entities = new array<IEntity>();
		GetEntitiesInside(entities);
		m_iCount = entities.Count();
		if (!m_bInitialized || thawTransition || m_iCount != m_iLastEvaluatedCount)
		{
			m_bInitialized = true;
			m_iLastEvaluatedCount = m_iCount;
			EvaluateObjectives_P(m_iCount);
		}
	}

	protected void EvaluateObjectives_P(int count)
	{
		bool rawCondition = count >= m_iNeedCount;
		bool targetCompleted = rawCondition;
		if (m_bReverseObjective)
			targetCompleted = !rawCondition;
		bool hasValidObjective;
		foreach (PS_Objective objective : m_aObjectives)
		{
			if (!objective)
				continue;
			hasValidObjective = true;
			if (m_bOneTime && objective.GetCompleted())
				continue;
			if (objective.GetCompleted() != targetCompleted)
				objective.SetCompleted(targetCompleted);
		}
		if (m_bOneTime && targetCompleted && hasValidObjective)
			EnablePeriodicQueries(false);
	}
	
	override protected void OnActivate(IEntity ent)
	{
		m_iCount++;
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		m_iCount--;
		if (m_iCount < 0)
			m_iCount = 0;
	}
}