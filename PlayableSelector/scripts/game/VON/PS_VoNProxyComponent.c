// PS_VoNProxyComponent — marks a per-player VoN proxy entity and resolves
// playerId -> proxy on every machine. Ported from LiteLobby (LL_VoNProxyComponent).
//
// WHY a proxy entity at all: menu/lobby/spectator voice (players without a living
// character) transmits through a VoN component + radio that is NOT on the controlled
// entity. The engine only routes such audio when the sender's entity and radio exist
// on the RECEIVING machine too - PlayerControllers replicate owner<->server only, so
// components on the controller transmit fine but are never heard by other clients.
// The proxy is a plain entity replicated to ALL with streaming disabled, carrying
// SCR_VoNComponent + BaseRadioComponent, spawned per player by PS_VoNRoomsManager.
//
// The single replicated int (playerId) arrives with the entity stream, so JIP clients
// register proxies with zero extra bookkeeping.

class PS_VoNProxyComponentClass : ScriptComponentClass
{
}

class PS_VoNProxyComponent : ScriptComponent
{
	[RplProp(onRplName: "OnPlayerIdChanged")]
	protected int m_iPlayerId = -1;

	// playerId -> proxy, maintained on every machine by replication callbacks.
	protected static ref map<int, PS_VoNProxyComponent> s_mByPlayerId = new map<int, PS_VoNProxyComponent>();

	// =====================================================================
	// LOOKUP
	// =====================================================================

	static PS_VoNProxyComponent GetByPlayerId(int playerId)
	{
		PS_VoNProxyComponent comp;
		if (s_mByPlayerId.Find(playerId, comp))
			return comp;
		return null;
	}

	static IEntity GetProxyEntity(int playerId)
	{
		PS_VoNProxyComponent comp = GetByPlayerId(playerId);
		if (comp)
			return comp.GetOwner();
		return null;
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	// =====================================================================
	// ASSIGNMENT
	// =====================================================================

	// Server-only, right after spawn.
	void SetPlayerId_S(int playerId)
	{
		m_iPlayerId = playerId;
		Replication.BumpMe();
		Register();
	}

	protected void OnPlayerIdChanged()
	{
		Register();
	}

	protected void Register()
	{
		if (m_iPlayerId <= 0)
			return;

		s_mByPlayerId.Set(m_iPlayerId, this);

		// Tune this proxy's radio from the current replicated channel state. On clients
		// the proxy entity can stream in AFTER the channel data already arrived (JIP) -
		// registration is the one reliable "this radio exists now" hook on every machine.
		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
			vonMgr.ApplyRadioKey(m_iPlayerId);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_iPlayerId > 0)
		{
			PS_VoNProxyComponent current;
			if (s_mByPlayerId.Find(m_iPlayerId, current) && current == this)
				s_mByPlayerId.Remove(m_iPlayerId);
		}

		super.OnDelete(owner);
	}
}
