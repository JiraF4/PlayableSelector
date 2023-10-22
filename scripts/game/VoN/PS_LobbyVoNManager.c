//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_LobbyVoNManagerClass: ScriptComponentClass
{
	
};


// Manage VoN "Rooms"
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_LobbyVoNManager : ScriptComponent
{	
	// for each pair faction + group mace unique coords
	ref map<string, vector> roomOffsets = new map<string, vector>();
	
	// Move speech bois underground
	static vector roomInitialPosition = "1 100000 1";
	// offset every room
	vector lastOffset;
	
	override protected void OnPostInit(IEntity owner)
	{
		roomOffsets[""] = roomInitialPosition;
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_LobbyVoNManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_LobbyVoNManager.Cast(gameMode.FindComponent(PS_LobbyVoNManager));
		else
			return null;
	}
	
	// Move speech boi to selectted place
	void MoveToRoom(int playerId, FactionKey factionKey, string groupName)
	{
		string roomKey = factionKey + groupName;
		vector roomPosition = GetOrCreateRoomPosition(roomKey);
		RPC_MoveToRoom(playerId, roomPosition);
		Rpc(RPC_MoveToRoom, playerId, roomPosition);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_MoveToRoom(int playerId, vector position)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		if (playerController.GetPlayerId() != playerId) return;
		
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.VoNPosition = position;
	}
	
	// Create position if new key provided
	vector GetOrCreateRoomPosition(string roomKey)
	{
		if (!roomOffsets.Contains(roomKey)) {
			lastOffset = lastOffset + lastOffset.Right;
			roomOffsets[roomKey] = roomInitialPosition + lastOffset * 100;
		}
		return roomOffsets[roomKey];
	}
};