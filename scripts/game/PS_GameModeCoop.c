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
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Lobby can be open after game start.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bCanOpenLobbyInGame;
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Faction locked after selection.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bFactionLock;
		
	override void OnGameStart()
	{	
		super.OnGameStart();
		
		if (RplSession.Mode() != RplMode.Dedicated) {
			OpenLobby();
			if (m_bCanOpenLobbyInGame) GetGame().GetInputManager().AddActionListener("OpenLobby", EActionTrigger.DOWN, OpenLobby);
		}
	}
	
	void OpenLobby()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
	}
	
	protected override void OnPlayerConnected(int playerId)
	{
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId)
	}
	
	void SpawnInitialEntity(int playerId)
	{
        Resource resource = Resource.Load("{EF9F633DDC485F1F}Prefabs/InitialPlayer.et");
		EntitySpawnParams params = new EntitySpawnParams();
		GetTransform(params.Transform);		
        IEntity initialEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SetInitialEntity(initialEntity);
		playerController.SetInitialMainEntity(initialEntity);
	}
	
	override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playerController.SetInitialMainEntity(playableController.GetInitialEntity());
		PS_LobbyVoNManager.GetInstance().MoveToRoom(playerId, "dead", "dead");
	}
	
	// Update state for disconnected and start timer if need
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playerController.SetInitialMainEntity(playableController.GetInitialEntity());
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
	
	protected override void OnControllableDeleted(IEntity entity)
	{
		
	}
	
	protected override void OnPlayerDeleted(int playerId, IEntity player)
	{
		
	}
	
	bool IsAdminMode()
	{
		return m_bAdminMode;
	}
	
	void FactionLockSwitch()
	{
		m_bFactionLock = !m_bFactionLock;
		Rpc(RPC_SetFactionLock, !m_bFactionLock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFactionLock()
	{
		m_bFactionLock = !m_bFactionLock;
	}
	
	
	bool IsFactionLockMode()
	{
		return m_bFactionLock;
	}
	
	override bool RplSave(ScriptBitWriter writer)
	{
		
		writer.WriteBool(m_bFactionLock);
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
	
		reader.ReadBool(m_bFactionLock);
		
		return true;
	}
};

