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
	
	override void OnPlayerConnected(int playerId)
	{
		//SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		//playerController.SetInitialMainEntity(playerController);
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
		Rpc(RPC_ReOpenPlayableMenu);
		RPC_ReOpenPlayableMenu();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();
		GetGame().GetInputManager().AddActionListener("PlayableSelector", EActionTrigger.DOWN, OpenPlayableMenu);
		
		EntitySpawnParams params();
		vector mat[4];
		GetTransform(mat);
		params.Transform = mat;
		if (CameraEntity == null)
			CameraEntity = GetGame().SpawnEntityPrefab(Resource.Load("{C8FDE42491F955CB}Prefabs/ManualCameraInitialPlayer.et"), GetGame().GetWorld(), params);
		
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_ReOpenPlayableMenu()
	{
		GetGame().GetCallqueue().CallLater(ReOpenPlayableMenu, 1);
	}
	
	void ReOpenPlayableMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		SCR_PlayableSelectorMenu menu = SCR_PlayableSelectorMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.PlayableSelector));
		if (menu) menu.UpdateList();
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

