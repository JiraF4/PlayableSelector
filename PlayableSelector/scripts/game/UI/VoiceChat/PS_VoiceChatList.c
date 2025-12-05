// Widget displays info about voice rooms in voice chat.
// Path: {35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout

class PS_VoiceChatList : SCR_ScriptedWidgetComponent
{
	// consts
	/*
	protected const ResourceName m_sPlayerVoiceSelectorPrefab = "{086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout";
	protected const ResourceName m_sVoiceRoomHeaderPrefab = "{C976E42779159507}UI/VoiceChat/VoiceRoomHeader.layout";
	*/
	
	[Attribute("{E705A59E59B577F2}UI/VoiceChat/VoiceChatRoom.layout")]
	protected ResourceName m_sVoiceChatRoomPrefab;
	
	// Global cached
	protected PlayerManager m_gPlayerManager;
	protected PS_PlayableManager m_gPlayableManager;
	protected PS_VoNRoomsManager m_gVoNRoomsManager;
	protected PlayerController m_pPlayerController;
	protected int m_iPlayerId;
	protected int m_iPublicRoomId;
	
	// Local
	// List of voice rooms
	protected VerticalLayoutWidget m_wRoomsList;
	protected ref map<int, PS_VoiceChatRoom> m_wRooms = new map<int, PS_VoiceChatRoom>;
	protected FactionKey m_sCurrentFactionKey;
	
	// -------------------- Handler events --------------------
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;
		
		super.HandlerAttached(w);
		
		// global
		m_gPlayerManager   = GetGame().GetPlayerManager();
		m_gPlayableManager = PS_PlayableManager.GetInstance();
		m_gVoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		
		// local
		m_wRoomsList = VerticalLayoutWidget.Cast(w.FindAnyWidget("RoomsList"));
		
		m_gVoNRoomsManager.m_eOnRoomChanged.Insert(MovePlayer);
		
		m_pPlayerController = GetGame().GetPlayerController();
		m_iPlayerId = m_pPlayerController.GetPlayerId();
		m_sCurrentFactionKey = m_gPlayableManager.GetPlayerFactionKey(m_iPlayerId);
		m_iPublicRoomId = m_gVoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Public" + m_iPlayerId.ToString());
		
		if (m_gVoNRoomsManager)
			Rebuild();
		
