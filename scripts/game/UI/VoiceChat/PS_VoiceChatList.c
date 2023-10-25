class PS_VoiceChatList : ScriptedWidgetComponent
{
	protected ResourceName m_sPlayerVoiceSelectorPrefab = "{086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout";
	protected ResourceName m_sVoiceRoomHeaderPrefab = "{C976E42779159507}UI/VoiceChat/VoiceRoomHeader.layout";
	
	// List of player voice selectors
	protected VerticalLayoutWidget m_wPlayersList;
	protected ref array<Widget> m_aPlayersListWidgets = {};
	
	// -------------------- Handler events --------------------
	override void HandlerAttached(Widget w)
	{
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("PlayersList"));
		Refreshing();
	}

	void Refreshing()
	{
		HardUpdate();
		
		GetGame().GetCallqueue().CallLater(Refreshing, 100);
	}
	
	
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
		
		// Clear old widgets
		foreach (Widget widget: m_aPlayersListWidgets)
		{
			m_wPlayersList.RemoveChild(widget);
		}
		m_aPlayersListWidgets.Clear();
		
		// Add new widgets
		foreach (int roomId: visibleRooms)
		{
			// Add header
			Widget roomHeader = GetGame().GetWorkspace().CreateWidgets(m_sVoiceRoomHeaderPrefab);
			PS_VoiceRoomHeader roomHeaderHandler = PS_VoiceRoomHeader.Cast(roomHeader.FindHandler(PS_VoiceRoomHeader));
			string roomName = VoNRoomsManager.GetRoomName(roomId);
			if (roomName == "") roomName = "Global";
			else {
				array<string> outTokens = new array<string>();
				roomName.Split("|", outTokens, false);
				roomName = outTokens[1];
			}
			roomHeaderHandler.SetRoomName(roomName);
			m_aPlayersListWidgets.Insert(roomHeader);
			m_wPlayersList.AddChild(roomHeader);
			
			foreach (int playerId: visiblePlayers)
			{
				if (VoNRoomsManager.GetPlayerRoom(playerId) != roomId) continue;
				
				Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerVoiceSelectorPrefab);
				PS_PlayerVoiceSelector handler = PS_PlayerVoiceSelector.Cast(playerSelector.FindHandler(PS_PlayerVoiceSelector));
				
				handler.SetPlayer(playerId);
				handler.m_OnClicked.Insert(Action_PlayerClick);
				
				m_aPlayersListWidgets.Insert(playerSelector);
				m_wPlayersList.AddChild(playerSelector);
			}
		}
		
		
		SoftUpdate();
	}
	
	void SoftUpdate()
	{
		
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
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		FactionKey currentPlayerFactionKey = playableManager.GetPlayerFactionKey(currentPlayerId);
		
		// No faction has only global
		if (currentPlayerFactionKey == "") {
			// Global room
			int globalRoom = VoNRoomsManager.GetRoomWithFaction("", "");
			outRoomsArray.Insert(globalRoom);
				
			return;
		}
		
		// Faction room
		int factionRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "Faction");
		outRoomsArray.Insert(factionRoom);
		
		// Room for commanders
		int commandRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "Command");
		outRoomsArray.Insert(commandRoom);
		
		// Room for each group
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableComponent playable = playables.GetElement(i);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			FactionKey factionKey = faction.GetFactionKey();
			
			if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip
			
			string groupName = playableManager.GetGroupNameByPlayable(playable.GetId());
			int groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, groupName); // No creation here :[
			if (!outRoomsArray.Contains(groupRoom))
				outRoomsArray.Insert(groupRoom);
		}
		
		
		// Current room
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


























