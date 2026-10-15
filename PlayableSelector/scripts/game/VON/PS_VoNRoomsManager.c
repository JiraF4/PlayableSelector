//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{

};

// just string but funnier
// struct: [FactionKey + "|"] + roomName
typedef string VoNRoomKey;

// Manage VON_DIRECT rooms for menu speakers.
//
// A channel key identifies a shared virtual position. Menu speech uses VON_DIRECT; proxy radios stay muted.

class PS_VoNRoomsManager : ScriptComponent
{
	// Replicated channel state
	ref array<string> m_aChannels = {};                                    // channel keys that exist (lazy)
	protected ref map<string, bool> m_mChannelsSet = new map<string, bool>(); // fast existence lookup
	ref map<int, string> m_mPlayersChannel = new map<int, string>();       // playerId -> channelKey

	// Move speech bois to space (kept only as the parked-body anchor for SetVoNPosition compatibility)
	static vector roomInitialPosition = "-1 1000000 1";

	// Invokers
	// (playerId, channelKey, oldChannelKey)
	ref ScriptInvoker m_eOnRoomChanged = new ScriptInvoker();

	// ---- Body-less VoN proxy (per-player tiny replicated entity carrying the radio + VoN) ----
	[Attribute("", UIWidgets.ResourceNamePicker, "Per-player VoN proxy prefab (PS_VoNProxyComponent + SCR_VoNComponent + muted BaseRadioComponent, RplComponent streaming disabled)", "et")]
	protected ResourceName m_sVoNProxyPrefab;
	protected ref map<int, IEntity> m_mProxies_S = new map<int, IEntity>(); // server-side, for cleanup
	protected ref map<int, int> m_mProxySlots_S = new map<int, int>();      // server-side, grid slot per player
	protected ref array<int> m_aPendingRadioMutes = {};                     // deferred (1 frame) mute queue
	protected bool m_bIsGamePhase = false; // During GAME, skip menu-inactive proxies in the mute audit.

