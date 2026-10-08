class PS_PolyZoneObjectiveTriggerCaptureClass : PS_PolyZoneObjectiveTriggerClass
{

}

class PS_PolyZoneObjectiveTriggerCapture : PS_PolyZoneObjectiveTrigger
{
	protected const float REPLICATION_INTERVAL = 0.25;

	[Attribute("30")]
	float m_fCaptureTime;
	[Attribute("2")]
	int m_iPowerBalance;
	[Attribute(""), RplProp()]
	FactionKey m_sCurrentFaction;
	[Attribute("0")]
	bool m_bCanRetake;
	[Attribute("0")]
	bool m_bEasyTake;
	[Attribute("0")]
	bool m_bTimeLoss;
	
	ref map<FactionKey, int> m_mFactionCounters = new map<FactionKey, int>();
	ref map<FactionKey, float> m_mFactionTimers = new map<FactionKey, float>();
	protected float m_fReplicationAccum;
	protected ref map<FactionKey, float> m_mLastSentTimers = new map<FactionKey, float>();
	protected ref map<IEntity, FactionKey> m_mOccupantFactions = new map<IEntity, FactionKey>();
	
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
		
		foreach (PS_Objective objective : m_aObjectives)
		{
			FactionKey factionKey = objective.GetFactionKey();
			objective.SetCompleted(factionKey == m_sCurrentFaction);
		}
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
		if (!Replication.IsServer())
			return;
		
		// Throttled send-on-change dispatch (BUG-73); the capture math below stays per-frame.
		m_fReplicationAccum += timeSlice;
		if (m_fReplicationAccum >= REPLICATION_INTERVAL)
		{
			m_fReplicationAccum -= REPLICATION_INTERVAL;
			DispatchChangedTimers();
		}
		
		int maxDiff = 0;
		int maxCount = 0;
		FactionKey maxFaction = "";
		foreach (FactionKey factionKey, int count : m_mFactionCounters)
		{
			if (m_mFactionCounters[factionKey] > maxCount)
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
				Replication.BumpMe();
				foreach (FactionKey factionKey, float timer : m_mFactionTimers)
				{
					m_mFactionTimers[factionKey] = 0;
				}
				DispatchChangedTimers();
				UpdateObjectives();
				if (!m_bCanRetake)
				{
					ClearEventMask(EntityEvent.FRAME);
				}
			}
		}
	}
	
	/**
	 * @brief Sends only the faction timers whose value changed since the last send.
	 *
	 * @issue BUG-73
	 * @cause UpdateFactionTimers() broadcast every faction's timer on the Reliable channel
	 *       once per server frame, so a single contested trigger saturated the reliable
	 *       queue at framerate regardless of whether a single timer had moved.
	 * @solution Send-on-change against a server-local last-sent map, driven by the 250 ms
	 *       accumulator in OnFrame; capture completion flushes once so the final zeroed
	 *       values reach clients before ClearEventMask(FRAME) stops the loop.
	 */
	void DispatchChangedTimers()
	{
		foreach (FactionKey factionKey, float timer : m_mFactionTimers)
		{
			if (m_mLastSentTimers.Contains(factionKey) && m_mLastSentTimers[factionKey] == timer)
				continue;
			Rpc(RPC_UpdateFactionTimers, factionKey, timer);
			m_mLastSentTimers[factionKey] = timer;
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_UpdateFactionTimers(FactionKey factionKey, float timer)
	{
		m_mFactionTimers[factionKey] = timer;
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!super.ScriptedEntityFilterForQuery(ent))
			return false;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		if (!character)
			return false;
		SCR_DamageManagerComponent damageManagerComponent = character.GetDamageManager();
		if (!damageManagerComponent)
			return false;
		
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
	
	/**
	 * @brief Resolves an occupant's faction key, caching the result on entry.
	 *
	 * @issue BUG-74
	 * @cause ScriptedEntityFilterForQuery admits any living SCR_ChimeraCharacter, including
	 *       non-playables and unaffiliated ones; Count() dereferenced the whole
	 *       character → playable → affiliation → faction → key chain unguarded, so a
	 *       single ineligible occupant null-dereferenced the server on activate/deactivate.
	 * @solution Guard-chain early return after every Cast/getter. Ineligible occupants are
	 *       counted for nobody instead of crashing the frame; the filter's admit-set is
	 *       deliberately unchanged so trigger enter/leave semantics do not drift.
	 */
	FactionKey ResolveOccupantFaction(IEntity ent)
	{
		if (!ent)
			return "";
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		if (!character)
			return "";
		PS_PlayableComponent playableComponent = character.PS_GetPlayable();
		if (!playableComponent)
			return "";
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		if (!factionAffiliationComponent)
			return "";
		Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
		if (!faction)
			return "";
		
		return faction.GetFactionKey();
	}
	
	/**
	 * @brief Adds or removes an occupant from its faction tally.
	 *
	 * The faction key is cached per entity on the way in, so the way out never has to
	 * re-resolve the chain: an occupant deleted in the same frame it leaves the zone
	 * would fail every guard and leave its faction counter permanently elevated,
	 * skewing the power balance for the rest of the mission.
	 */
	void Count(IEntity ent, int side)
	{
		if (!ent)
			return;
		
		FactionKey factionKey;
		if (side > 0)
		{
			factionKey = ResolveOccupantFaction(ent);
			if (!factionKey)
				return;
			m_mOccupantFactions[ent] = factionKey;
		}
		else
		{
			if (!m_mOccupantFactions.Contains(ent))
				return;
			factionKey = m_mOccupantFactions[ent];
			m_mOccupantFactions.Remove(ent);
		}
		
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
