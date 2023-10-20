// Coop game mode
// Open lobby on game start
// Disable respawn logic

//------------------------------------------------------------------------------------------------
class PS_GameModeCoopClass: SCR_BaseGameModeClass
{
};

//------------------------------------------------------------------------------------------------
class PS_GameModeCoop : SCR_BaseGameMode
{
	// Time during which disconnected players reserver playable for reconnection in ms, -1 for infinity time
	int m_iAvalibleReconnectTime = 2 * 1000 * 60;
		
	override void OnGameStart()
	{		
		super.OnGameStart();
		
		if (Replication.IsClient()) {
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
		}
	}
	
	protected override void OnPlayerConnected(int playerId)
	{
		
	}
	
	override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{	
		
	}
	
	// Update state for disconnected and start timer if need
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerState(playerId, PS_EPlayableControllerState.Disconected); 
		if (m_iAvalibleReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iAvalibleReconnectTime, false, playerId);
	}
	
	// If after m_iAvalibleReconnectTime player still disconnected release playable
	void RemoveDisconnectedPlayer(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_EPlayableControllerState state = playableManager.GetPlayerState(playerId);
		if (state == PS_EPlayableControllerState.Disconected)
		{
			playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		}
	}
};

