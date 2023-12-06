// Widget displays info about voice rooms in voice chat.
// Path: {35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout

class PS_VoiceChatList : ScriptedWidgetComponent
{
	protected ResourceName m_sPlayerVoiceSelectorPrefab = "{086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout";
	protected ResourceName m_sVoiceRoomHeaderPrefab = "{C976E42779159507}UI/VoiceChat/VoiceRoomHeader.layout";
	
	// List of player voice selectors
	protected VerticalLayoutWidget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	protected ref array<PS_PlayerVoiceSelector> m_aPlayersVoicesList = {};
	protected ref array<PS_VoiceRoomHeader> m_aVoiceRoomHeadersList = {};
	
	// -------------------- Handler events --------------------
	override void HandlerAttached(Widget w)
	{
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("PlayersList"));
	}
	
	private int m_iOldPlayersCount = 0;
	private int m_iOldRoomsCount = 0;
	
	// -------------------- Update content functions --------------------
	void HardUpdate()
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		
		// Select only visible players
		array<int> visiblePlayers = new array<int>();
		GetVisiblePlayers(visiblePlayers);
		// And rooms
		array<int> visibleRooms = new array<int>();
		GetVisibleRooms(visibleRooms);
		
		if (visiblePlayers.Count() != m_iOldPlayersCount
		 || visibleRooms.Count() != m_iOldRoomsCount
		 || VoNRoomsManager.GetChangesFlag())
		{
			// Clear old widgets
			foreach (Widget widget: m_aPlayersListWidgets)
			{
				m_wPlayersList.RemoveChild(widget);
			}
			m_aVoiceRoomHeadersList.Clear();
			m_aPlayersListWidgets.Clear();
			m_aPlayersVoicesList.Clear();
			
			// Add new widgets
			foreach (int roomId: visibleRooms)
			{
				// Add header
				Widget roomHeader = GetGame().GetWorkspace().CreateWidgets(m_sVoiceRoomHeaderPrefab);
				PS_VoiceRoomHeader roomHeaderHandler = PS_VoiceRoomHeader.Cast(roomHeader.FindHandler(PS_VoiceRoomHeader));
				string roomName = VoNRoomsManager.GetRoomName(roomId);
				FactionKey factionKey = "";
				if (roomName == "") roomName = "Room not registered on server";
				else {
					array<string> outTokens = new array<string>();
					roomName.Split("|", outTokens, false);
					factionKey = outTokens[0];
					roomName = outTokens[1];
				}
				SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
				roomHeaderHandler.SetRoomName(faction, roomName, roomId);
				m_aVoiceRoomHeadersList.Insert(roomHeaderHandler);
				m_aPlayersListWidgets.Insert(roomHeader);
				m_wPlayersList.AddChild(roomHeader);
				
				foreach (int playerId: visiblePlayers)
				{
					if (VoNRoomsManager.GetPlayerRoom(playerId) != roomId) continue;
					
					Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerVoiceSelectorPrefab);
					PS_PlayerVoiceSelector handler = PS_PlayerVoiceSelector.Cast(playerSelector.FindHandler(PS_PlayerVoiceSelector));
					
					handler.SetPlayer(playerId);
					handler.m_OnClicked.Insert(Action_PlayerClick);
					m_aPlayersVoicesList.Insert(handler);
					
					m_aPlayersListWidgets.Insert(playerSelector);
					m_wPlayersList.AddChild(playerSelector);
				}
			}
			VoNRoomsManager.ResetChangesFlag();
		}
		
		m_iOldPlayersCount = visiblePlayers.Count();
		m_iOldRoomsCount = visibleRooms.Count();
		
		SoftUpdate();
	}
	
	void SoftUpdate()
	{
		foreach (PS_VoiceRoomHeader voiceRoomHeader: m_aVoiceRoomHeadersList)
		{
			voiceRoomHeader.UpdateInfo();
		}
		foreach (PS_PlayerVoiceSelector playerVoiceSelector: m_aPlayersVoicesList)
		{
			playerVoiceSelector.UpdateInfo();
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
		FactionKey currentPlayerFactionKey = playableManager.GetPlayerFactionKey(currentPlayerId);
		
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
			array<PS_PlayableComponent> playables = playableManager.GetPlayablesSorted();
			for (int i = 0; i < playables.Count(); i++) {
				PS_PlayableComponent playable = playables[i];
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
				SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
				FactionKey factionKey = faction.GetFactionKey();
				
				if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip
				
				int groupCallSign = playableManager.GetGroupCallsignByPlayable(playable.GetId());
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
			
			RplId playableId = playableManager.GetPlayableByPlayer(currentPlayerId);
			if (playableId != RplId.Invalid())
			{
				PS_PlayableComponent playable = playableManager.GetPlayableById(playableId);
				int groupCallSign = playableManager.GetGroupCallsignByPlayable(playable.GetId());
				int groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, groupCallSign.ToString());
				outRoomsArray.Insert(groupRoom);
			}
		}
		
		// Current room if something gone wrong
		int currentRoom = VoNRoomsManager.GetPlayerRoom(currentPlayerId);
		if (!outRoomsArray.Contains(currentRoom))
			outRoomsArray.Insert(currentRoom);
		
	}
	void GetVisiblePlayers(out array<int> outPlayersArray)
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (int playerId: playerIds)
		{
			// Check faction
			if (playableManager.GetPlayerFactionKey(playerId) != playableManager.GetPlayerFactionKey(currentPlayerController.GetPlayerId())) continue;
			
			// If all checks completed, add to out list
			outPlayersArray.Insert(playerId);
		}
	}
	
};


























