class PS_PolyZoneObjectiveTriggerDestroyClass : PS_PolyZoneObjectiveTriggerClass
{

}

class PS_PolyZoneObjectiveTriggerDestroy : PS_PolyZoneObjectiveTrigger
{
	ref map<IEntity, SCR_DamageManagerComponent> m_mEntityDamageManagers = new map<IEntity, SCR_DamageManagerComponent>();
	float m_fDestroyedPercent;
	
	[Attribute("1")]
	int m_iSearchTimes;
	
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
	}
	
	override void LinkObjectives()
	{
		super.LinkObjectives();
		UpdateObjectives();
	}
	
	void UpdateObjectives()
	{
		if (m_bAfterGame)
			return;
		
		foreach (PS_Objective objective : m_aObjectives)
		{
			objective.SetCompleted(m_fDestroyedPercent >= 1.0);
		}
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (super.ScriptedEntityFilterForQuery(ent))
		
		if (!super.ScriptedEntityFilterForQuery(ent))
			return false;
		
		return true;
	}
	
	override protected void OnQueryFinished(bool bIsEmpty)
	{
		m_iSearchTimes--;
		if (m_iSearchTimes == 0)
			EnablePeriodicQueries(false);
	}
	
	override protected void OnActivate(IEntity ent)
	{
		SCR_DamageManagerComponent damageManagerComponent = SCR_DamageManagerComponent.Cast(ent.FindComponent(SCR_DamageManagerComponent));
		if (damageManagerComponent)
		{
			ScriptInvoker onDamageStateChanged = damageManagerComponent.GetOnDamageStateChanged();
			onDamageStateChanged.Insert(OnDamageStateChanged);
			m_mEntityDamageManagers.Insert(ent, damageManagerComponent);
		}
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		if (!m_mEntityDamageManagers.Contains(ent))
			return;
		SCR_DamageManagerComponent damageManagerComponent = m_mEntityDamageManagers[ent];
		ScriptInvoker onDamageStateChanged = damageManagerComponent.GetOnDamageStateChanged();
		onDamageStateChanged.Remove(OnDamageStateChanged);
		m_mEntityDamageManagers.Remove(ent);
	}
	
	void OnDamageStateChanged(EDamageState state)
	{
		int count = m_mEntityDamageManagers.Count();
		int destroyed = 0;
		foreach (IEntity entity, SCR_DamageManagerComponent damageManagerComponent : m_mEntityDamageManagers)
		{
			if (damageManagerComponent.GetState() == EDamageState.DESTROYED)
				destroyed++;
		}
		m_fDestroyedPercent = destroyed / count;
		UpdateObjectives();
	}
}
