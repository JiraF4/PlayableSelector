//------------------------------------------------------------------------------------------------
//! Central network and drawing manager for PS_TrackerComponent.
//! Gathers tracker positions on the server, coalesces deltas into batched RPCs,
//! and renders dynamically on the client map.

//! Wire representation of one tracker config and state.
class PS_TrackerData
{
	int m_iId;

	string m_sImageSet;
	string m_sQuad;
	string m_sLabel;
	float m_fColorR;
	float m_fColorG;
	float m_fColorB;
	float m_fColorA;
	bool m_bUseFactionColor;
	string m_sFactionKey;
	string m_sVisibleFactions;
	bool m_bAnyFaction;
	float m_fWorldSize;
	bool m_bUseWorldScale;
	float m_fMinSize;
	int m_iZOrder;
	bool m_bShowHeading;

	bool m_bBeacon;
	float m_fBeaconRange;

	vector m_vPos;
	float m_fYaw;
	bool m_bHasPos;
	bool m_bStale;

	bool m_bKeepLastPosition;
}

//! Authority-side bookkeeping for one live tracker.
class PS_TrackerRuntime
{
	PS_TrackerComponent m_Component;
	PS_TrackerData m_Data;
	float m_fInterval;
	float m_fAccum;
	int m_iSentX;
	int m_iSentZ;
	int m_iSentYaw;
	bool m_bEverSent;

	float m_fBeaconInterval;
	float m_fBeaconAccum;
}

class PS_TrackerManagerClass : SCR_BaseGameModeComponentClass
{
}

class PS_TrackerManager : SCR_BaseGameModeComponent
{
	protected const int BASE_TICK_MS = 250;
	protected const float BASE_TICK_S = 0.25;

	protected const int POS_EPSILON = 1;
	protected const int YAW_EPSILON = 5;

	protected const ResourceName MARKER_LAYOUT = "{52CA8FF5F56C6F31}UI/Map/ManualMapMarkerBase.layout";
	protected const ResourceName BEEP_PROJECT = "{69F1A2B3C4D50200}Sounds/Tracker/ps_tracker_sounds.acp";
	protected const string BEEP_EVENT = "PS_TRACKER_BEEP";

	protected const float STALE_OPACITY = 0.55;

	protected const int STRIDE_STRINGS = 5;
	protected const int STRIDE_INTS = 3;
	protected const int STRIDE_FLOATS = 10;

	protected ref array<ref PS_TrackerData> m_aTrackers = {};
	protected int m_iVersion;

	protected ref array<ref PS_TrackerRuntime> m_aRuntime = {};
	protected bool m_bLive;
	protected int m_iNextId;
	protected bool m_bSyncQueued;

	protected bool m_bBeepProjectLoaded;

	// Client map drawing cache
	protected SCR_MapEntity m_MapEntity;
	protected bool m_bMapOpen;
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref PS_TrackerData> m_aRenderData = {};
	protected int m_iBuiltVersion = -1;