		GetGame().GetCallqueue().CallLater(UpdateInfo, 100, true);
	}
	
	void ~PS_VoiceChatList()
	{
		if (!GetGame().InPlayMode())
			return;
		
		if (!m_gVoNRoomsManager)
			return;
		if (!m_gVoNRoomsManager.m_eOnRoomChanged)
			return;
		
		m_gVoNRoomsManager.m_eOnRoomChanged.Remove(MovePlayer);
	}
	
	private int m_iOldPlayersCount = 0;
	private int m_iOldRoomsCount = 0;
		
	// -------------------- Update content functions --------------------
	void Clear()
	{
		SCR_WidgetHelper.RemoveAllChildren(m_wRoomsList);
		m_wRooms.Clear();
	}
	
	void Rebuild()
	{
		Clear();
		
		// Create initial list of visible rooms
		array<int> visibleRooms = new array<int>();
		GetVisibleRooms(visibleRooms);
		foreach (int roomId : visibleRooms)
		{
			CreateRoom(roomId);
		}
		
		UpdateInfo();
	}
	
	void CreateRoomIfNeed(int roomId)
	{
		if (m_gVoNRoomsManager.IsPublicRoom(roomId))
			CreateRoom(roomId);
	}
	
	void CreateRoom(int roomId)
	{
		VoNRoomKey roomKey = m_gVoNRoomsManager.GetRoomName(roomId);
		if (roomKey == "") return;
		Widget roomWidget = GetGame().GetWorkspace().CreateWidgets(m_sVoiceChatRoomPrefab);
		PS_VoiceChatRoom voiceChatRoom = PS_VoiceChatRoom.Cast(roomWidget.FindHandler(PS_VoiceChatRoom));
		voiceChatRoom.SetRoomId(roomId);
		
		array<int> playersInRoom = new array<int>();
		m_gVoNRoomsManager.GetPlayersInRoom(playersInRoom, roomId);
		foreach (int playerId : playersInRoom)
		{
			voiceChatRoom.AddPlayer(playerId);
		}
		
		m_wRoomsList.AddChild(roomWidget);
		m_wRooms[roomId] = voiceChatRoom;
	}
	
	void RemoveRoomIfNeed(int roomId)
	{
		if (m_gVoNRoomsManager.IsGlobalRoom(roomId)) return;
		if (m_gVoNRoomsManager.IsLocalRoom(roomId)) return;
		
		// current player
		if (m_gVoNRoomsManager.IsPublicRoom(roomId))
		{
			if (m_iPublicRoomId != roomId)
			{
				array<int> playersInRoom = new array<int>();
				m_gVoNRoomsManager.GetPlayersInRoom(playersInRoom, roomId);
				if (playersInRoom.IsEmpty())
					RemoveRoom(roomId);
			}
		} else {
			if (!m_gVoNRoomsManager.IsFactionRoom(roomId, m_sCurrentFactionKey) && m_gVoNRoomsManager.GetPlayerRoom(m_iPlayerId) != roomId)
			{
				RemoveRoom(roomId);
			}
		}
	}
	
	void RemoveRoom(int roomId)
	{
		if (!m_wRooms.Contains(roomId)) return;
		PS_VoiceChatRoom voiceChatRoom = m_wRooms[roomId];
		voiceChatRoom.GetRootWidget().RemoveFromHierarchy();
		m_wRooms.Remove(roomId);
	}
	
	void SwitchFaction(FactionKey factionKey)
	{
		if (m_sCurrentFactionKey == factionKey) return;
		m_sCurrentFactionKey = factionKey;
		Rebuild();
	}
	
	FactionKey GetFactionKey()
	{
		return m_sCurrentFactionKey;
	}
	
	void MovePlayer(int playerId, int roomId, int oldRoomId)
	{
		// remove from old room
		if (m_wRooms.Contains(oldRoomId))
		{
			m_wRooms[oldRoomId].RemovePlayer(playerId);
			RemoveRoomIfNeed(oldRoomId);
		}
		if (!m_wRooms.Contains(roomId))
			CreateRoomIfNeed(roomId);
		else
		{
			m_wRooms[roomId].AddPlayer(playerId);
		}
		
		UpdateInfo();
	}
	
	void UpdateInfo()
	{
		foreach (int roomId, PS_VoiceChatRoom voiceChatRoom : m_wRooms)
		{
			voiceChatRoom.UpdateInfo();
		}
	}
	
	// ----- Actions -----
	protected void Action_PlayerClick(SCR_ButtonBaseComponent playerSelector)
	{	
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
	}
	
	// -------------------- Extra lobby functions --------------------
	void GetVisibleRooms(out array<int> outRoomsArray)
	{
		// global
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		SCR_EGameModeState gameState = gameMode.GetState();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		FactionKey currentPlayerFactionKey = m_sCurrentFactionKey;
		
		if (gameState != SCR_EGameModeState.BRIEFING)
		{
			// Local room if you want some privacy
			int localRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Local" + currentPlayerId.ToString());
			outRoomsArray.Insert(localRoom);
			
			// Global room
			int globalRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Global");
			outRoomsArray.Insert(globalRoom);
			
			if (currentPlayerFactionKey != "")
			{
				// Faction localRoom
				int factionRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Faction");
				outRoomsArray.Insert(factionRoom);
				
				// Room for commanders
				int commandRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Command");
				outRoomsArray.Insert(commandRoom);
			}
			
			// Room for each group
			array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
			for (int i = 0; i < playables.Count(); i++) {
				PS_PlayableContainer playable = playables[i];
				SCR_Faction faction = playable.GetFaction();
				FactionKey factionKey = faction.GetFactionKey();
				
				if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip
				
				int groupCallSign = playableManager.GetGroupCallsignByPlayable(playable.GetRplId());
				int groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, groupCallSign.ToString()); // No creation here :[
				if (!outRoomsArray.Contains(groupRoom))
					outRoomsArray.Insert(groupRoom);
			}
			
			// Player public room
			int publicRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Public" + currentPlayerId.ToString());
			outRoomsArray.Insert(publicRoom);
			
			// Other players public rooms
			array<int> playersPublicRooms = new array<int>();
			VoNRoomsManager.GetPlayersPublicRooms(playersPublicRooms);
			foreach (int roomId : playersPublicRooms)
			{
				if (!outRoomsArray.Contains(roomId))
					outRoomsArray.Insert(roomId);
			}
		} else {
			// For briefing only command and my group
			if (currentPlayerFactionKey != "")
			{
				// Room for commanders
				int commandRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Command");
				outRoomsArray.Insert(commandRoom);
			}
			
			if (playableManager.IsPlayerGroupLeader(currentPlayerId))
			{
				// TODO: separate to method
				// Room for each group
				array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
				for (int i = 0; i < playables.Count(); i++) {
					PS_PlayableContainer playable = playables[i];
					FactionKey factionKey = playable.GetFactionKey();
					
					if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip
					
					int groupCallSign = playableManager.GetGroupCallsignByPlayable(playable.GetRplId());
					int groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, groupCallSign.ToString()); // No creation here :[
					if (!outRoomsArray.Contains(groupRoom))
						outRoomsArray.Insert(groupRoom);
				}
			} else {
				RplId playableId = playableManager.GetPlayableByPlayer(currentPlayerId);
				if (playableId != RplId.Invalid())
				{
					PS_PlayableContainer playable = playableManager.GetPlayableById(playableId);
					int groupCallSign = playableManager.GetGroupCallsignByPlayable(playable.GetRplId());
					int groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, groupCallSign.ToString());
					outRoomsArray.Insert(groupRoom);
				}
			}
		}
		
		// Current room if something gone wrong
		int currentRoom = VoNRoomsManager.GetPlayerRoom(currentPlayerId);
		if (!outRoomsArray.Contains(currentRoom))
			outRoomsArray.Insert(currentRoom);
		
	}
	
	void SetSelectedPlayer(int playerId)
	{
		foreach (PS_VoiceChatRoom room : m_wRooms)
		{
			room.SetSelectedPlayer(playerId);
		}
	}
};


























