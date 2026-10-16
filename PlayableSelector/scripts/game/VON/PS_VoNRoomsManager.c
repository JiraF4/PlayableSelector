[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass : ScriptComponentClass
{
}

// DIRECT voice uses channel positions; proxy radio transceivers stay muted.
class PS_VoNRoomsManager : ScriptComponent
{
	ref array<string> m_aChannels = {};
	protected ref map<string, bool> m_mChannelsSet = new map<string, bool>();
	ref map<int, string> m_mPlayerChannels = new map<int, string>();
	protected ref map<int, bool> m_mPlayerParked = new map<int, bool>();
	// (playerId, channelKey, oldChannelKey), including removal with an empty key.
	ref ScriptInvoker m_OnPlayerChannelChanged = new ScriptInvoker();
	protected ref ScriptInvoker m_OnPlayerParkedChanged = new ScriptInvoker();
	protected ref array<int> m_aPendingSlotMoves_S = {};

	[Attribute("", UIWidgets.ResourceNamePicker, "Per-player VoN proxy prefab (PS_VoNProxyComponent + SCR_VoNComponent + muted BaseRadioComponent, RplComponent streaming disabled)", "et")]
	protected ResourceName m_sVoNProxyPrefab;
	protected ref map<int, IEntity> m_mProxies_S = new map<int, IEntity>();
	protected ref map<int, int> m_mProxySlots_S = new map<int, int>();
	protected ref array<int> m_aPendingRadioMutes = {};
	protected bool m_bRplLoaded;

	protected static const float PS_VON_SKY_ALTITUDE = 10000;
	protected static const float PS_VON_CHANNEL_BASE = 1000;
	protected static const float PS_VON_CHANNEL_SPACING = 1000;
	protected static const float PS_VON_MEMBER_STEP = 0.4;
	protected static const int PS_VON_GRID_COLS = 12;
	protected static const int PS_VON_TOTAL_SLOTS = 144;
	protected static const float PS_VON_ISOLATED_XZ = -5000;
	protected static const float PS_VON_NO_CHANNEL_XZ = -6000;

	override protected void OnPostInit(IEntity owner)
	{
		RegisterChannelLocal(GetLobbyChannelKey(), 0);
		m_bRplLoaded = Replication.IsServer();
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(ResolveSlotChannels_S);
		GetGame().GetCallqueue().Remove(MutePendingProxyRadios);
		GetGame().GetCallqueue().Remove(SpawnProxy_S);
		GetGame().GetCallqueue().Remove(FireJIPEvents);
		GetGame().GetCallqueue().Remove(UpdateParked_S);
		GetGame().GetCallqueue().Remove(UpdateAllParked_S);
		super.OnDelete(owner);
	}

	static PS_VoNRoomsManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		return PS_VoNRoomsManager.Cast(gameMode.FindComponent(PS_VoNRoomsManager));
	}

	bool IsReplicated()
	{
		return m_bRplLoaded;
	}

	ScriptInvoker GetOnPlayerParkedChanged()
	{
		return m_OnPlayerParkedChanged;
	}

	vector GetRoomPosition(int playerId)
	{
		// Controls a living character / not a menu speaker -> isolated far spot
		// Safe: the proxy radio is muted on every machine.
		if (IsPlayerParked(playerId))
			return Vector(PS_VON_ISOLATED_XZ, PS_VON_SKY_ALTITUDE, PS_VON_ISOLATED_XZ);

		string channelKey;
		if (!m_mPlayerChannels.Find(playerId, channelKey))
			return Vector(PS_VON_NO_CHANNEL_XZ - playerId * PS_VON_CHANNEL_SPACING, PS_VON_SKY_ALTITUDE, PS_VON_NO_CHANNEL_XZ);

		int index = m_aChannels.Find(channelKey);
		if (index < 0)
			return Vector(PS_VON_NO_CHANNEL_XZ - playerId * PS_VON_CHANNEL_SPACING, PS_VON_SKY_ALTITUDE, PS_VON_NO_CHANNEL_XZ);

		// Deterministic collision-free slot allocation within the room (12x12 grid = 144 slots >= 128 players).
		// Rank playerId against other players currently in the same channelKey.
		// Both client and server replicate m_mPlayerChannels, so this guarantees 100% identical coordinates
		// on all machines without any slot collisions even when server playerIds grow beyond 256.
		int slot = 0;
		for (int i = 0; i < m_mPlayerChannels.Count(); i++)
		{
			int otherId = m_mPlayerChannels.GetKey(i);
			if (otherId < playerId && m_mPlayerChannels.GetElement(i) == channelKey)
				slot++;
		}

		if (slot >= PS_VON_TOTAL_SLOTS)
			slot = PS_VON_TOTAL_SLOTS - 1;

		int gridX = slot % PS_VON_GRID_COLS;
		int gridZ = slot / PS_VON_GRID_COLS;

		// Centered 12x12 grid: (12 - 1) / 2 = 5.5
		vector memberOffset = Vector(
			(gridX - 5.5) * PS_VON_MEMBER_STEP,
			0,
			(gridZ - 5.5) * PS_VON_MEMBER_STEP
		);

		return Vector(PS_VON_CHANNEL_BASE + PS_VON_CHANNEL_SPACING * index, PS_VON_SKY_ALTITUDE, PS_VON_CHANNEL_BASE) + memberOffset;
	}

	static string GetLobbyChannelKey()
	{
		return "Lobby";
	}

	static string GetFactionChannelKey(string factionKey)
	{
		return "Faction_" + factionKey;
	}

	static string GetGroupChannelKey(int groupId, string factionKey)
	{
		return string.Format("Group_%1_%2", groupId, factionKey);
	}

	static string GetCommandChannelKey(string factionKey)
	{
		return "Command_" + factionKey;
	}

	static string GetPublicChannelKey(int playerId)
	{
		return string.Format("Public_%1", playerId);
	}

	static string GetDeafenChannelKey(int playerId)
	{
		return string.Format("Deafen_%1", playerId);
	}

	static bool IsDeafenChannelKey(string channelKey)
	{
		return channelKey.StartsWith("Deafen_");
	}

	static bool IsPublicChannelKey(string channelKey)
	{
		return channelKey.StartsWith("Public_");
	}

	// Player id from a "Public_{id}" / "Deafen_{id}" key, -1 otherwise.
	static int GetChannelKeyPlayerId(string channelKey)
	{
		int sep = channelKey.IndexOf("_");
		if (sep < 0)
			return -1;

		string prefix = channelKey.Substring(0, sep);
		if (prefix != "Public" && prefix != "Deafen")
			return -1;

		return channelKey.Substring(sep + 1, channelKey.Length() - sep - 1).ToInt();
	}

	// Faction key from "Faction_{key}" / "Command_{key}", empty otherwise.
	static string GetChannelKeyFactionKey(string channelKey)
	{
		if (channelKey.StartsWith("Faction_"))
			return channelKey.Substring(8, channelKey.Length() - 8);

		if (channelKey.StartsWith("Command_"))
			return channelKey.Substring(8, channelKey.Length() - 8);

		return "";
	}

	// Parse "Group_{groupId}_{factionKey}". Returns false for other keys.
	static bool ParseGroupChannelKey(string channelKey, out int groupId, out string factionKey)
	{
		if (!channelKey.StartsWith("Group_"))
			return false;

		string rest = channelKey.Substring(6, channelKey.Length() - 6);
		int sep = rest.IndexOf("_");
		if (sep <= 0)
			return false;

		groupId = rest.Substring(0, sep).ToInt();
		factionKey = rest.Substring(sep + 1, rest.Length() - sep - 1);
		return true;
	}

	// Reject unknown and noncanonical keys even for administrative moves.
	bool IsChannelKeyValid(string channelKey)
	{
		if (channelKey == GetLobbyChannelKey())
			return true;
		if (IsPublicChannelKey(channelKey) || IsDeafenChannelKey(channelKey))
		{
			int ownerId = GetChannelKeyPlayerId(channelKey);
			if (ownerId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(ownerId))
				return false;
			if (IsPublicChannelKey(channelKey))
				return channelKey == GetPublicChannelKey(ownerId);
			return channelKey == GetDeafenChannelKey(ownerId);
		}
		string factionKey = GetChannelKeyFactionKey(channelKey);
		if (factionKey != "")
			return GetGame().GetFactionManager().GetFactionByKey(factionKey) != null;

		int groupId;
		string groupFaction;
		if (!ParseGroupChannelKey(channelKey, groupId, groupFaction) || groupId < 0 || groupFaction == "")
			return false;
		if (channelKey != GetGroupChannelKey(groupId, groupFaction))
			return false;
		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		if (!pm)
			return false;
		foreach (PS_PlayableContainer slot : pm.GetPlayablesSorted())
		{
			if (slot.GetFactionKey() != groupFaction)
				continue;
			SCR_AIGroup group = pm.GetPlayerGroupByPlayable(slot.GetRplId());
			if (group && group.GetGroupID() == groupId)
				return true;
		}
		return false;
	}

	// Uses mirrored slot/phase data so the UI applies the server's access rule.
	bool CanPlayerJoinChannel(int playerId, string channelKey)
	{
		if (playerId <= 0 || !IsChannelKeyValid(channelKey))
			return false;
		if (channelKey == GetLobbyChannelKey())
			return true;
		if (IsDeafenChannelKey(channelKey))
			return GetChannelKeyPlayerId(channelKey) == playerId;
		if (IsPublicChannelKey(channelKey))
			return true;

		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!pm || !gameMode)
			return false;
		string playerFaction = pm.GetSlotFactionForPlayer(playerId);
		if (playerFaction == "")
			playerFaction = pm.GetPlayerFactionKey(playerId);
		string factionKey = GetChannelKeyFactionKey(channelKey);
		if (factionKey != "")
		{
			if (factionKey != playerFaction)
				return false;
			if (channelKey == GetCommandChannelKey(factionKey))
				return gameMode.GetState() == SCR_EGameModeState.BRIEFING || gameMode.m_bPublicCommandBriefing || pm.IsPlayerGroupLeader(playerId);
			return true;
		}
		int groupId;
		string groupFaction;
		return ParseGroupChannelKey(channelKey, groupId, groupFaction) && groupFaction == playerFaction;
	}

	bool CanPlayerVoiceKick(int callerId, int targetId, string channelKey)
	{
		if (callerId == targetId || !IsChannelKeyValid(channelKey))
			return false;
		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!pm || !gameMode || !pm.IsPlayerGroupLeader(callerId))
			return false;
		string factionKey = pm.GetSlotFactionForPlayer(callerId);
		string targetFaction = pm.GetSlotFactionForPlayer(targetId);
		if (factionKey == "" || targetFaction != factionKey)
			return false;
		string source = GetPlayerChannel(callerId);
		if (source == "" || source != GetPlayerChannel(targetId))
			return false;
		string ownGroup = pm.GetGroupVonRoomName(pm.GetPlayableByPlayer(callerId));
		string targetGroup = pm.GetGroupVonRoomName(pm.GetPlayableByPlayer(targetId));
		bool canKick = source == ownGroup && targetGroup != ownGroup;
		if (source == GetCommandChannelKey(factionKey))
			canKick = !pm.IsPlayerGroupLeader(targetId);
		if (!canKick || channelKey == source)
			return false;
		if (channelKey == GetLobbyChannelKey() || channelKey == GetFactionChannelKey(targetFaction))
			return true;
		return gameMode.GetState() == SCR_EGameModeState.BRIEFING && targetGroup != "" && channelKey == targetGroup;
	}

	void SetPlayerChannel_S(int playerId, string channelKey)
	{
		if (!Replication.IsServer() || playerId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(playerId) || !IsChannelKeyValid(channelKey))
			return;
		if (GetPlayerChannel(playerId) == channelKey)
			return;
		InitChannelIfNeeded(channelKey);
		RpcDo_SetPlayerChannel(playerId, channelKey);
		Rpc(RpcDo_SetPlayerChannel, playerId, channelKey);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPlayerChannel(int playerId, string channelKey)
	{
		PS_NetStat.Hit("RpcDo_SetPlayerChannel");
		string oldChannelKey = GetPlayerChannel(playerId);
		m_mPlayerChannels.Set(playerId, channelKey);
		m_OnPlayerChannelChanged.Invoke(playerId, channelKey, oldChannelKey);
	}

	void SetPlayerParked_S(int playerId, bool parked)
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
		if (m_mPlayerParked.Contains(playerId) && IsPlayerParked(playerId) == parked)
			return;
		RpcDo_SetPlayerParked(playerId, parked);
		Rpc(RpcDo_SetPlayerParked, playerId, parked);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPlayerParked(int playerId, bool parked)
	{
		PS_NetStat.Hit("RpcDo_SetPlayerParked");
		m_mPlayerParked.Set(playerId, parked);
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && pc.GetPlayerId() == playerId)
			PS_MenuVoN.Refresh();
		m_OnPlayerParkedChanged.Invoke(playerId, parked);
	}

	bool IsPlayerParked(int playerId)
	{
		bool parked;
		return m_mPlayerParked.Find(playerId, parked) && parked;
	}

	void UpdateParked_S(int playerId)
	{
		if (!Replication.IsServer())
			return;
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SetPlayerParked_S(playerId, gameMode && gameMode.GetState() == SCR_EGameModeState.GAME && !SCR_VoNComponent.PS_IsMenuSpeaker(playerId));
	}

	void UpdateAllParked_S()
	{
		if (!Replication.IsServer())
			return;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
			UpdateParked_S(playerId);
	}

	void QueueSlotChannelUpdate_S(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;
		if (m_aPendingSlotMoves_S.Contains(playerId))
			return;
		m_aPendingSlotMoves_S.Insert(playerId);
		if (m_aPendingSlotMoves_S.Count() == 1)
			GetGame().GetCallqueue().CallLater(ResolveSlotChannels_S, 0, false);
	}

	protected void ResolveSlotChannels_S()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		foreach (int playerId : m_aPendingSlotMoves_S)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			if (gameMode && gameMode.GetState() != SCR_EGameModeState.GAME)
				gameMode.AssignPhaseVoiceChannel(playerId);
			UpdateParked_S(playerId);
		}
		m_aPendingSlotMoves_S.Clear();
	}

	void SpawnProxy_S(int playerId)
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
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


		// FIX (GHOST NAMES): remove the player from the replicated channel map so late-joining
		// clients (JIP) no longer receive stale entries in their RplLoad snapshot. Without this,
		// m_mPlayerChannels accumulates entries for every player who ever connected, and the
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

	void MigratePublicRoomToGlobal_S(int ownerPlayerId)
	{
		if (!Replication.IsServer())
			return;
		array<int> occupants = GetPlayersInChannel(GetPublicChannelKey(ownerPlayerId));
		foreach (int playerId : occupants)
		{
			if (playerId != ownerPlayerId)
				SetPlayerChannel_S(playerId, GetLobbyChannelKey());
		}
	}

	// Called only by server disconnect cleanup; all mirrors remove membership and parking.
	void RemovePlayerFromChannel(int playerId)
	{
		if (!Replication.IsServer())
			return;
		MigratePublicRoomToGlobal_S(playerId);
		RpcDo_RemovePlayerFromChannel(playerId);
		Rpc(RpcDo_RemovePlayerFromChannel, playerId);
		m_aPendingSlotMoves_S.RemoveItem(playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RemovePlayerFromChannel(int playerId)
	{
		string oldChannelKey = GetPlayerChannel(playerId);
		m_mPlayerChannels.Remove(playerId);
		m_mPlayerParked.Remove(playerId);
		RemovePersonalChannelLocal(GetPublicChannelKey(playerId));
		RemovePersonalChannelLocal(GetDeafenChannelKey(playerId));
		m_OnPlayerChannelChanged.Invoke(playerId, "", oldChannelKey);
	}

	protected void RemovePersonalChannelLocal(string channelKey)
	{
		int index = m_aChannels.Find(channelKey);
		if (index < 0)
			return;
		// Keep the server's indices stable: deleting a room must not move every other room.
		m_aChannels[index] = "";
		m_mChannelsSet.Remove(channelKey);
	}

	protected void InitChannelIfNeeded(string channelKey)
	{
		if (m_mChannelsSet.Contains(channelKey))
			return;
		int index = m_aChannels.Find("");
		if (index < 0)
			index = m_aChannels.Count();
		RpcDo_CreateChannel(channelKey, index);
		Rpc(RpcDo_CreateChannel, channelKey, index);
	}

	// The server supplies the position index; packet arrival order cannot choose it.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_CreateChannel(string channelKey, int index)
	{
		RegisterChannelLocal(channelKey, index);
	}

	protected void RegisterChannelLocal(string channelKey, int index)
	{
		if (m_aChannels.Count() <= index)
			m_aChannels.Resize(index + 1);
		m_aChannels[index] = channelKey;
		m_mChannelsSet.Set(channelKey, true);
	}

	string GetPlayerChannel(int playerId)
	{
		string channelKey;
		m_mPlayerChannels.Find(playerId, channelKey);
		return channelKey;
	}

	array<string> GetChannels()
	{
		return m_aChannels;
	}

	array<int> GetPlayersInChannel(string channelKey)
	{
		array<int> players = {};
		foreach (int playerId, string playerChannel : m_mPlayerChannels)
		{
			if (playerChannel == channelKey)
				players.Insert(playerId);
		}
		players.Sort();
		return players;
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_aChannels.Count());
		foreach (string channelKey : m_aChannels)
			writer.WriteString(channelKey);
		writer.WriteInt(m_mPlayerChannels.Count());
		for (int i = 0; i < m_mPlayerChannels.Count(); i++)
		{
			int playerId = m_mPlayerChannels.GetKey(i);
			writer.WriteInt(playerId);
			writer.WriteString(m_mPlayerChannels.GetElement(i));
			writer.WriteBool(IsPlayerParked(playerId));
		}
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		m_aChannels.Clear();
		m_mChannelsSet.Clear();
		m_mPlayerChannels.Clear();
		m_mPlayerParked.Clear();
		int channelCount;
		reader.ReadInt(channelCount);
		for (int i = 0; i < channelCount; i++)
		{
			string channelKey;
			reader.ReadString(channelKey);
			if (channelKey == "")
				m_aChannels.Insert("");
			else
				RegisterChannelLocal(channelKey, i);
		}
		int playerCount;
		reader.ReadInt(playerCount);
		for (int i = 0; i < playerCount; i++)
		{
			int playerId;
			string playerChannel;
			bool parked;
			reader.ReadInt(playerId);
			reader.ReadString(playerChannel);
			reader.ReadBool(parked);
			m_mPlayerChannels.Set(playerId, playerChannel);
			m_mPlayerParked.Set(playerId, parked);
		}
		m_bRplLoaded = true;
		GetGame().GetCallqueue().CallLater(FireJIPEvents, 0, false);
		return true;
	}

	protected void FireJIPEvents()
	{
		foreach (int playerId, string channelKey : m_mPlayerChannels)
		{
			m_OnPlayerChannelChanged.Invoke(playerId, channelKey, "");
			m_OnPlayerParkedChanged.Invoke(playerId, IsPlayerParked(playerId));
		}
		PS_MenuVoN.Refresh();
	}
}
