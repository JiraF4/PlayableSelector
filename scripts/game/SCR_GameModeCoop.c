modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	PlayableSelector
}

//------------------------------------------------------------------------------------------------
class SCR_GameModeCoopClass: SCR_BaseGameModeClass
{
};

//------------------------------------------------------------------------------------------------
class SCR_GameModeCoop : SCR_BaseGameMode
{
	
	override void OnPlayerConnected(int playerId)
	{
		
	}

	
	//------------------------------------------------------------------------------------------------
	override void HandleOnCharacterDeath(notnull CharacterControllerComponent characterController, IEntity instigator)
	{
		super.HandleOnCharacterDeath(characterController, instigator);
		Print("Bitch-Ass dead!",LogLevel.WARNING);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();
		GetGame().GetInputManager().AddActionListener("PlayableSelector", EActionTrigger.DOWN, OpenPlayableMenu);
		//OpenPlayableMenu();
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
		EntitySpawnParams params();
		vector mat[4];
		player.GetTransform(mat);
		params.Transform = mat;
		GetGame().SpawnEntityPrefab(Resource.Load("{C8FDE42491F955CB}Prefabs/ManualCameraInitialPlayer.et"), GetGame().GetWorld(), params);
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		
	}
	
};