	static PS_TrackerManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		return PS_TrackerManager.Cast(gameMode.FindComponent(PS_TrackerManager));
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		GetGame().GetCallqueue().Call(SubscribeToState);
	}

	override void OnDelete(IEntity owner)
	{
		PS_GameModeCoop mode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (mode)
			mode.GetOnGameStateChange().Remove(OnLobbyStateChanged_S);

		UnhookMapEntity_C();
		CloseOnMap_C();
		GetGame().GetCallqueue().Remove(Sample_S);
		GetGame().GetCallqueue().Remove(FlushSync_S);
		GetGame().GetCallqueue().Remove(UpdateOnMap_C);

		super.OnDelete(owner);
	}

	protected void SubscribeToState()
	{
		PS_GameModeCoop mode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (mode)
		{
			mode.GetOnGameStateChange().Insert(OnLobbyStateChanged_S);
			if (Replication.IsServer() && mode.GetState() == SCR_EGameModeState.GAME)
				GoLive_S();
		}

		if (RplSession.Mode() != RplMode.Dedicated)
		{
			PreloadBeep_C();
			HookMapEntity_C();
		}
	}

	protected void OnLobbyStateChanged_S(int state)
	{
		if (!Replication.IsServer())
			return;

		SCR_EGameModeState s = state;
		if (s == SCR_EGameModeState.GAME)
			GoLive_S();
		else
			GoDark_S();
	}

	protected void GoLive_S()
	{
		if (m_bLive)
			return;
		m_bLive = true;

		array<PS_TrackerComponent> trackers = PS_TrackerComponent.GetAll();
		if (trackers)
		{
			foreach (PS_TrackerComponent comp : trackers)
			{
				if (comp)
					AddTracker_S(comp);
			}
		}

		QueueSync_S();
		Sample_S();

		GetGame().GetCallqueue().CallLater(Sample_S, BASE_TICK_MS, true);
	}

	protected void GoDark_S()
	{
		if (!m_bLive)
			return;
		m_bLive = false;

		GetGame().GetCallqueue().Remove(Sample_S);
		m_aRuntime.Clear();
		m_aTrackers.Clear();
		m_iVersion++;
		QueueSync_S();
	}

	void OnTrackerAdded(notnull PS_TrackerComponent comp)
	{
		if (!Replication.IsServer() || !m_bLive)
			return;

		if (FindRuntime_S(comp))
			return;

		AddTracker_S(comp);
		QueueSync_S();

		PS_TrackerRuntime runtime = FindRuntime_S(comp);
		array<int> ids = {};
		array<int> posData = {};
		if (runtime && !SampleOne_S(runtime, ids, posData))
			RetireTracker_S(runtime);
	}

	void OnTrackerRemoved(notnull PS_TrackerComponent comp)
	{
		if (!Replication.IsServer())
			return;

		PS_TrackerRuntime runtime = FindRuntime_S(comp);
		if (!runtime)
			return;

		RetireTracker_S(runtime);
	}

	protected void AddTracker_S(notnull PS_TrackerComponent comp)
	{
		m_iNextId++;

		PS_TrackerData data = new PS_TrackerData();
		data.m_iId = m_iNextId;
		comp.ExportConfig(data);

		PS_TrackerRuntime runtime = new PS_TrackerRuntime();
		runtime.m_Component = comp;
		runtime.m_Data = data;
		runtime.m_fInterval = Math.Max(comp.GetUpdateInterval(), BASE_TICK_S);
		runtime.m_fBeaconInterval = Math.Max(comp.GetBeaconInterval(), BASE_TICK_S);

		m_aTrackers.Insert(data);
		m_aRuntime.Insert(runtime);
		m_iVersion++;
	}

	protected void RetireTracker_S(notnull PS_TrackerRuntime runtime)
	{
		PS_TrackerData data = runtime.m_Data;
		m_aRuntime.RemoveItem(runtime);

		if (data.m_bKeepLastPosition && data.m_bHasPos)
			data.m_bStale = true;
		else
			m_aTrackers.RemoveItem(data);

		m_iVersion++;
		QueueSync_S();
	}

	protected PS_TrackerRuntime FindRuntime_S(PS_TrackerComponent comp)
	{
		foreach (PS_TrackerRuntime runtime : m_aRuntime)
		{
			if (runtime.m_Component == comp)
				return runtime;
		}
		return null;
	}

	protected void Sample_S()
	{
		if (!m_bLive)
			return;

		array<int> ids = {};
		array<int> posData = {};
		array<int> beepIds = {};
		array<int> beepPos = {};
		array<ref PS_TrackerRuntime> retire = {};

		foreach (PS_TrackerRuntime runtime : m_aRuntime)
		{
			if (!AccumulateBeacon_S(runtime, beepIds, beepPos))
			{
				retire.Insert(runtime);
				continue;
			}

			runtime.m_fAccum += BASE_TICK_S;

			if (runtime.m_bEverSent && runtime.m_fAccum + 0.001 < runtime.m_fInterval)
				continue;
			runtime.m_fAccum = 0;

			if (!SampleOne_S(runtime, ids, posData))
				retire.Insert(runtime);
		}

		foreach (PS_TrackerRuntime dead : retire)
		{
			RetireTracker_S(dead);
		}

		if (!ids.IsEmpty() && !m_bSyncQueued)
			Rpc(RpcDo_SetPositions, ids, posData);

		if (!beepIds.IsEmpty())
		{
			Rpc(RpcDo_Beep, beepIds, beepPos);

			if (RplSession.Mode() != RplMode.Dedicated)
				RpcDo_Beep(beepIds, beepPos);
		}
	}

	protected bool AccumulateBeacon_S(notnull PS_TrackerRuntime runtime, notnull array<int> beepIds, notnull array<int> beepPos)
	{
		if (!runtime.m_Data.m_bBeacon)
			return true;

		runtime.m_fBeaconAccum += BASE_TICK_S;
		if (runtime.m_fBeaconAccum + 0.001 < runtime.m_fBeaconInterval)
			return true;
		runtime.m_fBeaconAccum = 0;

		PS_TrackerComponent comp = runtime.m_Component;
		vector pos;
		float yaw;
		if (!comp || !comp.SampleTarget(pos, yaw))
			return false;

		int x = Math.Round(pos[0]);
		int y = Math.Round(pos[1]);
		int z = Math.Round(pos[2]);

		beepIds.Insert(runtime.m_Data.m_iId);
		beepPos.Insert(x);
		beepPos.Insert(y);
		beepPos.Insert(z);
		return true;
	}

	protected bool SampleOne_S(notnull PS_TrackerRuntime runtime, notnull array<int> ids, notnull array<int> posData)
	{
		PS_TrackerComponent comp = runtime.m_Component;
		vector pos;
		float yaw;
		if (!comp || !comp.SampleTarget(pos, yaw))
			return false;

		FactionKey factionKey = comp.GetTargetFactionKey();
		if (factionKey != runtime.m_Data.m_sFactionKey)
		{
			runtime.m_Data.m_sFactionKey = factionKey;
			m_iVersion++;
			QueueSync_S();
		}

		int x = Math.Round(pos[0]);
		int z = Math.Round(pos[2]);
		int yawI = Math.Round(yaw);

		runtime.m_Data.m_vPos = Vector(pos[0], 0, pos[2]);
		runtime.m_Data.m_fYaw = yaw;
		runtime.m_Data.m_bHasPos = true;

		if (runtime.m_bEverSent)
		{
			bool moved = Math.AbsInt(x - runtime.m_iSentX) >= POS_EPSILON || Math.AbsInt(z - runtime.m_iSentZ) >= POS_EPSILON;
			bool turned = runtime.m_Data.m_bShowHeading && Math.AbsInt(yawI - runtime.m_iSentYaw) >= YAW_EPSILON;
			if (!moved && !turned)
				return true;
		}

		runtime.m_iSentX = x;
		runtime.m_iSentZ = z;
		runtime.m_iSentYaw = yawI;
		runtime.m_bEverSent = true;

		ids.Insert(runtime.m_Data.m_iId);
		posData.Insert(x);
		posData.Insert(z);
		posData.Insert(yawI);
		return true;
	}

	protected void QueueSync_S()
	{
		if (m_bSyncQueued)
			return;
		m_bSyncQueued = true;
		GetGame().GetCallqueue().CallLater(FlushSync_S, 0, false);
	}

	protected void FlushSync_S()
	{
		m_bSyncQueued = false;

		if (!Replication.IsServer() || !GetOwner())
			return;

		array<string> strings = {};
		array<int> ints = {};
		array<float> floats = {};
		PackState(strings, ints, floats);
		Rpc(RpcDo_SyncTrackers, strings, ints, floats);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SyncTrackers(array<string> strings, array<int> ints, array<float> floats)
	{
		ApplyState(strings, ints, floats);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPositions(array<int> ids, array<int> posData)
	{
		for (int i = 0, count = ids.Count(); i < count; i++)
		{
			PS_TrackerData data = FindById(ids[i]);
			if (!data)
				continue;

			data.m_vPos = Vector(posData[i * 3], 0, posData[i * 3 + 1]);
			data.m_fYaw = posData[i * 3 + 2];
			data.m_bHasPos = true;
		}
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RpcDo_Beep(array<int> ids, array<int> beepPos)
	{
		IEntity listener = GetLocalListener_C();
		if (!listener)
			return;

		vector listenerPos = listener.GetOrigin();

		for (int i = 0, count = ids.Count(); i < count; i++)
		{
			PS_TrackerData data = FindById(ids[i]);
			if (!data || data.m_fBeaconRange <= 0)
				continue;

			vector pos = Vector(beepPos[i * 3], beepPos[i * 3 + 1], beepPos[i * 3 + 2]);
			if (vector.DistanceSq(pos, listenerPos) > data.m_fBeaconRange * data.m_fBeaconRange)
				continue;

			PlayBeep_C(pos);
		}
	}

	protected void PlayBeep_C(vector pos)
	{
		PreloadBeep_C();

		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = pos;
		AudioSystem.PlayEvent(BEEP_PROJECT, BEEP_EVENT, mat);
	}

	protected void PreloadBeep_C()
	{
		if (m_bBeepProjectLoaded)
			return;

		m_bBeepProjectLoaded = true;
		AudioSystem.PlayEventInitialize(BEEP_PROJECT);
	}

	protected IEntity GetLocalListener_C()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		return pc.GetControlledEntity();
	}

	protected PS_TrackerData FindById(int id)
	{
		foreach (PS_TrackerData data : m_aTrackers)
		{
			if (data.m_iId == id)
				return data;
		}
		return null;
	}

	protected void PackState(notnull array<string> strings, notnull array<int> ints, notnull array<float> floats)
	{
		foreach (PS_TrackerData data : m_aTrackers)
		{
			strings.Insert(data.m_sImageSet);
			strings.Insert(data.m_sQuad);
			strings.Insert(data.m_sLabel);
			strings.Insert(data.m_sFactionKey);
			strings.Insert(data.m_sVisibleFactions);

			ints.Insert(data.m_iId);
			ints.Insert(data.m_iZOrder);
			ints.Insert(PackFlags(data));

			floats.Insert(data.m_fColorR);
			floats.Insert(data.m_fColorG);
			floats.Insert(data.m_fColorB);
			floats.Insert(data.m_fColorA);
			floats.Insert(data.m_fWorldSize);
			floats.Insert(data.m_fMinSize);
			floats.Insert(data.m_fBeaconRange);
			floats.Insert(data.m_vPos[0]);
			floats.Insert(data.m_vPos[2]);
			floats.Insert(data.m_fYaw);
		}
	}

	protected void ApplyState(array<string> strings, array<int> ints, array<float> floats)
	{
		m_aTrackers = {};

		int count = ints.Count() / STRIDE_INTS;
		for (int i = 0; i < count; i++)
		{
			int s = i * STRIDE_STRINGS;
			int n = i * STRIDE_INTS;
			int f = i * STRIDE_FLOATS;

			PS_TrackerData data = new PS_TrackerData();
			data.m_sImageSet = strings[s];
			data.m_sQuad = strings[s + 1];
			data.m_sLabel = strings[s + 2];
			data.m_sFactionKey = strings[s + 3];
			data.m_sVisibleFactions = strings[s + 4];

			data.m_iId = ints[n];
			data.m_iZOrder = ints[n + 1];
			UnpackFlags(data, ints[n + 2]);

			data.m_fColorR = floats[f];
			data.m_fColorG = floats[f + 1];
			data.m_fColorB = floats[f + 2];
			data.m_fColorA = floats[f + 3];
			data.m_fWorldSize = floats[f + 4];
			data.m_fMinSize = floats[f + 5];
			data.m_fBeaconRange = floats[f + 6];
			data.m_vPos = Vector(floats[f + 7], 0, floats[f + 8]);
			data.m_fYaw = floats[f + 9];

			m_aTrackers.Insert(data);
		}

		m_iVersion++;
	}

	protected int PackFlags(notnull PS_TrackerData data)
	{
		int flags;
		if (data.m_bAnyFaction)
			flags |= 1;
		if (data.m_bUseWorldScale)
			flags |= 2;
		if (data.m_bShowHeading)
			flags |= 4;
		if (data.m_bUseFactionColor)
			flags |= 8;
		if (data.m_bStale)
			flags |= 16;
		if (data.m_bHasPos)
			flags |= 32;
		if (data.m_bBeacon)
			flags |= 64;
		return flags;
	}

	protected void UnpackFlags(notnull PS_TrackerData data, int flags)
	{
		data.m_bAnyFaction = (flags & 1) != 0;
		data.m_bUseWorldScale = (flags & 2) != 0;
		data.m_bShowHeading = (flags & 4) != 0;
		data.m_bUseFactionColor = (flags & 8) != 0;
		data.m_bStale = (flags & 16) != 0;
		data.m_bHasPos = (flags & 32) != 0;
		data.m_bBeacon = (flags & 64) != 0;
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		array<string> strings = {};
		array<int> ints = {};
		array<float> floats = {};
		PackState(strings, ints, floats);

		writer.WriteInt(ints.Count() / STRIDE_INTS);
		foreach (string s : strings)
		{
			writer.WriteString(s);
		}
		foreach (int i : ints)
		{
			writer.WriteInt(i);
		}
		foreach (float f : floats)
		{
			writer.WriteFloat(f);
		}
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		int count;
		reader.ReadInt(count);

		array<string> strings = {};
		array<int> ints = {};
		array<float> floats = {};

		for (int i = 0; i < count * STRIDE_STRINGS; i++)
		{
			string s;
			reader.ReadString(s);
			strings.Insert(s);
		}
		for (int i = 0; i < count * STRIDE_INTS; i++)
		{
			int v;
			reader.ReadInt(v);
			ints.Insert(v);
		}
		for (int i = 0; i < count * STRIDE_FLOATS; i++)
		{
			float f;
			reader.ReadFloat(f);
			floats.Insert(f);
		}

		ApplyState(strings, ints, floats);
		return true;
	}

	// ---------------------------------------------------------------------------------------------
	// Client map hooks & dynamic rendering
	// ---------------------------------------------------------------------------------------------

	protected void HookMapEntity_C()
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity)
		{
			GetGame().GetCallqueue().CallLater(HookMapEntity_C, 100);
			return;
		}

		m_MapEntity.GetOnMapOpen().Insert(OnMapOpen_C);
		m_MapEntity.GetOnMapClose().Insert(OnMapClose_C);

		if (m_MapEntity.IsOpen())
			OnMapOpen_C(null);
	}

	protected void UnhookMapEntity_C()
	{
		if (m_MapEntity)
		{
			m_MapEntity.GetOnMapOpen().Remove(OnMapOpen_C);
			m_MapEntity.GetOnMapClose().Remove(OnMapClose_C);
			m_MapEntity = null;
		}
	}

	protected void OnMapOpen_C(MapConfiguration config)
	{
		m_bMapOpen = true;
		OpenOnMap_C(m_MapEntity);
		GetGame().GetCallqueue().CallLater(UpdateOnMap_C, 0, true);
	}

	protected void OnMapClose_C(MapConfiguration config)
	{
		m_bMapOpen = false;
		GetGame().GetCallqueue().Remove(UpdateOnMap_C);
		CloseOnMap_C();
	}

	void OpenOnMap_C(SCR_MapEntity mapEnt)
	{
		CloseOnMap_C();
		if (!mapEnt)
			return;

		Widget mapMenuRoot = mapEnt.GetMapMenuRoot();
		if (!mapMenuRoot)
			return;

		Widget mapFrame = mapMenuRoot.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			mapFrame = mapMenuRoot;
		if (!mapFrame)
			return;

		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		WorkspaceWidget workspace = GetGame().GetWorkspace();

		foreach (PS_TrackerData data : m_aTrackers)
		{
			Widget w = workspace.CreateWidgets(MARKER_LAYOUT, mapFrame);
			if (!w)
				continue;

			PS_ManualMarkerComponent handler = PS_ManualMarkerComponent.Cast(w.FindHandler(PS_ManualMarkerComponent));
			if (!handler)
			{
				w.RemoveFromHierarchy();
				continue;
			}

			Color color = new Color(data.m_fColorR, data.m_fColorG, data.m_fColorB, data.m_fColorA);
			if (data.m_bUseFactionColor && factionMgr && data.m_sFactionKey != "")
			{
				SCR_Faction faction = SCR_Faction.Cast(factionMgr.GetFactionByKey(data.m_sFactionKey));
				if (faction)
					color = faction.GetFactionColor();
			}

			w.SetVisible(false);
			w.SetZOrder(data.m_iZOrder);
			handler.SetImage(data.m_sImageSet, data.m_sQuad);
			handler.SetDescription(data.m_sLabel);
			handler.SetColor(color);
			handler.SetOpacity(1.0);
			handler.OnMouseLeave(null, null, 0, 0);

			m_aWidgets.Insert(w);
			m_aRenderData.Insert(data);
		}

		m_iBuiltVersion = m_iVersion;
	}

	void UpdateOnMap_C()
	{
		if (!m_bMapOpen || !m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (m_iVersion != m_iBuiltVersion)
			OpenOnMap_C(m_MapEntity);

		FactionKey localFaction = GetLocalFactionKey_C();

		for (int i = 0, count = m_aWidgets.Count(); i < count; i++)
		{
			Widget w = m_aWidgets[i];
			PS_TrackerData data = m_aRenderData[i];
			if (!w || !data)
				continue;

			bool show = data.m_bHasPos && IsVisibleFor_C(data, localFaction);
			w.SetVisible(show);
			if (!show)
				continue;

			PS_ManualMarkerComponent handler = PS_ManualMarkerComponent.Cast(w.FindHandler(PS_ManualMarkerComponent));
			if (!handler)
				continue;

			vector rotation = Vector(90, 0, 0);
			if (data.m_bShowHeading)
				rotation = Vector(data.m_fYaw, 0, 0);

			handler.SetSlotWorld(data.m_vPos, rotation, data.m_fWorldSize, data.m_bUseWorldScale, data.m_fMinSize);

			if (data.m_bStale)
				handler.SetOpacity(STALE_OPACITY);
			else
				handler.SetOpacity(1.0);
		}
	}

	void CloseOnMap_C()
	{
		foreach (Widget w : m_aWidgets)
		{
			if (w)
				w.RemoveFromHierarchy();
		}
		m_aWidgets.Clear();
		m_aRenderData.Clear();
		m_iBuiltVersion = -1;
	}

	protected bool IsVisibleFor_C(notnull PS_TrackerData data, FactionKey factionKey)
	{
		if (data.m_bAnyFaction)
			return true;

		if (factionKey == "" || data.m_sVisibleFactions == "")
			return false;

		array<string> allowed = {};
		data.m_sVisibleFactions.Split(",", allowed, true);
		return allowed.Contains(factionKey);
	}

	protected FactionKey GetLocalFactionKey_C()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return "";

		SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(
			pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (affiliation)
		{
			Faction faction = affiliation.GetAffiliatedFaction();
			if (faction)
				return faction.GetFactionKey();
		}

		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		if (pm)
		{
			FactionKey key = pm.GetPlayerFactionKey(pc.GetPlayerId());
			if (key != "")
				return key;
		}

		return "";
	}
}
