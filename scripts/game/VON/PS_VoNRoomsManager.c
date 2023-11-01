//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{
	
};


// Manage VoN "Rooms"
// Flying wallles rooms for naked VoN bois, radio soon.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManager : ScriptComponent
{	
	// server data
	ref map<int, vector> m_mRoomOffsets = new map<int, vector>(); // offset from initial position for each room
	ref map<string, int> m_mVoiceRoomsFromName = new map<string, int>(); // room key to roomId relationship
	
	// Replication data
	ref map<int, string> m_mVoiceRooms = new map<int, string>(); // room names for UI
	ref map<int, int> m_mPlayersRooms = new map<int, int>(); // player to room relationship
	
	// Move speech bois underground
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
		// Set default room position
		m_mRoomOffsets[0] = roomInitialPosition;
		m_mVoiceRooms[0] = "";
		m_mVoiceRoomsFromName[""] = 0;
		if (Replication.IsServer()) m_bRplLoaded = true;
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
		if (factionKey == "") roomName = ""; // Global has only one room
		
		int roomId = GetOrCreateRoomWithFaction(factionKey, roomName);
		vector roomPosition = GetOrCreateRoomPosition(roomId);
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_EGameModeState state = gameMode.GetState();
		if (state == SCR_EGameModeState.BRIEFING) { // On briefing also separate to squads
			RplId playableId = playableManager.GetPlayableByPlayer(playerId);
			int GroupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
			playableController.SetVoNKey("Menu" + factionKey + GroupCallSign.ToString());
		}
		else playableController.SetVoNKey("Menu" + factionKey); // Сhange VoN zone
		
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
	int GetOrCreateRoom(string roomKey)
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
	void RPC_CreateRoom(int roomId, string roomKey)
	{
		m_mVoiceRoomsFromName[roomKey] = roomId;
		m_mVoiceRooms[roomId] = roomKey;
	}
	int GetOrCreateRoomWithFaction(FactionKey factionKey, string roomName)
	{
		string roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		return GetOrCreateRoom(roomKey);
	}
	
	// Create position if new roomId provided
	vector GetOrCreateRoomPosition(int roomId)
	{
		if (!m_mRoomOffsets.Contains(roomId)) {
			lastOffset = lastOffset + lastOffset.Right;
			m_mRoomOffsets[roomId] = roomInitialPosition + lastOffset * 100;
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
			
			Print("--------");
			Print("key" + key.ToString());
			Print("value" + value.ToString());
			
			m_mPlayersRooms.Insert(key, value);
		}
		Print("----------------");
		int voiceRoomsCount;
		reader.ReadInt(voiceRoomsCount);
		for (int i = 0; i < voiceRoomsCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			Print("key" + key.ToString());
			Print("value" + value);
			Print("--------");
			
			m_mVoiceRooms.Insert(key, value);
			m_mVoiceRoomsFromName.Insert(value, key);
		}
		
		m_bRplLoaded = true;
		
		return true;
	}
};