	// FIX (TIMER LEAK): The self-rescheduling VoNAuditTick (every 5s) lives on the global
	// callqueue. On server restart the component is destroyed with its GameMode entity, but
	// the callqueue holds a strong reference — preventing GC and accumulating stale ticks.
	// OnDelete fires deterministically when the entity is destroyed (unlike a destructor which
	// would be delayed until GC, which can't run while the callqueue still holds the reference).
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		GetGame().GetCallqueue().Remove(VoNAuditTick);
	}

	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}

	override protected void OnPostInit(IEntity owner)
	{
		// The "" (global lobby) channel always exists
		RegisterChannelLocal("");
		if (Replication.IsServer())
			m_bRplLoaded = true;
		// Re-assert mute on proxy radios on each machine.
		GetGame().GetCallqueue().CallLater(VoNAuditTick, 5000, false);
	}

	// more singletons for singletons god, make our spagetie kingdom great
	static PS_VoNRoomsManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_VoNRoomsManager.Cast(gameMode.FindComponent(PS_VoNRoomsManager));
		else
			return null;
	}

	// [PS_VoNDBG] per-channel-change diagnostic toggle. Default false for production.
	static bool s_bVoNDebug = false;

	protected static const float PS_VONFIX_SKY_ALTITUDE    = 10000;
	protected static const float PS_VONFIX_CHANNEL_BASE    = 1000;
	protected static const float PS_VONFIX_CHANNEL_SPACING = 1000;
	protected static const float PS_VONFIX_MEMBER_STEP     = 0.4;
	protected static const int   PS_VONFIX_GRID_COLS       = 12;
	protected static const int   PS_VONFIX_TOTAL_SLOTS     = 144;
	protected static const float PS_VONFIX_ISOLATED_XZ     = -5000;
	protected static const float PS_VONFIX_NO_CHANNEL_XZ   = -6000;

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Расчет трехмерных виртуальных координат игрока в сетке голосовой комнаты на 128 игроков.
	 * @param[in] playerId ID игрока
	 * @return Вектор трехмерных мировых координат
	 */
	vector GetRoomPosition(int playerId)
	{
		// Controls a living character / not a menu speaker -> isolated far spot
		// Safe: the proxy radio is muted on every machine.
		if (!SCR_VoNComponent.PS_IsMenuSpeaker(playerId))
			return Vector(PS_VONFIX_ISOLATED_XZ, PS_VONFIX_SKY_ALTITUDE, PS_VONFIX_ISOLATED_XZ);

		string channelKey;
		if (!m_mPlayersChannel.Find(playerId, channelKey))
			return Vector(PS_VONFIX_NO_CHANNEL_XZ, PS_VONFIX_SKY_ALTITUDE, PS_VONFIX_NO_CHANNEL_XZ);

		int index = m_aChannels.Find(channelKey);
		if (index < 0)
			return Vector(PS_VONFIX_NO_CHANNEL_XZ, PS_VONFIX_SKY_ALTITUDE, PS_VONFIX_NO_CHANNEL_XZ);

		// Deterministic collision-free slot allocation within the room (12x12 grid = 144 slots >= 128 players).
		// Rank playerId against other players currently in the same channelKey.
		// Both client and server replicate m_mPlayersChannel, so this guarantees 100% identical coordinates
		// on all machines without any slot collisions even when server playerIds grow beyond 256.
		int slot = 0;
		for (int i = 0; i < m_mPlayersChannel.Count(); i++)
		{
			int otherId = m_mPlayersChannel.GetKey(i);
			if (otherId < playerId && m_mPlayersChannel.GetElement(i) == channelKey)
				slot++;
		}

		if (slot >= PS_VONFIX_TOTAL_SLOTS)
			slot = PS_VONFIX_TOTAL_SLOTS - 1;

		int gridX = slot % PS_VONFIX_GRID_COLS;
		int gridZ = slot / PS_VONFIX_GRID_COLS;

		// Centered 12x12 grid: (12 - 1) / 2 = 5.5
		vector memberOffset = Vector(
			(gridX - 5.5) * PS_VONFIX_MEMBER_STEP,
			0,
			(gridZ - 5.5) * PS_VONFIX_MEMBER_STEP
		);

		return Vector(PS_VONFIX_CHANNEL_BASE + PS_VONFIX_CHANNEL_SPACING * index, PS_VONFIX_SKY_ALTITUDE, PS_VONFIX_CHANNEL_BASE) + memberOffset;
	}

	// ------------------------- Channel keys -------------------------
	// Channel key format preserved from the old room keys: factionKey + "|" + roomName ("" == global)
	static string MakeChannelKey(FactionKey factionKey, string roomName)
	{
		string channelKey = factionKey + "|" + roomName;
		if (channelKey == "|")
			channelKey = "";
		return channelKey;
	}

	// True if channelKey is a FACTION-SCOPED room (Command / Faction / group) whose faction prefix differs from
	// expectedFaction. The factionless pools (Global / Local / Public / "") never count. Used by the server VoN
	// reconcile safety-net to detect ONLY a player whose audio drifted onto another faction's channel, while
	// leaving legitimate same-faction manual room choices and the factionless pools untouched.
	bool IsForeignFactionChannel(string channelKey, FactionKey expectedFaction)
	{
		int sep = channelKey.IndexOf("|");
		if (sep < 0)
			return false; // "" or malformed - treat as factionless
		string faction = channelKey.Substring(0, sep);
		string roomName = channelKey.Substring(sep + 1, channelKey.Length() - sep - 1);
		if (roomName == ""
			|| roomName.StartsWith("#PS-VoNRoom_Global")
			|| roomName.StartsWith("#PS-VoNRoom_Local")
			|| roomName.StartsWith("#PS-VoNRoom_Public"))
			return false;
		return faction != expectedFaction;
	}

	// ------------------------- Channel changing -------------------------
	// Move to channel by faction/room name, creating the channel if new. RUN ONLY ON SERVER
	void MoveToRoom(int playerId, FactionKey factionKey, string roomName)
	{
		if (!Replication.IsServer())
			return;

		// SAFETY NET against the cross-faction VoN leak. A FACTION-SCOPED room (Command, Faction, or a
		// group channel) built with the WRONG faction key assigns a player into another faction's channel.
		// This also merges rooms across factions (e.g. both sides' HQ, or groups with the same id, into one
		// channel). The guard catches both EMPTY factionKey (caller passed "" when the slot has a faction)
		// and WRONG factionKey (stale/swapped). The authoritative source is always the SLOT's faction key
		// (via GetSlotFactionForPlayer) — the group faction is deliberately excluded because TvT setups
		// reuse group IDs across factions, making FindGroup return a group from the wrong side.
		// The faction-LESS rooms (Global / Local / Public / "") legitimately use "". For anything else,
		// force the faction to match the slot. This is the single choke point every room move funnels
		// through (briefing loop, lobby UI, admin "move player", JIP/reconnect).
		if (roomName != ""
			&& !roomName.StartsWith("#PS-VoNRoom_Global")
			&& !roomName.StartsWith("#PS-VoNRoom_Local")
			&& !roomName.StartsWith("#PS-VoNRoom_Public"))
		{
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
			{				FactionKey authoritativeFaction = playableManager.GetSlotFactionForPlayer(playerId);
				if (authoritativeFaction != "" && authoritativeFaction != factionKey)
					{
						// Only warn when the caller passed a WRONG faction (e.g. stale data from RPC_MoveVoNToRoom).
						// An empty factionKey means "derive from slot" (the new AssignPhaseVoiceChannel pattern) -
						// the correction is expected and silent.
						if (factionKey != "")
							Print(string.Format("[PS_VoN] MoveToRoom guard: player %1 had faction '%2' for room '%3' - corrected to '%4' to prevent cross-faction channel leak", playerId, factionKey, roomName, authoritativeFaction), LogLevel.WARNING);
						factionKey = authoritativeFaction;
					}
			}
		}

		string channelKey = MakeChannelKey(factionKey, roomName);
		InitChannelIfNeeded(channelKey);

		// Skip if already in this channel
		if (channelKey == GetPlayerChannel(playerId))
			return;

		// The channel determines the proxy's virtual position for direct voice.

		// Finally move player to channel
		RPC_SetPlayerChannel(playerId, channelKey);
		Rpc(RPC_SetPlayerChannel, playerId, channelKey);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerChannel(int playerId, string channelKey)
	{
		PS_NetStat.Hit("RPC_SetPlayerChannel");
		string oldChannelKey = GetPlayerChannel(playerId);

		RegisterChannelLocal(channelKey); // ensure the channel exists on every client
		m_mPlayersChannel[playerId] = channelKey;

		// TEMP DIAGNOSTIC (cross-group voice leak): log the channel + this player's group id & callsign, so the
		// client log shows whether two DIFFERENT groups (different groupId) resolve to the SAME channel key.
		// Gated behind s_bVoNDebug (default OFF): it runs on EVERY machine on EVERY channel change (a burst during
		// the briefing mass-assignment), so the lookups + PrintFormat cost nothing in production. Flip s_bVoNDebug
		// to re-enable for live cross-group/faction verification.
		if (s_bVoNDebug)
		{
			PS_PlayableManager pmDbg = PS_PlayableManager.GetInstance();
			if (pmDbg)
			{
				RplId playableDbg = pmDbg.GetPlayableByPlayer(playerId);
				SCR_AIGroup grpDbg = pmDbg.GetPlayerGroupByPlayable(playableDbg);
				int gidDbg = -1;
				if (grpDbg)
					gidDbg = grpDbg.GetGroupID();
				PrintFormat("[PS_VoNDBG] player=%1 channel='%2' groupId=%3 callsign=%4", playerId, channelKey, gidDbg, pmDbg.GetGroupCallsignByPlayable(playableDbg));
			}
		}

		MuteProxyRadio(playerId);

		SCR_VoNComponent.InvalidateEditorLocCache(-1);
		m_eOnRoomChanged.Invoke(playerId, channelKey, oldChannelKey);
	}

	// Re-apply a player's current channel (e.g. after they respawn into a new body)
	void RestoreRoom(int playerId)
	{
		string channelKey = GetPlayerChannel(playerId);
		RPC_SetPlayerChannel(playerId, channelKey);
		Rpc(RPC_SetPlayerChannel, playerId, channelKey);
	}

	// ============================ Body-less VoN proxy ============================
	// Per-player replicated VoN endpoint for menu speakers. Its radio component stays muted;
	// VON_DIRECT routes speech through the proxy's virtual position for the selected room.

	// Server: spawn this player's proxy, or RE-BIND an existing one on reconnect.
	// Reconnect keeps the SAME playerId but gives a NEW connection (RplIdentity). The existing proxy is
	// still rpl.Give'n to the dead OLD connection, so the reconnected client can no longer drive capture
	// on it = nobody hears that player (the reported "can't hear Banan after his first reconnect" bug).
	// We also can't rely on RemoveProxy_S running on disconnect: the live server's QuickTvT gamemode
	// overrides OnPlayerDisconnected and does not run our cleanup, so the proxy persists. So instead of
	// skipping when a proxy exists, re-bind it (re-Give + re-register + re-mute) to the new connection.
	void SpawnProxy_S(int playerId)
	{
		if (!Replication.IsServer())
			return;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
		{
			// Controller can lag behind the connect callback - try again.
			GetGame().GetCallqueue().CallLater(SpawnProxy_S, 500, false, playerId);
			return;
		}

		IEntity proxy;
		bool reusing = m_mProxies_S.Find(playerId, proxy) && proxy;
		if (!reusing)
		{
			// Use the operator-assigned attribute if set, else the registered proxy prefab GUID. The
			// hardcoded fallback means this works regardless of which game-mode prefab the server loads
			// (the QuickTvT gamemode that extends this lobby makes attribute assignment ambiguous).
			ResourceName proxyPrefab = m_sVoNProxyPrefab;
			if (proxyPrefab == "")
				proxyPrefab = "{8DE2B66B89E14F88}Prefabs/VoN/PS_VoNProxy.et";

			// Keep proxy entities spatially separated; room membership is routed by virtual position,
			// not by radio frequency or encryption key.
			int slot = AllocateProxySlot_S(playerId);
			EntitySpawnParams params = new EntitySpawnParams();
			Math3D.MatrixIdentity4(params.Transform);
			params.Transform[3] = Vector(200 * Math.Mod(slot, 16), 10000, 200 * Math.Floor(slot / 16));

			proxy = GetGame().SpawnEntityPrefab(Resource.Load(proxyPrefab), GetGame().GetWorld(), params);
			if (!proxy)
			{
				Print("[PS_VoN] Failed to spawn VoN proxy prefab", LogLevel.ERROR);
				return;
			}

			PS_VoNProxyComponent proxyComp = PS_VoNProxyComponent.Cast(proxy.FindComponent(PS_VoNProxyComponent));
			if (proxyComp)
				proxyComp.SetPlayerId_S(playerId);
			else
				Print("[PS_VoN] VoN proxy prefab is missing PS_VoNProxyComponent", LogLevel.ERROR);

			m_mProxies_S.Set(playerId, proxy);
		}

		// Ownership: the owning client must be allowed to drive capture. ALWAYS (re-)Give to the current
		// connection - on reconnect this transfers the proxy off the dead old connection onto the new one.
		RplIdentity playerRplID = pc.GetRplIdentity();
		if (playerRplID != RplIdentity.Local())
		{
			RplComponent rpl = RplComponent.Cast(proxy.FindComponent(RplComponent));
			if (rpl)
				rpl.Give(playerRplID);
		}

		// The radio gadget wrapper does not init itself on a non-character entity - force it.
		SCR_RadioComponent radioGadget = SCR_RadioComponent.Cast(proxy.FindComponent(SCR_RadioComponent));
		if (radioGadget)
			radioGadget.OnPostInit(proxy);

		// Register the endpoint with the SERVER's VoN system, or the server computes an empty delivery set
		// (transmissions arrive but reach nobody). Re-done on reconnect: the per-player editor manager is
		// recreated, so the old registration is stale.
		SCR_VoNComponent vonComp = SCR_VoNComponent.Cast(proxy.FindComponent(SCR_VoNComponent));
		if (vonComp)
			vonComp.ConnectEditorToVoNSystem(playerId);

		if (reusing)
			Print(string.Format("[PS_VoN] VoN proxy re-bound to reconnected player %1", playerId), LogLevel.NORMAL);
		else
			Print(string.Format("[PS_VoN] VoN proxy spawned for player %1 at %2", playerId, proxy.GetOrigin().ToString()), LogLevel.NORMAL);

		MuteProxyRadio(playerId); // mute the proxy radio after spawn or reconnect

		// Guard check on server for 1.8 transceiver badge
		IEntity proxyEntity;
		if (m_mProxies_S.Find(playerId, proxyEntity) && proxyEntity)
		{
			BaseRadioComponent r = BaseRadioComponent.Cast(proxyEntity.FindComponent(BaseRadioComponent));
			if (!r || r.TransceiversCount() < 1 || !EditorFactionTransceiver.Cast(r.GetTransceiver(0)))
			{
				Print(string.Format("[PS_VoN] WARNING: VoN proxy radio for player %1 has no EditorFactionTransceiver - menu voice will be SILENT on 1.8+ (transmit works, delivery never happens).", playerId), LogLevel.ERROR);
			}
		}
	}

	// Server: delete this player's proxy (on disconnect).
	void RemoveProxy_S(int playerId)
	{
		if (!Replication.IsServer())
			return;
		IEntity proxy;
		if (m_mProxies_S.Find(playerId, proxy) && proxy)
			SCR_EntityHelper.DeleteEntityAndChildren(proxy);
		m_mProxies_S.Remove(playerId);
		m_mProxySlots_S.Remove(playerId);

		SCR_VoNComponent.InvalidateEditorLocCache(-1);

		// FIX (GHOST NAMES): remove the player from the replicated channel map so late-joining
		// clients (JIP) no longer receive stale entries in their RplLoad snapshot. Without this,
		// m_mPlayersChannel accumulates entries for every player who ever connected, and the
		// voice-chat UI (GetPlayersInRoom) displays ghost names for long-disconnected players.
		RemovePlayerFromChannel(playerId);
	}

	// Smallest free grid slot (slots are reused as players leave - ids grow forever).
	protected int AllocateProxySlot_S(int playerId)
	{
		int existing;
		if (m_mProxySlots_S.Find(playerId, existing))
			return existing;

		int slot = 0;
		while (true)
		{
			bool taken = false;
			foreach (int pid, int s : m_mProxySlots_S)
			{
				if (s == slot)
				{
					taken = true;
					break;
				}
			}
			if (!taken)
				break;
			slot++;
		}
		m_mProxySlots_S.Set(playerId, slot);
		return slot;
	}

	// Mute each replicated proxy radio on every machine. Same-frame updates are coalesced
	// so the proxy is present before the transceiver state is applied.
	protected bool m_bProxyRadioMuteScheduled = false;
	void MuteProxyRadio(int playerId)
	{
		if (!m_aPendingRadioMutes.Contains(playerId))
			m_aPendingRadioMutes.Insert(playerId);
		if (!m_bProxyRadioMuteScheduled)
		{
			m_bProxyRadioMuteScheduled = true;
			GetGame().GetCallqueue().CallLater(MutePendingProxyRadios, 0, false);
		}
	}
	protected void MutePendingProxyRadios()
	{
		m_bProxyRadioMuteScheduled = false;
		foreach (int playerId : m_aPendingRadioMutes)
			MuteProxyRadioNow(playerId);
		m_aPendingRadioMutes.Clear();
	}
	// Set the GAME-phase flag so the audit can skip proxies not used for menu voice.
	void SetVoNGamePhase(bool isGame)
	{
		m_bIsGamePhase = isGame;
	}

	// Re-mute menu proxies if engine initialization changes their transceiver state.
	void VoNAuditTick()
	{
		GetGame().GetCallqueue().CallLater(VoNAuditTick, 5000, false);

		if (!Replication.IsServer())
		{
			array<int> connectedPlayers = {};
			GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
			array<int> staleIds = {};
			foreach (int pid, string channelKey : m_mPlayersChannel)
			{
				if (!connectedPlayers.Contains(pid))
					staleIds.Insert(pid);
			}
			foreach (int staleId : staleIds)
			{
				string staleKey = m_mPlayersChannel[staleId];
				Print(string.Format("[PS_VoN] VoNAuditTick: pruning stale channel entry for disconnected player %1 (channel '%2')", staleId, staleKey), LogLevel.NORMAL);
				m_mPlayersChannel.Remove(staleId);
				m_eOnRoomChanged.Invoke(staleId, "", staleKey);
			}
		}

		int audited = 0;
		int remuted = 0;
		foreach (int playerId, string channelKey : m_mPlayersChannel)
		{
			if (m_bIsGamePhase && !SCR_VoNComponent.PS_IsMenuSpeaker(playerId) && !IsLocalEditorOpenFor(playerId))
				continue;

			IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
			if (!proxy)
				continue;
			BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
			if (!radio || radio.TransceiversCount() < 1)
				continue;
			BaseTransceiver transceiver = radio.GetTransceiver(0);
			if (!transceiver)
				continue;
			audited++;

			if (!transceiver.IsMuted())
			{
				remuted++;
				MuteProxyRadioNow(playerId);
				if (s_bVoNDebug)
					PrintFormat("[PS_VoNAudit] proxy radio was unmuted; muted again player=%1 channel='%2'", playerId, channelKey);
			}
		}

		if (s_bVoNDebug && audited > 0)
			PrintFormat("[PS_VoNAudit] checked=%1 remuted=%2", audited, remuted);
	}
	protected void MuteProxyRadioNow(int playerId)
	{
		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
		if (!proxy)
			return;
		BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
		if (!radio || radio.TransceiversCount() < 1)
			return;
		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (transceiver)
			transceiver.SetMuteState(true);
	}
	// True only on the machine of a player whose OWN Game Master editor is open.
	protected bool IsLocalEditorOpenFor(int playerId)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() != playerId)
			return false;
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		return editorManager && editorManager.IsOpened();
	}

	// ------------------------- Player channel cleanup -------------------------
	// Remove a player from the replicated channel map on ALL machines and fire the
	// room-changed event so the voice-chat UI drops their widget. Called from RemoveProxy_S
	// (disconnect) and the client-side stale-entry prune (VoNAuditTick).
	void RemovePlayerFromChannel(int playerId)
	{
		if (!m_mPlayersChannel.Contains(playerId))
			return;
		string oldChannelKey = m_mPlayersChannel[playerId];
		RPC_RemovePlayerFromChannel(playerId, oldChannelKey);
		Rpc(RPC_RemovePlayerFromChannel, playerId, oldChannelKey);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_RemovePlayerFromChannel(int playerId, string oldChannelKey)
	{
		PS_NetStat.Hit("RPC_RemovePlayerFromChannel");
		if (!m_mPlayersChannel.Contains(playerId))
			return;
		m_mPlayersChannel.Remove(playerId);
		SCR_VoNComponent.InvalidateEditorLocCache(-1);
		m_eOnRoomChanged.Invoke(playerId, "", oldChannelKey);
	}

	// ------------------------- Channel creation -------------------------
	// Get (and lazily create) the channel key for a faction/room name. RUN ONLY ON SERVER
	string GetOrCreateRoomWithFaction(FactionKey factionKey, string roomName)
	{
		string channelKey = MakeChannelKey(factionKey, roomName);
		InitChannelIfNeeded(channelKey);
		return channelKey;
	}
	// Create the channel on all clients if it does not exist yet. RUN ONLY ON SERVER
	void InitChannelIfNeeded(string channelKey)
	{
		if (!Replication.IsServer())
			return;
		if (m_mChannelsSet.Contains(channelKey))
			return;
		RPC_CreateChannel(channelKey);
		Rpc(RPC_CreateChannel, channelKey);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_CreateChannel(string channelKey)
	{
		PS_NetStat.Hit("RPC_CreateChannel");
		RegisterChannelLocal(channelKey);
	}
	protected void RegisterChannelLocal(string channelKey)
	{
		if (m_mChannelsSet.Contains(channelKey))
			return;
		m_mChannelsSet[channelKey] = true;
		m_aChannels.Insert(channelKey);
	}

	// ------------------------- Get -------------------------
	string GetPlayerChannel(int playerId)
	{
		if (!m_mPlayersChannel.Contains(playerId))
			return "";
		return m_mPlayersChannel[playerId];
	}
	// Channel key for a faction/room name (deterministic - does not create)
	string GetRoomWithFaction(FactionKey factionKey, string roomName)
	{
		return MakeChannelKey(factionKey, roomName);
	}
	// The display name of a channel IS its key (kept for UI compatibility)
	string GetRoomName(string channelKey)
	{
		return channelKey;
	}

	void GetPlayersInRoom(out notnull array<int> players, string channelKey)
	{
		foreach (int playerId, string playerChannel : m_mPlayersChannel)
		{
			if (playerChannel == channelKey)
				players.Insert(playerId);
		}
	}

	void GetPlayersPublicChannels(out notnull array<string> channelKeys)
	{
		foreach (int playerId, string playerChannel : m_mPlayersChannel)
		{
			if (channelKeys.Contains(playerChannel))
				continue;
			if (IsPublicRoom(playerChannel))
				channelKeys.Insert(playerChannel);
		}
	}

	// ------------------------- Channel classification (by key) -------------------------
	bool IsPublicRoom(string channelKey)
	{
		if (channelKey.Length() <= 13)
			return false;
		return channelKey.ContainsAt("Public", 13);
	}

	bool IsFactionRoom(string channelKey, FactionKey factionKey)
	{
		if (factionKey == "")
			return channelKey.StartsWith("|");
		return channelKey.StartsWith(factionKey + "|");
	}

	bool IsGlobalRoom(string channelKey)
	{
		return channelKey == "|#PS-VoNRoom_Global";
	}

	bool IsLocalRoom(string channelKey)
	{
		return channelKey.StartsWith("|#PS-VoNRoom_Local");
	}

	// ------------------------- JIP Replication -------------------------
	// Only the channel list and player->channel map (no positions, no per-player auto rooms)
	override bool RplSave(ScriptBitWriter writer)
	{
		int channelsCount = m_aChannels.Count();
		writer.WriteInt(channelsCount);
		for (int i = 0; i < channelsCount; i++)
			writer.WriteString(m_aChannels[i]);

		int playersChannelCount = m_mPlayersChannel.Count();
		writer.WriteInt(playersChannelCount);
		for (int i = 0; i < playersChannelCount; i++)
		{
			writer.WriteInt(m_mPlayersChannel.GetKey(i));
			writer.WriteString(m_mPlayersChannel.GetElement(i));
		}

		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		int channelsCount;
		reader.ReadInt(channelsCount);
		for (int i = 0; i < channelsCount; i++)
		{
			string channelKey;
			reader.ReadString(channelKey);
			RegisterChannelLocal(channelKey);
		}

		int playersChannelCount;
		reader.ReadInt(playersChannelCount);
		for (int i = 0; i < playersChannelCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			m_mPlayersChannel.Insert(key, value);
		}

		m_bRplLoaded = true;

		return true;
	}
};
