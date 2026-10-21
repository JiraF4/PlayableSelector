modded class SCR_MapMarkerSyncComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_iPlacedMarkerLimit = 100;
	}

	protected bool PS_CanEditSharedMarkers()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
			return false;

		return true;
	}

	override protected void RPC_AskAddStaticMarker(SCR_MapMarkerBase markerData)
	{
		if (!PS_CanEditSharedMarkers())
			return;

		super.RPC_AskAddStaticMarker(markerData);
	}

	override protected void RPC_AskRemoveStaticMarker(int markerID)
	{
		if (!PS_CanEditSharedMarkers())
			return;

		super.RPC_AskRemoveStaticMarker(markerID);
	}

	//! Never clears markers on disconnect. The vanilla loop advances only when the removal
	//! RPC it fires runs synchronously; it is a Server-receiver RPC invoked from the server,
	//! so `m_OwnedMarkers` can fail to shrink and the loop never terminates — the watchdog
	//! then force-crashes the server with no callstack. Keeping the markers is also the
	//! behaviour wanted here: lobby players time out and reconnect, and the markers
	//! belonging to a departing player would otherwise never come back.
	//!
	//! Orphaned markers — FR-002, resolved by operator on 2026-10-21 ("accept the risk"):
	//! they keep their stale ownerID, stay faction-gated by the server-side routing, and the
	//! playerId-reuse collision (a new player inheriting the old ownerID) is accepted for the
	//! lobby. `m_OwnedMarkers` dies with the departing controller, so orphans no longer count
	//! toward any limit — the per-player limit (m_iPlacedMarkerLimit = 100) is the sole brake
	//! on a live controller's placements. Removing an orphan still requires its stale ownerID
	//! unless the mission header enables delete-by-anyone.
	override void ClearOwnedMarkers()
	{
		PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		int playerId;
		if (controller)
			playerId = controller.GetPlayerId();

		Print(string.Format("[PS_Markers] Keeping %1 marker(s) of disconnected player %2", m_OwnedMarkers.Count(), playerId), LogLevel.NORMAL);
	}
}


class PS_MapMarkersBaseJson : JsonApiStruct
{
	ref array<ref PS_MapMarkerBaseJson> m_aMapMarkers = {};
	
	void PS_MapMarkersBaseJson()
	{
		RegAll();
	}
}

class PS_MapMarkerBaseJson : JsonApiStruct
{
	int m_eType;
	int m_iConfigID;
	int m_iFlags;
	int m_iPosWorldX;
	int m_iPosWorldY;
	int m_iFactionFlags;
	int m_iColorEntry;
	int m_iIconEntry;
	int m_iRotation;
	string m_sCustomText;
	
	SCR_MapMarkerBase GetMapMarkerBase()
	{
		SCR_MapMarkerBase mapMarkerBase = new SCR_MapMarkerBase();
		mapMarkerBase.PS_SetMapMarkerBaseJson(this);
		return mapMarkerBase;
	}
	
	void PS_MapMarkerBaseJson()
	{
		RegAll();
	}
}

modded class SCR_MapMarkerBase
{
	PS_MapMarkerBaseJson PS_GetMapMarkerBaseJson()
	{
		PS_MapMarkerBaseJson mapMarkerBaseJson = new PS_MapMarkerBaseJson();
		mapMarkerBaseJson.m_eType = m_eType;
		mapMarkerBaseJson.m_iConfigID = m_iConfigID;
		mapMarkerBaseJson.m_iFlags = m_iFlags;
		mapMarkerBaseJson.m_iPosWorldX = m_iPosWorldX;
		mapMarkerBaseJson.m_iPosWorldY = m_iPosWorldY;
		mapMarkerBaseJson.m_iFactionFlags = m_iFactionFlags;
		mapMarkerBaseJson.m_iColorEntry = m_iColorEntry;
		mapMarkerBaseJson.m_iIconEntry = m_iIconEntry;
		mapMarkerBaseJson.m_iRotation = m_iRotation;
		mapMarkerBaseJson.m_sCustomText = m_sCustomText;
		return mapMarkerBaseJson;
	}
	
	void PS_SetMapMarkerBaseJson(PS_MapMarkerBaseJson mapMarkerBaseJson)
	{
		m_eType = mapMarkerBaseJson.m_eType;
		m_iConfigID = mapMarkerBaseJson.m_iConfigID;
		m_iFlags = mapMarkerBaseJson.m_iFlags;
		m_iPosWorldX = mapMarkerBaseJson.m_iPosWorldX;
		m_iPosWorldY = mapMarkerBaseJson.m_iPosWorldY;
		m_iFactionFlags = mapMarkerBaseJson.m_iFactionFlags;
		m_iColorEntry = mapMarkerBaseJson.m_iColorEntry;
		m_iIconEntry = mapMarkerBaseJson.m_iIconEntry;
		m_iRotation = mapMarkerBaseJson.m_iRotation;
		m_sCustomText = mapMarkerBaseJson.m_sCustomText;
	}

	// Replication codec. Do NOT remove, even though every body only forwards to super.
	// SCR_MapMarkerBase declares Extract/Inject/Encode/Decode/SnapCompare/PropCompare as plain
	// statics, not overrides, and the script code generator regenerates the codec for a modded
	// class from the declarations found on that class. Deleting them fails the whole module with
	// "Method `SCR_MapMarkerBase.Encode` is missing".
	//------------------------------------------------------------------------------------------------
	
	override static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		super.Encode(snapshot, ctx, packet);
	}
	
	override static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		return super.Decode(packet, ctx, snapshot);
	}
	
	override static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
	{
		return super.SnapCompare(lhs, rhs , ctx);
	}

	override static bool PropCompare(SCR_MapMarkerBase instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return super.PropCompare(instance, snapshot, ctx);

	}
	
	override static bool Extract(SCR_MapMarkerBase instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		return super.Extract(instance, ctx, snapshot);
	}

	//------------------------------------------------------------------------------------------------
	override static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, SCR_MapMarkerBase instance)
	{
		return super.Inject(snapshot, ctx, instance);
	}
}
