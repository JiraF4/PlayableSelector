class PS_ObjectiveClass : PS_MissionDescriptionClass
{

}

class PS_Objective : PS_MissionDescription
{
	[Attribute("")]
	int m_iScore;

	[Attribute("")]
	FactionKey m_sFactionKey;

	[RplProp(onRplName: "OnObjectiveUpdate")]
	bool m_bCompleted;

	ref ScriptInvokerVoid m_OnObjectiveUpdate;

	RplComponent m_RplComponent;
	
	[Attribute("")]
	protected bool m_bAdvanceWhenTriggered;
	
	protected PS_GameModeCoop m_GameModeCoop;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
	}
	
	// Set
	void SetCompleted(bool completed)
	{
		m_bCompleted = completed;
		Replication.BumpMe();
		OnObjectiveUpdate();
	}

	// Get
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}

	int GetScore()
	{
		return m_iScore;
	}

	bool GetCompleted()
	{
		return m_bCompleted;
	}

	void OnObjectiveUpdate()
	{
		if (m_OnObjectiveUpdate)
			m_OnObjectiveUpdate.Invoke();
		
		if (Replication.IsServer() && m_bAdvanceWhenTriggered && m_bCompleted)
		{
			GetGame().GetCallqueue().CallLater(LateAdvance, 1100);	
		}
	}
	
	void LateAdvance()
	{
		if (m_bCompleted)
			m_GameModeCoop.AdvanceGameState(SCR_EGameModeState.GAME);
	}
	
	RplId GetRplId()
	{
		return m_RplComponent.ChildId(this);
	}
	
	ScriptInvokerVoid GetOnObjectiveUpdate()
	{
		if (!m_OnObjectiveUpdate)
			m_OnObjectiveUpdate = new ScriptInvokerVoid();
		return m_OnObjectiveUpdate;
	}

	void PS_Objective(IEntitySource src, IEntity parent)
	{
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().RegisterObjective(this);
	}
	
	void ~PS_Objective()
	{
		SetEventMask(EntityEvent.INIT);
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().UnRegisterObjective(this);
	}
}
