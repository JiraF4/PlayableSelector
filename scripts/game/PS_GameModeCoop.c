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
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove squad markers.", category: WB_GAME_MODE_CATEGORY)]
	protected bool m_bRemoveSquadMarkers;
	
	[Attribute("30000", UIWidgets.EditBox, "Freeze time", "", category: WB_GAME_MODE_CATEGORY)]
	int m_iFreezeTime = ;
	
	// ------------------------------------------ Events ------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();
		
		string loadSave = GameSessionStorage.s_Data.Get("SCR_SaveFileManager_FileNameToLoad");
		if (loadSave != "")
		{
			SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
			saveManager.Load(loadSave);
		}
		
		if (Replication.IsServer())
		{
			PS_VoNRoomsManager.GetInstance().GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Global" );
		}
		
		if (RplSession.Mode() != RplMode.Dedicated) {
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.WaitScreen);
			GetGame().GetInputManager().AddActionListener("OpenLobby", EActionTrigger.DOWN, Action_OpenLobby);
		}
		
		GetGame().GetCallqueue().CallLater(AddAdvanceAction, 0, false);
		
		SCR_EditorModeEntity editorModeEntity = SCR_EditorModeEntity.GetInstance();
		editorModeEntity.GetOnClosed().Insert(EditorClosed);
	}
	
	void EditorClosed()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playerController.SetControlledEntity(null);
		playableController.SwitchToMenu(GetState());
	}
	
	void AddAdvanceAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("adv");
		invoker.Insert(AdvanceStage_Callback);
		invoker = chatPanelManager.GetCommandInvoker("lom");
		invoker.Insert(LoadMap_Callback);
		invoker = chatPanelManager.GetCommandInvoker("sav");
		invoker.Insert(ExportMissionData_Callback);
	}
	
	void ExportMissionData_Callback(SCR_ChatPanel panel, string data)
	{
		PS_MissionDataManager.GetInstance().SaveData();
	}
	
	void LoadMap_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR && !Replication.IsServer()) return;
		
		playableController.LoadMission(data);
	}
	
	void AdvanceStage_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR && !Replication.IsServer()) return;
		
		playableController.AdvanceGameState(SCR_EGameModeState.NULL);
	}
	
	void removeRestrictedZones()
	{
		BaseGameMode gamemode = GetGame().GetGameMode();
		SCR_PlayersRestrictionZoneManagerComponent restrictionZoneManager = SCR_PlayersRestrictionZoneManagerComponent.Cast(gamemode.FindComponent(SCR_PlayersRestrictionZoneManagerComponent));
		set<SCR_EditorRestrictionZoneEntity> zones = restrictionZoneManager.GetZones();
		
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_End");
		
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			restrictionZoneManager.ResetPlayerZoneData(playerId);
		}
		
		for (int i = 0; i < zones.Count(); i++)
		{
			SCR_EditorRestrictionZoneEntity zone = zones.Get(i);
			SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
	}
	
	protected override void OnPlayerConnected(int playerId)
	{
		// TODO: remove CallLater
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId);
		m_OnPlayerConnected.Invoke(playerId);
	}
	
	protected override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		if (!Replication.IsServer()) return;
		
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
		
		playerController.SetInitialMainEntity(null);
		
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);

		if (IsMaster())
		{
			if (controlledEntity)
			{
				if (SCR_ReconnectComponent.GetInstance() && SCR_ReconnectComponent.GetInstance().IsReconnectEnabled())
				{
					if (SCR_ReconnectComponent.GetInstance().OnPlayerDC(playerId, cause))	// if conditions to allow reconnect pass, skip the entity delete  
					{
						CharacterControllerComponent charController = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
						if (charController)
						{
							charController.SetMovement(0, vector.Forward);
						}
						
						CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(controlledEntity.FindComponent(CompartmentAccessComponent)); // TODO nullcheck
						if (compAccess)
						{
							BaseCompartmentSlot compartment = compAccess.GetCompartment();
							if (compartment)
							{
								if(GetGame().GetIsClientAuthority())
								{
									CarControllerComponent carController = CarControllerComponent.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent));
									if (carController)
									{
										carController.Shutdown();
										carController.StopEngine(false);
									}
								}
								else
								{
									CarControllerComponent_SA carController = CarControllerComponent_SA.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent_SA));
									if (carController)
									{
										carController.Shutdown();
										carController.StopEngine(false);
									}
								}
							}
						}
						
						return;
					}
				}
			}
		}
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
		VoNRoomsManager.RestoreRoom(playerId);
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
						VoNRoomsManager.MoveToRoom(playerId, "", "#PS-VoNRoom_Global");
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
		if (state == SCR_EGameModeState.GAME) return;
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
				restrictedZonesTimer(m_iFreezeTime);
				PS_PlayableManager.GetInstance().ResetRplStream();
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
	
	// TODO: move it to component
	void restrictedZonesTimer(int freezeTime)
	{
		// reduce time by second
		int time = 1000;
		if (freezeTime < time) time = freezeTime;
		freezeTime -= time;
		
		// Show timer on clients synced to server
		if (RplSession.Mode() != RplMode.Dedicated) RPC_restrictedZonesTimer(freezeTime);
		Rpc(RPC_restrictedZonesTimer, freezeTime);
		
		// next second or end
		if (freezeTime <= 0)
			removeRestrictedZones();
		else
			GetGame().GetCallqueue().CallLater(restrictedZonesTimer, time, false, freezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_restrictedZonesTimer(int freezeTime)
	{
		if (freezeTime <= 0)
		{
			if (m_hFreezeTimeCounter)
			{
				m_hFreezeTimeCounter.GetRootWidget().RemoveFromHierarchy();
			}
			return;
		}
		
		if (m_hFreezeTimeCounter == null)
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{EC8A548C3F53BE4F}UI/layouts/FreezeTime/FreezeTimeCounter.layout");
			m_hFreezeTimeCounter = PS_FreezeTimeCounter.Cast(widget.FindHandler(PS_FreezeTimeCounter));
		}
		
		m_hFreezeTimeCounter.SetTime(freezeTime);
		
	}
	PS_FreezeTimeCounter m_hFreezeTimeCounter;
	
	// ------------------------------------------ Global flags ------------------------------------------
	bool IsAdminMode()
	{
		return m_bAdminMode;
	}
	
	bool IsFactionLockMode()
	{
		return m_bFactionLock;
	}
	
	bool GetMarkersOnlyOnBriefing()
	{
		return m_bMarkersOnlyOnBriefing;
	}
	void SetMarkersOnlyOnBriefing(bool markersOnlyOnBriefing)
	{
		RPC_SetMarkersOnlyOnBriefing(markersOnlyOnBriefing);
		Rpc(RPC_SetMarkersOnlyOnBriefing, markersOnlyOnBriefing);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetMarkersOnlyOnBriefing(bool markersOnlyOnBriefing)
	{
		m_bMarkersOnlyOnBriefing = markersOnlyOnBriefing;
	}
	
	bool GetDisableLeaderSquadMarkers()
	{
		return m_bRemoveSquadMarkers;
	}
	void SetDisableLeaderSquadMarkers(bool disableLeaderSquadMarkers)
	{
		RPC_SetDisableLeaderSquadMarkers(disableLeaderSquadMarkers);
		Rpc(RPC_SetDisableLeaderSquadMarkers, disableLeaderSquadMarkers);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetDisableLeaderSquadMarkers(bool disableLeaderSquadMarkers)
	{
		m_bRemoveSquadMarkers = disableLeaderSquadMarkers;
	}
	
	// Global flags set
	void FactionLockSwitch()
	{
		m_bFactionLock = !m_bFactionLock;
		Rpc(RPC_SetFactionLock, m_bFactionLock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFactionLock(bool factionLock)
	{
		m_bFactionLock = factionLock;
	}
	
	// ------------------------------------------ Global variables ------------------------------------------
	int GetFreezeTime()
	{
		return m_iFreezeTime;
	}
	void SetFreezeTime(int freezeTime)
	{
		RPC_SetFreezeTime(freezeTime);
		Rpc(RPC_SetFreezeTime, freezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFreezeTime(int freezeTime)
	{
		m_iFreezeTime = freezeTime;
	}
	
	int GetReconnectTime()
	{
		return m_iAvailableReconnectTime;
	}
	void SetReconnectTime(int availableReconnectTime)
	{
		RPC_SetReconnectTime(availableReconnectTime);
		Rpc(RPC_SetReconnectTime, availableReconnectTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetReconnectTime(int availableReconnectTime)
	{
		m_iAvailableReconnectTime = availableReconnectTime;
	}
	
	bool GetKillRedundantUnits()
	{
		return m_bKillRedundantUnits;
	}
	void SetKillRedundantUnits(bool killRedundantUnits)
	{
		RPC_SetKillRedundantUnits(killRedundantUnits);
		Rpc(RPC_SetKillRedundantUnits, killRedundantUnits);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetKillRedundantUnits(bool killRedundantUnits)
	{
		m_bKillRedundantUnits = killRedundantUnits;
	}
	
	bool GetCanOpenLobbyInGame()
	{
		return m_bCanOpenLobbyInGame;
	}
	void SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		RPC_SetCanOpenLobbyInGame(canOpenLobbyInGame);
		Rpc(RPC_SetCanOpenLobbyInGame, canOpenLobbyInGame);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		m_bCanOpenLobbyInGame = canOpenLobbyInGame;
	}
	
	// ------------------------------------------ JIP Replication ------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bFactionLock);
		writer.WriteInt(m_iFreezeTime);
		writer.WriteInt(m_iAvailableReconnectTime);
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bFactionLock);
		reader.ReadInt(m_iFreezeTime);
		reader.ReadInt(m_iAvailableReconnectTime);
		
		return true;
	}
};

