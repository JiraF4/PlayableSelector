//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{

};

// just string but funnier
// struct: [FactionKey + "|"] + roomName
typedef string VoNRoomKey;

// Manage VoN "channels" (formerly position-based "rooms").
//
// Echo-style channel model: a channel is just a string key (which is also the radio encryption key
// that separates who can hear whom). Channels are created LAZILY - only when a player actually joins
// one - and player<->channel is a single replicated map. The old system pre-created a Local + Public
// room for EVERY player on connect (110+ rooms at 55 players, each via a Reliable Broadcast RPC) and
// also moved bodies to per-room sky positions; on a full server that reliable-channel traffic is a
// direct contributor to "Replication Flooded/Stalled" kicks. This version drops both.
//
// The voice routing itself (radio encryption keys via PS_PlayableControllerComponent.SetVoNKey) is
// unchanged - only channel identity (string key instead of int roomId + position) and replication.

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
	[Attribute("", UIWidgets.ResourceNamePicker, "Per-player VoN proxy prefab (PS_VoNProxyComponent + SCR_VoNComponent + BaseRadioComponent, RplComponent streaming disabled)", "et")]
	protected ResourceName m_sVoNProxyPrefab;
	protected ref map<int, IEntity> m_mProxies_S = new map<int, IEntity>(); // server-side, for cleanup
	protected ref map<int, int> m_mProxySlots_S = new map<int, int>();      // server-side, grid slot per player
	protected ref array<int> m_aPendingRadioApplies = {};                   // deferred (1 frame) re-tune queue
	protected ref map<int, float> m_mChannelChangeTime = new map<int, float>(); // FIX (DESYNC): per-player world-time when channel last changed, used to suppress false-positive DESYNC log during the 1-frame deferred ApplyRadioKey window + replication jitter
	protected bool m_bIsGamePhase = false; // FIX (PERF): during GAME, skip parked (alive) proxies in audit — vanilla radios handle voice for them. Spectator (menu speaker) proxies are still audited since they're LIVE on Global and can drift.

	// BaseRadioComponent.SetEncryptionKey() silently truncates keys to a very short
	// length (observed: 5 chars max on 1.7.0.41). Keys longer than the limit are
	// truncated, so "PSVoN_USSR_..." becomes just "PSVoN" — all proxies collapse to
	// one key = cross-channel / cross-faction voice leak. The fix: use ultra-short
	// keys derived from the channel index ("P0"-"P99" for channels, "Q" + playerId
	// for parked), which always fit. Channel identity is in m_aChannels + frequency;
	// the encryption key only needs to be unique per channel, not human-readable.

	const static int PS_VON_KEY_MAX_LEN = 5;

	static string EncodeVoNKey(int channelIndex)
	{
		// Channel key: "P" + zero-padded index. "P0"-"P999" fits in 5 chars for
		// up to 1000 channels. Unique per channel, always short enough.
		return "P" + channelIndex.ToString();
	}
	static string EncodeParkedKey(int playerId)
	{
		// Parked key: "Q" + playerId. Each parked player gets a unique key so
		// they don't hear each other. "Q1"-"Q9999" fits in 5 chars.
		return "Q" + playerId.ToString();
	}

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
		// Start the per-machine VoN audit (no-op unless s_bVoNDebug). Runs on the server AND each client so we can
		// compare, for every player, the INTENDED channel (m_mPlayersChannel = what the widget/UI shows) against
		// the REAL radio tuning (encryption key + frequency actually on the proxy transceiver). See VoNAuditTick.
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

	// [PS_VoNDBG] per-channel-change diagnostic toggle. Currently ON for live cross-group/faction verification.
	// NOTE: it runs on EVERY machine on EVERY channel move (a burst during the briefing mass-assignment), so set
	// this back to false once verification is done to drop that cost in production.
	static bool s_bVoNDebug = true;

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

		// Body-less: the encryption key + frequency live on the player's VoN proxy and are
		// derived from the channel itself (factionKey|roomName already encodes faction/group),
		// applied on every machine in RPC_SetPlayerChannel -> ApplyRadioKey. No body radios.

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
		// FIX (DESYNC): record when this player's channel changed so VoNAuditTick can suppress
		// false-positive DESYNC logs during the 1-frame deferred ApplyRadioKey window.
		m_mChannelChangeTime[playerId] = GetGame().GetWorld().GetWorldTime();

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

		ApplyRadioKey(playerId); // re-tune this player's VoN proxy radio on this machine

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
	// Per-player tiny replicated entity carrying the radio + SCR_VoNComponent, so menu
	// speakers (no living character) transmit/receive without a controlled body. Ported
	// from LiteLobby. The radio key+frequency are applied on EVERY machine from replicated
	// channel state (proxies replicate to all), so routing works regardless of ownership.

	// Server: spawn this player's proxy, or RE-BIND an existing one on reconnect.
	// Reconnect keeps the SAME playerId but gives a NEW connection (RplIdentity). The existing proxy is
	// still rpl.Give'n to the dead OLD connection, so the reconnected client can no longer drive capture
	// on it = nobody hears that player (the reported "can't hear Banan after his first reconnect" bug).
	// We also can't rely on RemoveProxy_S running on disconnect: the live server's QuickTvT gamemode
	// overrides OnPlayerDisconnected and does not run our cleanup, so the proxy persists. So instead of
	// skipping when a proxy exists, re-bind it (re-Give + re-register + re-tune) to the new connection.
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

			// Spread proxies 200m apart at 10km altitude: a radio also carries proximity speech
			// around the sender, so co-located proxies would let every menu speaker hear every
			// other regardless of channel. Encryption keys still seal cross-channel audio.
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

		ApplyRadioKey(playerId); // key may have been assigned before the proxy existed
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
		m_mChannelChangeTime.Remove(playerId); // FIX (DESYNC): clean up grace period entry

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

	// Re-tune a player's proxy radio. Runs on every machine (proxies replicate everywhere).
	// Deferred one frame + coalesced: a single event can deliver two state updates for one
	// player in the same frame, and the transceiver does not reliably apply two SetFrequency
	// calls in one frame.
	protected bool m_bRadioApplyScheduled = false;
	void ApplyRadioKey(int playerId)
	{
		if (!m_aPendingRadioApplies.Contains(playerId))
			m_aPendingRadioApplies.Insert(playerId);
		// Schedule the coalesced apply EXACTLY ONCE. The old code did Remove()+CallLater() on every NEW playerId,
		// which in a busy lobby (a new player changing channel almost every frame) kept cancelling and re-pushing
		// the single pending apply to "next frame" - so proxy radios could stay un-keyed on the connect-time
		// channel for a long time and players kept HEARING that stale shared channel (the busy-lobby voice leak).
		// Scheduling once (and never rescheduling) guarantees it fires next frame and drains the whole pending set,
		// no matter how much churn there is. Per-player coalescing is still handled by the Contains() check above
		// and by ApplyRadioKeyNow reading the latest m_mPlayersChannel.
		if (!m_bRadioApplyScheduled)
		{
			m_bRadioApplyScheduled = true;
			GetGame().GetCallqueue().CallLater(ApplyPendingRadioApplies, 0, false);
		}
	}
	protected void ApplyPendingRadioApplies()
	{
		m_bRadioApplyScheduled = false;
		foreach (int playerId : m_aPendingRadioApplies)
			ApplyRadioKeyNow(playerId);
		m_aPendingRadioApplies.Clear();
	}

	// Periodic per-machine VoN audit (gated by s_bVoNDebug). For every player it compares the INTENDED channel
	// (m_mPlayersChannel - what the widget / "visual" shows) against the REAL radio tuning (encryption key + freq
	// actually on the proxy transceiver, read back via GetEncryptionKey/GetFrequency). A DESYNC line means the
	// player's radio is NOT on the channel the UI shows => they transmit AND hear the wrong channel (the leak).
	// Runs on the server AND each client: the SERVER log shows the authoritative routing, each CLIENT log shows
	// that client's own proxies - including the LOCAL player's, i.e. their actual transmit/receive tuning.
	// Set/clear the GAME-phase flag so the audit tick can skip parked (alive) proxies during GAME.
	// Vanilla radios handle voice for alive players; their proxies are parked and cannot drift.
	// Spectator (menu speaker) proxies are LIVE on Global and MUST still be audited.
	// Runs on ALL machines (server + clients) since the audit runs everywhere.
	void SetVoNGamePhase(bool isGame)
	{
		m_bIsGamePhase = isGame;
	}

	void VoNAuditTick()
	{
		GetGame().GetCallqueue().CallLater(VoNAuditTick, 5000, false); // self-reschedule

		string machine = "client";
		if (Replication.IsServer())
			machine = "SERVER";
		PlayerController localPc = GetGame().GetPlayerController();
		int localId = -1;
		if (localPc)
			localId = localPc.GetPlayerId();

		// SELF-HEAL + audit in ONE pass (runs ALWAYS, even with debug off). For each proxy compute what
		// ApplyRadioKeyNow WOULD set (mute + frequency; the encryption key is inert so it is not compared) and read
		// what the radio REALLY is. Re-apply ONLY a proxy that DRIFTED (e.g. a remote proxy left stale on a previous
		// channel's frequency). This touches just genuinely-stale proxies, so the steady state (desync 0) costs only
		// a few cheap getters per proxy and NO re-tune. The old code blindly re-applied EVERY proxy every 5s in every
		// phase, which on a full server during GAME (alive proxies parked, never changing) was pure waste - this
		// keeps VoN effectively idle in GAME, matching the reconcile loop that fully stops there.
		// FIX (GHOST NAMES): prune stale player-channel entries on every machine. A disconnected
		// player's RemoveProxy_S broadcasts RPC_RemovePlayerFromChannel, but a JIP client whose
		// RplLoad snapshot was captured between the disconnect and the RPC delivery still holds the
		// stale entry. This sweep catches those stragglers + any other leak path. GetPlayers() returns
		// only currently-connected players; anything in m_mPlayersChannel not in that set is stale.
		if (!Replication.IsServer())
		{
			array<int> connectedPlayers = {};
			GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
			array<int> staleIds = {};
			foreach (int pid, string ck : m_mPlayersChannel)
			{
				if (!connectedPlayers.Contains(pid))
					staleIds.Insert(pid);
			}
			foreach (int staleId : staleIds)
			{
				string staleKey = m_mPlayersChannel[staleId];
				Print(string.Format("[PS_VoN] VoNAuditTick: pruning stale channel entry for disconnected player %1 (channel '%2')", staleId, staleKey), LogLevel.NORMAL);
				m_mPlayersChannel.Remove(staleId);
				m_mChannelChangeTime.Remove(staleId);
				m_eOnRoomChanged.Invoke(staleId, "", staleKey);
			}
		}

		int audited = 0;
		int desync = 0;
		int skipped = 0;
		foreach (int playerId, string channelKey : m_mPlayersChannel)
		{
			// During GAME, skip parked (alive) players — vanilla radios handle voice for them
			// and their proxies are muted. Only audit menu speakers (spectators) whose proxies
			// are LIVE on Global and can actually drift. Saves ~80% of proxy reads per tick.
			if (m_bIsGamePhase && !SCR_VoNComponent.PS_IsMenuSpeaker(playerId) && !IsLocalEditorOpenFor(playerId))
			{
				skipped++;
				continue;
			}

			IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
			if (!proxy)
				continue;
			BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
			if (!radio || radio.TransceiversCount() < 1)
				continue;
			BaseTransceiver tsv = radio.GetTransceiver(0);
			if (!tsv)
				continue;
			audited++;

			// A proxy is OFF the net when muted; both parked (alive / own editor) and a not-yet-registered channel
			// are expected muted. Separation is by FREQUENCY + MUTE (the key is inert, so it is not compared).
			bool shouldPark = !SCR_VoNComponent.PS_IsMenuSpeaker(playerId) || IsLocalEditorOpenFor(playerId);
			int chIdx = m_aChannels.Find(channelKey);
			bool expectedMuted = shouldPark || chIdx < 0;
			int expectedFreq;
			if (expectedMuted)
				expectedFreq = tsv.GetMinFrequency();
			else
				expectedFreq = ChannelFrequencyFor(tsv, channelKey);

			int actualFreq = tsv.GetFrequency();
			bool actualMuted = tsv.IsMuted();

			// Drift = wrong mute state, or (while live on the net) wrong frequency.
			if (actualMuted != expectedMuted || (!expectedMuted && actualFreq != expectedFreq))
			{
				desync++;
				ApplyRadioKeyNow(playerId); // SELF-HEAL: re-tune ONLY this drifted proxy
				// FIX (DESYNC): suppress the DESYNC log during a grace window after the channel
				// changed. ApplyRadioKey is deferred 1 frame, and replication can jitter, so the
				// radio may briefly differ from the intended state. The heal above runs regardless;
				// only the LOG is suppressed to eliminate false-positive noise.
				float changeTime;
				bool inGrace = m_mChannelChangeTime.Find(playerId, changeTime)										&& (GetGame().GetWorld().GetWorldTime() - changeTime) < 1000; // 1s grace
				if (s_bVoNDebug && !inGrace)
				{
					string localTag = "";
					if (playerId == localId)
						localTag = " LOCAL";
					PrintFormat("[PS_VoNAudit][%1]%2 DESYNC player=%3: visual(channel)='%4' SHOULD muted=%5 freq=%6, but radio REALLY muted=%7 freq=%8 - healed",
						machine, localTag, playerId, channelKey, expectedMuted, expectedFreq, actualMuted, actualFreq);
				}
			}
		}
		if (s_bVoNDebug && audited > 0)
		{
			if (skipped > 0)
				PrintFormat("[PS_VoNAudit][%1] audited=%2 desync=%3 (skipped %4 parked proxies in GAME phase)", machine, audited, desync, skipped);
			else
				PrintFormat("[PS_VoNAudit][%1] audited=%2 desync=%3", machine, audited, desync);
		}

		if (!s_bVoNDebug)
			return;

		// During GAME, skip the VoNHear diagnostic for alive local players - their proxy is muted
		// and the O(n²) comparison produces meaningless noise. Spectators (menu speakers) still get it.
		if (m_bIsGamePhase && localId > 0 && !SCR_VoNComponent.PS_IsMenuSpeaker(localId))
			return;

		// Who does the LOCAL player actually share a radio net with (= who they can HEAR)? Independent of the
		// channel map - it groups by the REAL FREQUENCY on each proxy transceiver (the encryption key is inert,
		// so frequency + mute are what separate channels). Muted proxies are off the net and skipped. Anyone on
		// the local frequency but in a DIFFERENT channel is a concrete cross-channel / cross-faction LEAK and is
		// tagged + raised to WARNING so it stands out in the log.
		if (localId > 0)
		{
			IEntity localProxy = PS_VoNProxyComponent.GetProxyEntity(localId);
			BaseRadioComponent localRadio;
			BaseTransceiver localTsv;
			if (localProxy)
			{
				localRadio = BaseRadioComponent.Cast(localProxy.FindComponent(BaseRadioComponent));
				if (localRadio && localRadio.TransceiversCount() > 0)
					localTsv = localRadio.GetTransceiver(0);
			}
			if (localTsv)
			{
				int localFreq = localTsv.GetFrequency();
				bool localMuted = localTsv.IsMuted();
				string localChannel;
				if (!m_mPlayersChannel.Find(localId, localChannel))
					localChannel = "";
				string sharers = "";
				bool leak = false;
				foreach (int pid, string ck : m_mPlayersChannel)
				{
					if (pid == localId)
						continue;
					IEntity p = PS_VoNProxyComponent.GetProxyEntity(pid);
					if (!p)
						continue;
					BaseRadioComponent r = BaseRadioComponent.Cast(p.FindComponent(BaseRadioComponent));
					if (!r || r.TransceiversCount() < 1)
						continue;
					BaseTransceiver rt = r.GetTransceiver(0);
					if (!rt || rt.IsMuted())
						continue; // off the net, can't be heard
					if (rt.GetFrequency() != localFreq)
						continue; // different frequency = different channel
					string tag = "";
					if (ck != localChannel)
					{
						tag = "<LEAK:diff-channel>";
						leak = true;
					}
					sharers = sharers + pid.ToString() + "(" + ck + ")" + tag + " ";
				}
				LogLevel lvl = LogLevel.NORMAL;
				if (leak)
					lvl = LogLevel.WARNING;
				Print(string.Format("[PS_VoNHear][%1] LOCAL player=%2 visual(channel)='%3' freq=%4 muted=%5 -> shares frequency with: [%6]",
					machine, localId, localChannel, localFreq, localMuted, sharers), lvl);
			}
		}
	}
	protected void ApplyRadioKeyNow(int playerId)
	{
		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
		if (!proxy)
			return;
		BaseRadioComponent radio = BaseRadioComponent.Cast(proxy.FindComponent(BaseRadioComponent));
		if (!radio || radio.TransceiversCount() < 1)
			return;
		BaseTransceiver tsv = radio.GetTransceiver(0);
		if (!tsv)
			return;

		string channelKey;
		if (!m_mPlayersChannel.Find(playerId, channelKey))
			channelKey = "";

		// Take the proxy OFF the menu net by MUTING the transceiver when the player is NOT a menu speaker
		// (alive in a playable) or their own GM editor is open. We MUTE (not just "park on a silent key")
		// because the proxy radio's per-channel encryption key does NOT apply at runtime - every proxy reads
		// back the prefab default "PSVoN" - so isolation can NOT rely on the key. Parking previously left every
		// off-net proxy on the SAME min frequency with a unique-but-inert key, so they all collapsed onto one
		// frequency and could hear each other. A muted transceiver is silent regardless of key/freq.
		if (!SCR_VoNComponent.PS_IsMenuSpeaker(playerId) || IsLocalEditorOpenFor(playerId))
		{
			tsv.SetMuteState(true);
			radio.SetEncryptionKey(EncodeParkedKey(playerId));
			tsv.SetFrequency(tsv.GetMinFrequency());
			if (s_bVoNDebug)
				PrintFormat("[PS_VoNApply] player=%1 PARKED+MUTED (not a menu speaker / editor open)", playerId);
			return;
		}

		// chIdx < 0 means this machine has not registered the channel yet - a transient replication-order window
		// (the player->channel map can arrive before the channel list on a JIP client). The old code fell back to
		// ChannelFrequencyFor -> minFreq, the SAME frequency parked proxies use, so an un-registered-channel proxy
		// collapsed onto the parked sink and (key inert) could hear them. MUTE until the channel registers; the
		// audit self-heal re-applies (unmutes + tunes) once the channel list catches up.
		int chIdx = m_aChannels.Find(channelKey);
		if (chIdx < 0)
		{
			tsv.SetMuteState(true);
			if (s_bVoNDebug)
				PrintFormat("[PS_VoNApply] player=%1 MUTED (channel '%2' not registered on this machine yet)", playerId, channelKey);
			return;
		}

		int freq = ChannelFrequencyFor(tsv, channelKey);
		tsv.SetMuteState(false);
		radio.SetEncryptionKey(EncodeVoNKey(chIdx));
		tsv.SetFrequency(freq);
		if (s_bVoNDebug)
			PrintFormat("[PS_VoNApply] player=%1 KEYED key='%2' freq=%3 chIdx=%4 channelKey='%5'", playerId, EncodeVoNKey(chIdx), freq, chIdx, channelKey);

		// The LOCAL player's menu-voice RECEPTION is armed once in PS_MenuVoN.Activate and otherwise stays on the
		// room active at that moment; re-point it at this freshly-tuned transceiver so listening follows the
		// channel the same way transmitting does (fixes hearing the connect-time channel after moving rooms).
		PlayerController localPc = GetGame().GetPlayerController();
		if (localPc && localPc.GetPlayerId() == playerId)
			PS_MenuVoN.PS_ReapplyReceive(tsv);
	}

	// Frequency from the channel's index in the replicated m_aChannels (consistent order on
	// every machine), stepped by the transceiver resolution within its band.
	protected int ChannelFrequencyFor(BaseTransceiver tsv, string channelKey)
	{
		int minFreq = tsv.GetMinFrequency();
		int maxFreq = tsv.GetMaxFrequency();
		int step = tsv.GetFrequencyResolution();
		if (step <= 0)
			step = 10;

		int index = m_aChannels.Find(channelKey);
		if (index < 0)
			return minFreq;

		int freq = minFreq + (index + 1) * step;
		if (freq > maxFreq)
		{
			Print(string.Format("[PS_VoN] Out of frequency slots for channel '%1' (%2 channels) - widen the proxy radio band", channelKey, m_aChannels.Count()), LogLevel.WARNING);
			freq = maxFreq;
		}
		return freq;
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
		m_mChannelChangeTime.Remove(playerId);
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
