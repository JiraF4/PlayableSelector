class PS_PolyZoneObjectiveTriggerCaptureClass : PS_PolyZoneObjectiveTriggerClass
{

}

class PS_PolyZoneObjectiveTriggerCapture : PS_PolyZoneObjectiveTrigger
{
	[Attribute("30")]
	float m_fCaptureTime;
	[Attribute("2")]
	int m_iPowerBalance;
	[Attribute("")]
	FactionKey m_sCurrentFaction;
	[Attribute("0")]
	bool m_bCanRetake;
	[Attribute("0")]
	bool m_bEasyTake;
	[Attribute("0")]
	bool m_bTimeLoss;
	
	ref map<FactionKey, int> m_mFactionCounters = new map<FactionKey, int>();
	ref map<FactionKey, float> m_mFactionTimers = new map<FactionKey, float>();
	
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (Replication.IsServer())
		{
			SetEventMask(EntityEvent.FRAME);
			SetEventMask(EntityEvent.POSTFRAME);
		}
	}
	
	override void LinkObjectives()
	{
		super.LinkObjectives();
		UpdateObjectives();
	}
	
	override void OnOnlyOneFactionAlive(FactionKey aliveFaction)
	{
		m_sCurrentFaction = aliveFaction;
		UpdateObjectives();
	}
	
	void UpdateObjectives()
	{
		if (m_bAfterGame)
			return;
		
		foreach (PS_Objective objective : m_aObjectives)
		{
			FactionKey factionKey = objective.GetFactionKey();
			objective.SetCompleted(factionKey == m_sCurrentFaction);
		}
	}
	
	override void OnFrame(IEntity owner, float timeSlice)
	{
		int maxDiff = 0;
		int maxCount = 0;
		FactionKey maxFaction = "";
		foreach (FactionKey factionKey, int count : m_mFactionCounters)
		{
			if (m_mFactionCounters[factionKey] >= maxCount)
			{
				maxDiff = m_mFactionCounters[factionKey] - maxCount;
				maxCount = m_mFactionCounters[factionKey];
				maxFaction = factionKey;
			}
		}
		
		foreach (FactionKey factionKey, float timer : m_mFactionTimers)
		{
			if (factionKey != maxFaction)
				m_mFactionTimers[factionKey] = timer - timeSlice;
			if (m_mFactionTimers[factionKey] < 0)
				m_mFactionTimers[factionKey] = 0;
		}
		
		if (maxFaction == "")
			return;
		if (maxFaction == m_sCurrentFaction)
			return;
		
		if (maxDiff > m_iPowerBalance || (m_bEasyTake && maxDiff == maxCount && maxDiff > 0))
		{
			m_mFactionTimers[maxFaction] = m_mFactionTimers[maxFaction] + timeSlice;
			if (m_mFactionTimers[maxFaction] > m_fCaptureTime)
			{
				m_sCurrentFaction = maxFaction;
				foreach (FactionKey factionKey, float timer : m_mFactionTimers)
				{
					m_mFactionTimers[factionKey] = 0;
				}
				UpdateObjectives();
				if (!m_bCanRetake)
				{
					ClearEventMask(EntityEvent.FRAME);
				}
			}
		}
	}
	
	void UpdateFactionTimers()
	{
		foreach (FactionKey factionKey, float timer : m_mFactionTimers)
		{
			Rpc(RPC_UpdateFactionTimers, factionKey, timer);
		}
	}
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	void RPC_UpdateFactionTimers(FactionKey factionKey, float timer)
	{
		m_mFactionTimers[factionKey] = timer;
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!super.ScriptedEntityFilterForQuery(ent))
			return false;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		PS_PlayableComponent playableComponent = character.PS_GetPlayable();
		SCR_DamageManagerComponent damageManagerComponent = playableComponent.GetCharacterDamageManagerComponent();
		
		return damageManagerComponent.GetState() != EDamageState.DESTROYED;
	}
	
	bool IsCurrentFaction(FactionKey factionKey)
	{
		return m_sCurrentFaction == factionKey;
	}
	
	float GetFactionTime(FactionKey factionKey)
	{
		if (!m_mFactionTimers.Contains(factionKey))
			return m_fCaptureTime;
		return m_fCaptureTime - m_mFactionTimers[factionKey];
	}
	
	override protected void OnActivate(IEntity ent)
	{
		super.OnActivate(ent);
		Count(ent, 1);
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		super.OnDeactivate(ent);
		Count(ent, -1);
	}
	
	void Count(IEntity ent, int side)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		PS_PlayableComponent playableComponent = character.PS_GetPlayable();
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
		FactionKey factionKey = faction.GetFactionKey();
		if (!m_mFactionCounters.Contains(factionKey))
		{
			m_mFactionCounters[factionKey] = 0;
			m_mFactionTimers[factionKey] = 0;
		}
		int counter = m_mFactionCounters[factionKey];
		counter += side;
		m_mFactionCounters[factionKey] = counter;
	}
}
