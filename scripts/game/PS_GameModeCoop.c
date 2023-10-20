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
	[Attribute("120000", UIWidgets.EditBox, "Time during which disconnected players reserve playable for reconnection in ms, -1 for infinity time", "", category: WB_GAME_MODE_CATEGORY)]
	int m_iAvailableReconnectTime = ;
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Game may be started only if admin exists on server.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bAdminMode;
	
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
		if (m_iAvailableReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iAvailableReconnectTime, false, playerId);
	}
	
	// If after m_iAvailableReconnectTime player still disconnected release playable
	void RemoveDisconnectedPlayer(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_EPlayableControllerState state = playableManager.GetPlayerState(playerId);
		if (state == PS_EPlayableControllerState.Disconected)
		{
			playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		}
	}
	
	bool IsAdminMode()
	{
		return m_bAdminMode;
	}
};

