// Coop game mode
// Open lobby on game start
// Disable respawn logic

class PS_GameModeCoopClass: SCR_BaseGameModeClass
{
};

class PS_GameModeCoop : SCR_BaseGameMode
{
	[Attribute("120000", UIWidgets.EditBox, "Time during which disconnected players reserve playable for reconnection in ms, -1 for infinity time", "", category: WB_GAME_MODE_CATEGORY)]
	int m_iAvailableReconnectTime = ;
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Game may be started only if admin exists on server.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bAdminMode;
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Lobby can be open after game start.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bCanOpenLobbyInGame;
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Faction locked after selection.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bFactionLock;
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Markers can place only commanders on briefing.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bMarkersOnlyOnBriefing;
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove unused units.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bKillRedundantUnits;
	
	[Attribute("30000", UIWidgets.EditBox, "Freeze time", "", category: WB_GAME_MODE_CATEGORY)]
	int m_iFreezeTime = ;
	
	// ------------------------------------------ Events ------------------------------------------
	override void OnGameStart()
	{	
		super.OnGameStart();
		
		if (RplSession.Mode() != RplMode.Dedicated) {
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.WaitScreen);
			GetGame().GetInputManager().AddActionListener("OpenLobby", EActionTrigger.DOWN, Action_OpenLobby);
		}
	}
	
	void removeRestrictedZones()
	{
		BaseGameMode gamemode = GetGame().GetGameMode();
		SCR_PlayersRestrictionZoneManagerComponent restrictionZoneManager = SCR_PlayersRestrictionZoneManagerComponent.Cast(gamemode.FindComponent(SCR_PlayersRestrictionZoneManagerComponent));
		set<SCR_EditorRestrictionZoneEntity> zones = restrictionZoneManager.GetZones();
		for (int i = 0; i < zones.Count(); i++)
		{
			SCR_EditorRestrictionZoneEntity zone = zones.Get(i);
			SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
	}
	
	protected override void OnPlayerConnected(int playerId)
	{
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId);
		m_OnPlayerConnected.Invoke(playerId);
	}
	
	override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{
		if (!IsMaster()) return;
		
		// TODO: remove CallLater
		GetGame().GetCallqueue().CallLater(SwitchToInitialEntity, 200, false, playerId);
	}
	
	// Update state for disconnected and start timer if need
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerState(playerId, PS_EPlayableControllerState.Disconected); 
		if (m_iAvailableReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iAvailableReconnectTime, false, playerId);
		
		IEntity controlledEntity = playerController.GetControlledEntity();
		if (controlledEntity) {
			RplComponent rpl = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
			rpl.GiveExt(RplIdentity.Local(), false);
		}
		playerController.SetInitialMainEntity(playableController.GetInitialEntity());
		
		super.OnPlayerDisconnected(playerId, cause, timeout);
	}
	
	// ------------------------------------------ Actions ------------------------------------------
	// Open lobby in game
	void Action_OpenLobby()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!m_bCanOpenLobbyInGame && playerRole != EPlayerRole.ADMINISTRATOR) return;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
	}
	
	// Force open current game state menu
	void OpenCurrentMenuOnClients()
	{
		if (RplSession.Mode() != RplMode.Dedicated) RPC_OpenCurrentMenu(GetState());
		Rpc(RPC_OpenCurrentMenu, GetState());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_OpenCurrentMenu(SCR_EGameModeState state)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		if (playerController.GetPlayerId() == 0) return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(state);
	}
	
	void SpawnInitialEntity(int playerId)
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
        Resource resource = Resource.Load("{E1B415916312F029}Prefabs/InitialPlayer.et");
		EntitySpawnParams params = new EntitySpawnParams();
		GetTransform(params.Transform);		
        IEntity initialEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SetInitialEntity(initialEntity);
		playerController.SetInitialMainEntity(initialEntity);
	}
	
	void SwitchToInitialEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		playableManager.ApplyPlayable(playerId);
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
	
	override void OnGameStateChanged()
	{
		super.OnGameStateChanged();
		
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
	
		SCR_EGameModeState state = GetState();
		switch (state)
		{
			case SCR_EGameModeState.BRIEFING: // Force move to voice rooms
				foreach (int playerId: playerIds)
				{
					RplId playableId = playableManager.GetPlayableByPlayer(playerId);
					if (playableId == RplId.Invalid())
					{
						playableManager.SetPlayerFactionKey(playerId, "");
						VoNRoomsManager.MoveToRoom(playerId, "", "");
					}else{
						if (playableManager.IsPlayerGroupLeader(playerId)) 
						{
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), "#PS-VoNRoom_Command");
						} else {
							string groupName = playableManager.GetGroupCallsignByPlayable(playableId).ToString();
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), groupName);
						}
					}
				}
				break;
		}
	}
	
	// Switch to next game state
	void AdvanceGameState(SCR_EGameModeState oldState)
	{
		SCR_EGameModeState state = GetState();
		if (oldState != SCR_EGameModeState.NULL && oldState != state) return;
		switch (state) 
		{
			case SCR_EGameModeState.PREGAME:
				SetGameModeState(SCR_EGameModeState.SLOTSELECTION);
				break;
			case SCR_EGameModeState.SLOTSELECTION:
				SetGameModeState(SCR_EGameModeState.BRIEFING);
				break;
			case SCR_EGameModeState.BRIEFING:
				if (m_bKillRedundantUnits) PS_PlayableManager.GetInstance().KillRedundantUnits();
				GetGame().GetCallqueue().CallLater(removeRestrictedZones, m_iFreezeTime);
				StartGameMode();
				break;
			case SCR_EGameModeState.GAME:
				SetGameModeState(SCR_EGameModeState.POSTGAME);
				break;
			case SCR_EGameModeState.POSTGAME:
				break;
		}
		OpenCurrentMenuOnClients();
	}
	
	// ------------------------------------------ Global flags ------------------------------------------
	bool IsAdminMode()
	{
		return m_bAdminMode;
	}
	
	bool IsFactionLockMode()
	{
		return m_bFactionLock;
	}
	
	bool IsMarkersOnlyOnBriefing()
	{
		return m_bMarkersOnlyOnBriefing;
	}
	
	// Global flags set
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
	
	// ------------------------------------------ JIP Replication ------------------------------------------
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

