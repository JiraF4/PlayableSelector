//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{
	
};

// just string but funnier struct: [FactionKey + "|"] + string
typedef string VoNRoomKey;

// Manage VoN "Rooms"
// Flying wallles rooms for naked VoN bois.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManager : ScriptComponent
{	
	// server data
	ref map<int, vector> m_mRoomOffsets = new map<int, vector>(); // offset from initial position for each room
	ref map<VoNRoomKey, int> m_mVoiceRoomsFromName = new map<VoNRoomKey, int>(); // room key to roomId relationship
	
	// Replication data
	ref map<int, VoNRoomKey> m_mVoiceRooms = new map<int, VoNRoomKey>(); // room names for UI
	ref map<int, int> m_mPlayersRooms = new map<int, int>(); // player to room relationship
	
	// Move speech bois to space
	static vector roomInitialPosition = "1 1000000 1";
	
	// offset every room
	vector lastOffset;
	int m_iLastRoomId = 1;
	
	// Multiple menus may broken
	bool m_bHasChanges = true;
	void ResetChangesFlag()
	{
		m_bHasChanges = false;
	}
	bool GetChangesFlag()
	{
		return m_bHasChanges;
	}
	
	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		SCR_BaseGameMode baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		baseGameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
		
		// Set default room position
		m_mRoomOffsets[0] = roomInitialPosition;
		m_mVoiceRooms[0] = "";
		m_mVoiceRoomsFromName[""] = 0;
		if (Replication.IsServer()) m_bRplLoaded = true;
	}
	
	void OnPlayerConnected(int playerId)
	{
		// Create local room for player
		GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Local" + playerId.ToString());
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
	
	// ------------------------- Room changing -------------------------
	// Move to room by key and create if not exist, RUN ONLY ON SERVER
	void MoveToRoom(int playerId, FactionKey factionKey, string roomName)
	{
		if (!Replication.IsServer()) return;
		
		// Create new room and id if need
		int roomId = GetOrCreateRoomWithFaction(factionKey, roomName);
		FactionManager factionManager = GetGame().GetFactionManager();
		vector roomPosition = GetOrCreateRoomPosition(roomId, factionManager.GetFactionIndex(factionManager.GetFactionByKey(factionKey)));
		
		// Get global stuff
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_EGameModeState state = gameMode.GetState();
		
		// Channel VoN switch
		if (roomName.StartsWith("#PS-VoNRoom_Local"))
		{
			// We need silence
			playableController.SetVoNKey(roomName);
		}
		else if (state == SCR_EGameModeState.BRIEFING) { // On briefing also separate to squads
			// May be reworked later
			RplId playableId = playableManager.GetPlayableByPlayer(playerId);
			int GroupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
			playableController.SetVoNKey("Menu" + factionKey + GroupCallSign.ToString());
		}
		else playableController.SetVoNKey("Menu" + factionKey); // Сhange VoN zone
		
		// Finally move client to room
		RPC_MoveToRoom(playerId, roomId, roomPosition);
		Rpc(RPC_MoveToRoom, playerId, roomId, roomPosition);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_MoveToRoom(int playerId, int roomId, vector position)
	{
		m_bHasChanges = true;
		
		m_mPlayersRooms[playerId] = roomId;
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		if (playerController.GetPlayerId() != playerId) return;
		
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.VoNPosition = position;
	}
	
	
	// ------------------------- Room creation -------------------------
	// Create room if new key provided, RUN ONLY ON SERVER
	int GetOrCreateRoomWithFaction(FactionKey factionKey, string roomName)
	{
		VoNRoomKey roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		return GetOrCreateRoom(roomKey);
	}
	int GetOrCreateRoom(VoNRoomKey roomKey)
	{
		if (!Replication.IsServer()) return -1;
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) {
			// Create on every client
			RPC_CreateRoom(m_iLastRoomId, roomKey);
			Rpc(RPC_CreateRoom, m_iLastRoomId, roomKey);
			m_iLastRoomId++;
		}
		return m_mVoiceRoomsFromName[roomKey];
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_CreateRoom(int roomId, VoNRoomKey roomKey)
	{
		m_mVoiceRoomsFromName[roomKey] = roomId;
		m_mVoiceRooms[roomId] = roomKey;
	}
	
	// Create position if new roomId provided
	vector GetOrCreateRoomPosition(int roomId, int factionIndex)
	{
		if (!m_mRoomOffsets.Contains(roomId)) {
			lastOffset = lastOffset + lastOffset.Up * 100;
			m_mRoomOffsets[roomId] = roomInitialPosition + lastOffset;
		}
		return m_mRoomOffsets[roomId];
	}
	
	// ------------------------- Get -------------------------
	int GetPlayerRoom(int playerId)
	{
		if (!m_mPlayersRooms.Contains(playerId)) return -1;
		return m_mPlayersRooms[playerId];
	}
	int GetRoomWithFaction(FactionKey factionKey, string roomName)
	{
		string roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) return -1;
		return m_mVoiceRoomsFromName[roomKey];
	}
	string GetRoomName(int roomId)
	{
		if (!m_mVoiceRooms.Contains(roomId)) return "";
		return m_mVoiceRooms[roomId];
	}
	
	// ------------------------- JIP Replication -------------------------
	// Send our precision data, we need it on clients
	override bool RplSave(ScriptBitWriter writer)
	{
		int playersRoomsCount = m_mPlayersRooms.Count();
		writer.WriteInt(playersRoomsCount);
		for (int i = 0; i < playersRoomsCount; i++)
		{
			writer.WriteInt(m_mPlayersRooms.GetKey(i));
			writer.WriteInt(m_mPlayersRooms.GetElement(i));
		}
		
		int voiceRoomsCount = m_mVoiceRooms.Count();
		writer.WriteInt(voiceRoomsCount);
		for (int i = 0; i < voiceRoomsCount; i++)
		{
			writer.WriteInt(m_mVoiceRooms.GetKey(i));
			writer.WriteString(m_mVoiceRooms.GetElement(i));
		}
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		int playersRoomsCount;
		reader.ReadInt(playersRoomsCount);
		for (int i = 0; i < playersRoomsCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_mPlayersRooms.Insert(key, value);
		}
		int voiceRoomsCount;
		reader.ReadInt(voiceRoomsCount);
		for (int i = 0; i < voiceRoomsCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_mVoiceRooms.Insert(key, value);
			m_mVoiceRoomsFromName.Insert(value, key);
		}
		
		m_bRplLoaded = true;
		
		return true;
	}
};