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
	
	[Attribute("-1")]
	int m_bPolyzoneColorToFactionAlpha;
	
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

	/**
	 * @brief Invoker callback and [RplProp] replication handler for m_bCompleted.
	 *
	 * @issue BUG-77
	 * @sync Registered as the onRplName handler of m_bCompleted, so it executes on the
	 *       server and on every client that receives the state; the invoker call and the
	 *       server-only LateAdvance path are therefore shared by all peers.
	 * @cause The recolour branch dereferenced faction, GetParent() and the PS_PolyZone
	 *       child without guards, so an objective with an unresolvable faction key, no
	 *       parent, or no PolyZone child VME'd on every connected client at once.
	 * @solution Guard each link and skip only the recolour when one is missing; the
	 *       invoker invoke and the server-side LateAdvance scheduling stay unconditional
	 *       so objective flow is unaffected by a degenerate link.
	 */
	void OnObjectiveUpdate()
	{
		if (m_OnObjectiveUpdate)
			m_OnObjectiveUpdate.Invoke();
		
		if (m_bCompleted && m_bPolyzoneColorToFactionAlpha >= 0)
			RecolourPolyZone();
		
		if (Replication.IsServer() && m_bAdvanceWhenTriggered && m_bCompleted)
		{
			GetGame().GetCallqueue().CallLater(LateAdvance, 1100);	
		}
	}
	
	/**
	 * @brief Tints the parent PolyZone in the objective faction colour, skipping the
	 *        tint when any link of the objective → faction → parent → PolyZone chain
	 *        is missing.
	 */
	protected void RecolourPolyZone()
	{
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(m_sFactionKey));
		if (!faction)
			return;
		IEntity parent = GetParent();
		if (!parent)
			return;
		PS_PolyZone polyZone = PS_PolyZone.Cast(parent.FindComponent(PS_PolyZone));
		if (!polyZone)
			return;
		Color colorFill = faction.GetFactionColor();
		Color colorOutline = faction.GetOutlineFactionColor();
		polyZone.m_cPolygonColor = Color.FromRGBA(colorFill.R() * 255, colorFill.G() * 255, colorFill.B() * 255, m_bPolyzoneColorToFactionAlpha);
		polyZone.m_cPolygonBorderColor = Color.FromRGBA(colorOutline.R() * 255, colorOutline.G() * 255, colorOutline.B() * 255, m_bPolyzoneColorToFactionAlpha);
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
		GetGame().GetCallqueue().Call(LateRegister);
	}
	
	void LateRegister()
	{
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().RegisterObjective(this);
	}
	
	/**
	 * @brief Unregisters the objective from the manager on teardown.
	 *
	 * @issue BUG-78
	 * @cause The destructor mutated the entity's own event mask via SetEventMask(INIT)
	 *       and ran manager bookkeeping during GC, whose ordering is not deterministic.
	 *       BUGS.md's prescribed replacement, override void OnDelete(IEntity owner), is not
	 *       available: OnDelete is a ScriptComponent hook and PS_Objective is a
	 *       GenericEntity, which exposes no entity delete event.
	 * @solution Drop the event-mask mutation entirely — a dying entity gains nothing from
	 *       an INIT mask — and keep only the GetInstance()-null-guarded unregister, the
	 *       pattern already proven by the base ~PS_MissionDescription. UnRegisterObjective
	 *       reduces to RemoveItem, so a repeated or out-of-order call is a no-op and GC
	 *       ordering cannot fault it.
	 */
	void ~PS_Objective()
	{
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().UnRegisterObjective(this);
	}
}
