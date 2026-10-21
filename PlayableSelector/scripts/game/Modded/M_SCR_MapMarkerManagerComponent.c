//! Server-side faction gating for player-placed static markers, plus a one-shot client
//! filter for the join-in-progress payload.
//!
//! Vanilla broadcasts every marker to every client and filters on arrival in
//! OnAddSynchedMarker, where the guard "if (localFaction && !isMyFaction) remove" fails
//! open on a null faction: spectators and players whose affiliation has not resolved keep
//! every faction-restricted marker, including the enemy's. Filtering a broadcast client-side
//! is also a leak by definition (Principle IV), so the routing is done here instead.
//!
//! One path bypasses this routing and is handled separately: the manager's RplLoad fills
//! m_aStaticMarkers straight from the JIP payload with no faction test at all, and that
//! payload is shared by every joining client, so it cannot be filtered per player on the
//! server. It is filtered once, after loading, on the client.
//!
//! Dropping a marker client-side is safe here because the server re-sends on faction change
//! (PS_OnPlayerFactionChanged_S), which also fixes vanilla's destructive one-way
//! OnPlayerFactionChanged: the proxy used to delete the other factions' markers and never
//! re-add them, so a lobby player switching playable in runtime lost the whole map.
modded class SCR_MapMarkerManagerComponent
{
	//! Server only. Restricts a faction-scoped marker to the players entitled to see it and
	//! routes the payload per player; an unrestricted marker keeps the vanilla broadcast.
	override void OnAskAddStaticMarker(SCR_MapMarkerBase markerData)
	{
		if (!Replication.IsServer())
		{
			super.OnAskAddStaticMarker(markerData);
			return;
		}

		// Unrestricted (0) means "visible to everyone" and must reach every client, including
		// the factionless ones. Server-side storage is already handled before this call by
		// SCR_MapMarkerSyncComponent.RPC_AskAddStaticMarker, so the routing below applies to a
		// dedicated server exactly as it does to a listen host.
		if (!markerData || markerData.GetMarkerFactionFlags() == 0)
		{
			super.OnAskAddStaticMarker(markerData);
			return;
		}

		FactionManager factionManager = GetGame().GetFactionManager();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!factionManager || !playerManager)
		{
			super.OnAskAddStaticMarker(markerData);
			return;
		}

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
			if (!faction || !markerData.IsFaction(factionManager.GetFactionIndex(faction)))
				continue;

			PS_SendMarkerToPlayer_S(playerId, markerData);
		}
	}

	//! Server only. Vanilla registers this on SCR_FactionManager.GetOnPlayerFactionChanged_S
	//! and uses it for dynamic-marker streaming rules only. Static markers are re-sent here so
	//! a player who switches playable in runtime sees the new faction's markers instead of the
	//! gutted list its proxy just deleted.
	override void OnPlayerFactionChanged_S(int playerID, SCR_PlayerFactionAffiliationComponent playerComponent, Faction faction)
	{
		super.OnPlayerFactionChanged_S(playerID, playerComponent, faction);

		if (!Replication.IsServer() || !faction)
			return;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		int factionIndex = factionManager.GetFactionIndex(faction);
		if (factionIndex < 0)
			return;

		foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
		{
			if (marker && marker.GetMarkerFactionFlags() != 0 && marker.IsFaction(factionIndex))
				PS_SendMarkerToPlayer_S(playerID, marker);
		}

		foreach (SCR_MapMarkerBase marker : m_aDisabledMarkers)
		{
			if (marker && marker.GetMarkerFactionFlags() != 0 && marker.IsFaction(factionIndex))
				PS_SendMarkerToPlayer_S(playerID, marker);
		}
	}

	protected void PS_SendMarkerToPlayer_S(int playerId, notnull SCR_MapMarkerBase marker)
	{
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!controller)
			return;

		PS_PlayableControllerComponent clientBridge = PS_PlayableControllerComponent.Cast(controller.FindComponent(PS_PlayableControllerComponent));
		if (!clientBridge)
			return;

		clientBridge.PS_SendStaticMarker_S(marker);
	}

	//! Proxy only. Vanilla RplLoad inserts every marker of the JIP payload without a faction
	//! test; this drops the ones this client is not entitled to, once, right after the load.
	override event protected bool RplLoad(ScriptBitReader reader)
	{
		bool loaded = super.RplLoad(reader);

		// The server owns the full set and must not shed markers other factions will need.
		if (!Replication.IsServer())
			PS_DropUnentitledMarkers(m_aStaticMarkers);

		return loaded;
	}

	protected void PS_DropUnentitledMarkers(notnull array<ref SCR_MapMarkerBase> markers)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		// -1 stands for "no faction resolved yet", which is exactly the case vanilla lets through.
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		int localIndex = -1;
		if (localFaction)
			localIndex = factionManager.GetFactionIndex(localFaction);

		for (int i = markers.Count() - 1; i >= 0; i--)
		{
			SCR_MapMarkerBase marker = markers[i];
			if (!marker || marker.GetMarkerFactionFlags() == 0)
				continue;

			if (localIndex >= 0 && marker.IsFaction(localIndex))
				continue;

			markers.Remove(i);
			marker.OnDelete();
		}
	}

	override void Update(float timeSlice)
	{
		// Vanilla OnPostInit already resolves m_MapEntity once; re-subscribing to
		// GetOnMapPanEnd() here duplicated the listener on every cache miss.
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();

		super.Update(timeSlice);
	}

	override void OnMapPanEnd(float x, float y)
	{
		if (m_MapEntity)
			super.OnMapPanEnd(x, y);
	}
}