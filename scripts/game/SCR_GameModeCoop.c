modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CoopLobby,
	PlayableSelector
}

//------------------------------------------------------------------------------------------------
class SCR_GameModeCoopClass: SCR_BaseGameModeClass
{
};

//------------------------------------------------------------------------------------------------
class SCR_GameModeCoop : SCR_BaseGameMode
{
	IEntity CameraEntity;
	
	
	static ref map<int, PlayableControllerState> playersStates = new map<int, PlayableControllerState>; // Controllers server only. (╯°□°）╯︵ ┻━┻
	
	void SetPlayerState(int playerId, PlayableControllerState state)
	{
		Rpc_SetPlayerStateClient(playerId, state, false);
		Rpc(Rpc_SetPlayerStateClient, playerId, state, false);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SetPlayerStateClient(int playerId, PlayableControllerState state, bool skipUpdate)
	{
		playersStates.Set(playerId, state);
		if (!skipUpdate) SCR_GameModeCoop.Cast(GetGame().GetGameMode()).UpdateMenu();
	}
	
	static PlayableControllerState GetPlayerState(int playerId)
	{
		if (!playersStates.Contains(playerId)) return PlayableControllerState.NotReady;
		return playersStates[playerId];
	}
	
	
	override void OnPlayerConnected(int playerId)
	{
		//SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		//playerController.SetInitialMainEntity(playerController);
		
		// TODO: more elegant pls
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		foreach (int remotePlayerId: playerIds)
		{
			Rpc(Rpc_SetPlayerStateClient, remotePlayerId, GetPlayerState(remotePlayerId), true);
		}
		
		GetGame().GetCallqueue().CallLater(RPC_UpdateMenu, 1000); // TODO: Fix delay
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandleOnCharacterDeath(notnull CharacterControllerComponent characterController, IEntity instigator)
	{
		super.HandleOnCharacterDeath(characterController, instigator);
		if (!instigator) return;
			
		UpdateMenu()
	}
	
	void UpdateMenu()
	{
		Rpc(RPC_UpdateMenu);
		RPC_UpdateMenu();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnGameStart()
	{
		playersStates = new map<int, PlayableControllerState>();
		
		super.OnGameStart();
		GetGame().GetInputManager().AddActionListener("PlayableSelector", EActionTrigger.DOWN, OpenPlayableMenu);
		
		EntitySpawnParams params();
		vector mat[4];
		GetTransform(mat);
		params.Transform = mat;
		//if (CameraEntity == null)
		//	CameraEntity = GetGame().SpawnEntityPrefab(Resource.Load("{C8FDE42491F955CB}Prefabs/ManualCameraInitialPlayer.et"), GetGame().GetWorld(), params);
		
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_UpdateMenu()
	{
		GetGame().GetCallqueue().CallLater(UpdateMenuClient, 1);
	}
	
	void UpdateMenuClient()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		SCR_PlayableSelectorMenu menu = SCR_PlayableSelectorMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.PlayableSelector));
		if (menu) menu.UpdateList();
		SCR_CoopLobby lobby = SCR_CoopLobby.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (lobby) lobby.UpdateMenu();
	}
	
	void OpenPlayableMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.PlayableSelector);
	}
	
	override bool CanPlayerRespawn(int playerID)
	{
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}
	
	
	override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{	
		vector mat[4];
		player.GetTransform(mat);
		Rpc(RPC_ForceCamera, playerId, mat[0], mat[1], mat[2], mat[3]); 
		RPC_ForceCamera(playerId, mat[0], mat[1], mat[2], mat[3]) 
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_ForceCamera(int playerId, vector m0, vector m1, vector m2, vector m3) // Literally garbage
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController.GetPlayerId() == playerId) 
		{
			EntitySpawnParams params();
			vector mat[4];
			mat[0] = m0;
			mat[1] = m1;
			mat[2] = m2;
			mat[3] = m3;
			params.Transform = mat;
			if (CameraEntity == null)
				CameraEntity = GetGame().SpawnEntityPrefab(Resource.Load("{C8FDE42491F955CB}Prefabs/ManualCameraInitialPlayer.et"), GetGame().GetWorld(), params);
		}
	}
	
	void RemoveCamera() 
	{
		if (CameraEntity) {
			RplComponent.DeleteRplEntity(CameraEntity, false);
			CameraEntity = null;
		}
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		
	}
	
};

