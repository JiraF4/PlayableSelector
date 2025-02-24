// Coop game mode
// Open lobby on game start
// Disable respawn logic

class PS_GameModeCoopClass : SCR_BaseGameModeClass
{
}

class PS_GameModeCoop : SCR_BaseGameMode
{
	[Attribute("120000", UIWidgets.EditBox, "Time during which disconnected players reserve role for reconnection in ms, -1 for infinity time", "", category: "Reforger Lobby")]
	int m_iReconnectTime;

	[Attribute("-1", UIWidgets.EditBox, "Time during which disconnected players reserve role for reconnection in ms, -1 for infinity time", "", category: "Reforger Lobby")]
	int m_iReconnectTimeAfterBriefing;

	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Game may be started only if admin on server.", category: "Reforger Lobby")]
	protected bool m_bAdminMode;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Anyone can open lobby in game stage.", category: "Reforger Lobby")]
	protected bool m_bTeamSwitch;

	//[Attribute("0", uiwidget: UIWidgets.CheckBox, "Faction locked after selection.", category: "Reforger Lobby")]
	protected bool m_bFactionLock;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Markers can be placed only by squad leaders and only on briefing.", category: "Reforger Lobby")]
	protected bool m_bMarkersOnlyOnBriefing;

	[Attribute("0", UIWidgets.CheckBox, "Instead of just the leaders, every member is moved to their factions HQ room for a common briefing.\nMoving back to group channel is still possible.", category: "Reforger Lobby")]
	bool m_bPublicCommandBriefing;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove units not occupied by players.", category: "Reforger Lobby")]
	protected bool m_bRemoveRedundantUnits;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove default markers on squad leaders.", category: "Reforger Lobby")]
	protected bool m_bRemoveSquadMarkers;

	[Attribute("60000", UIWidgets.EditBox, "Time in milliseconds before restriction zones are removed.", category: "Reforger Lobby")]
	int m_iFreezeTime;

	[Attribute("0", UIWidgets.CheckBox, "Disables text chat for alive players on game stage. Admins can always see text chat.", category: "Reforger Lobby")]
	protected bool m_bDisableChat;

	[RplProp()]
	protected float m_fCurrentFreezeTime = 1;

	[Attribute("0", UIWidgets.CheckBox, "Creates a whitelist on the server for players who have taken roles and also for players specified in $profile:PS_SlotsReserver_Config.json and kicks everyone else.", category: "Reforger Lobby")]
	protected bool m_bReserveSlots;

	[Attribute("", UIWidgets.Auto, "", category: "Reforger Lobby")]
	protected ref array<ref PS_FactionRespawnCount> m_aFactionRespawnCount;
	protected ref map<FactionKey, PS_FactionRespawnCount> m_mFactionRespawnCount = new map<FactionKey, PS_FactionRespawnCount>();

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableVanillaGroupMenu;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisablePlayablesStreaming;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFriendliesSpectatorOnly;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFreezeTimeShootingForbiden;
	
	[Attribute("1", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableZalupaVision;
	
	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableBuildingModeAfterFreezeTime;
	
	[Attribute("-1", UIWidgets.Auto, "", category: "Reforger Lobby (WIP)")]
	protected int m_iFactionsBalance;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby (WIP)")]
	protected bool m_bShowCutscene;

	[Attribute("1", UIWidgets.CheckBox, "", category: "Reforger Lobby (WIP)")]
	protected bool m_bHolsterWeapon;

	[Attribute("0", UIWidgets.Auto, "", category: "Reforger Lobby (WIP)")]
	protected int m_iForceMenuFramerate;
	protected static int m_iOldMenuFramerate;

	protected ref ScriptInvokerInt m_OnGameStateChange = new ScriptInvokerInt();
	ScriptInvokerInt GetOnGameStateChange()
	{
		return m_OnGameStateChange;
	}

	protected ref ScriptInvokerString m_OnOnlyOneFactionAlive = new ScriptInvokerString();
	ScriptInvokerString GetOnOnlyOneFactionAlive()
	{
		return m_OnOnlyOneFactionAlive;
	}

	protected ref ScriptInvoker m_OnHandlePlayerKilled = new ScriptInvoker();
	ScriptInvoker GetOnHandlePlayerKilled()
	{
		return m_OnHandlePlayerKilled;
	}

	// Cache global
	protected PS_PlayableManager m_playableManager;
	protected PS_CutsceneManager m_CutsceneManager;

	// ------------------------------------------ Events ------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager && m_bDisableVanillaGroupMenu)
		{
			inputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowPlayerList);
			inputManager.RemoveActionListener("ShowGroupMenu", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowGroupMenu);
		}

		Widget FreezeTimeCounterOverlay = GetGame().GetWorkspace().FindAnyWidget("FreezeTimeCounterOverlay");
		if (FreezeTimeCounterOverlay)
			FreezeTimeCounterOverlay.RemoveFromHierarchy();

		m_playableManager = PS_PlayableManager.GetInstance();
		m_CutsceneManager = PS_CutsceneManager.GetInstance();

		foreach (PS_FactionRespawnCount factionRespawnCount : m_aFactionRespawnCount)
		{
			m_mFactionRespawnCount.Insert(
				factionRespawnCount.m_sFactionKey,
				factionRespawnCount
			);
		}

		string loadSave = GameSessionStorage.s_Data.Get("SCR_SaveFileManager_FileNameToLoad");
		if (loadSave != "")
		{
			SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
			saveManager.Load(loadSave);
		}

		if (Replication.IsServer())
		{
			PS_VoNRoomsManager.GetInstance().GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Global");

			m_fCurrentFreezeTime = m_iReconnectTime;
			Replication.BumpMe();
		}

		if (RplSession.Mode() != RplMode.Dedicated) {
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.WaitScreen);
			GetGame().GetInputManager().AddActionListener("OpenLobby", EActionTrigger.DOWN, Action_OpenLobby);
		}

		GetGame().GetCallqueue().CallLater(AddAdvanceAction, 0, false);

		GetGame().GetCallqueue().CallLater(RegisterEditorClosed, 100, false);

		if (!Replication.IsServer() && m_iForceMenuFramerate != 0)
		{
			BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
			video.Get("MaxFps", m_iOldMenuFramerate);
			video.Set("MaxFps", m_iForceMenuFramerate);
			GetGame().GetCallqueue().CallLater(ForceFramerate, 1000, true);
		}
	}
	void ForceFramerate()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		if (PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetState() == SCR_EGameModeState.GAME)
		{
			video.Set("MaxFps", m_iOldMenuFramerate);
			GetGame().UserSettingsChanged();

			GetGame().GetCallqueue().Remove(ForceFramerate);
		}
		else
		{
			video.Set("MaxFps", m_iForceMenuFramerate);
			GetGame().UserSettingsChanged();
		}
	}

	void RegisterEditorClosed()
	{
		SCR_EditorModeEntity editorModeEntity = SCR_EditorModeEntity.GetInstance();
		if (editorModeEntity)
		{
			editorModeEntity.GetOnClosed().Insert(EditorClosed);
		}
		else
			GetGame().GetCallqueue().CallLater(RegisterEditorClosed, 100, false);
	}
	
	void FreezeTimerAdvance(int time)
	{
		Rpc(RPC_FreezeTimerAdvance, time);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_FreezeTimerAdvance(int time)
	{
		time = time * 1000;
		m_iFreezeTime += time;
		m_fCurrentFreezeTime += time;
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(m_fCurrentFreezeTime + time);
	}
	
	void FreezeTimerEnd()
	{
		Rpc(RPC_FreezeTimerEnd);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_FreezeTimerEnd()
	{
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(5000);
	}

	void EditorClosed()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		playableController.SaveCameraTransform();
		playableController.SwitchFromObserver();
		
		IEntity entity = playerController.GetControlledEntity();
		if (!entity)
		{
			playableController.SwitchToMenu(SCR_EGameModeState.GAME);
			return;
		}
		
		PS_LobbyVoNComponent von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		if (von)
		{
			playableController.SwitchToMenu(SCR_EGameModeState.GAME);
			return;
		}
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
		invoker = chatPanelManager.GetCommandInvoker("tst");
		invoker.Insert(Test_Callback);
		invoker = chatPanelManager.GetCommandInvoker("pgc");
		invoker.Insert(PlayGameConfig_Callback);
		invoker = chatPanelManager.GetCommandInvoker("res");
		invoker.Insert(Respawn_Callback);
		invoker = chatPanelManager.GetCommandInvoker("rei");
		invoker.Insert(RespawnInit_Callback);
		invoker = chatPanelManager.GetCommandInvoker("unc");
		invoker.Insert(ForceUnconsious_Callback);
		invoker = chatPanelManager.GetCommandInvoker("spw");
		invoker.Insert(SpawnInit_Callback);
		invoker = chatPanelManager.GetCommandInvoker("spp");
		invoker.Insert(SpawnPosition_Callback);
		invoker = chatPanelManager.GetCommandInvoker("fta");
		invoker.Insert(FreezeTimerAdvance_Callback);
		invoker = chatPanelManager.GetCommandInvoker("fte");
		invoker.Insert(FreezeTimerEnd_Callback);
	}
	
	void FreezeTimerAdvance_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerAdvance(data.ToInt());
		FreezeTimerAdvance_Notify();
		Rpc(FreezeTimerAdvance_Notify);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void FreezeTimerAdvance_Notify()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_time_advanced");
	}
	
	void FreezeTimerEnd_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerEnd();
		FreezeTimerEnd_Notify();
		Rpc(FreezeTimerEnd_Notify);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void FreezeTimerEnd_Notify()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_time_force_end");
	}

	void SpawnPosition_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		array<string> outTokens = {};
		data.Split(" ", outTokens, true);
		string positionStr = outTokens[0];
		positionStr.Replace("|", " ");
		positionStr.Replace("<", " ");
		positionStr.Replace(">", " ");
		positionStr.Replace(",", " ");
		vector position = positionStr.ToVector();
		
		playableController.SpawnPrefab(data, position);
	}
	
	void SpawnInit_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.SpawnPrefab(data, "0 0 0");
	}

	void ForceUnconsious_Callback(SCR_ChatPanel panel, string data)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return;
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		if (characterControllerComponent.IsUnconscious())
			return;
		characterControllerComponent.SetUnconscious(true);
		GetGame().GetCallqueue().CallLater(ResetUnconsious, 400, false, characterControllerComponent);
	}
	void ResetUnconsious(CharacterControllerComponent characterControllerComponent)
	{
		characterControllerComponent.SetUnconscious(false);
	}

	void Respawn_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.ForceRespawnPlayer();
	}

	void RespawnInit_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.ForceRespawnPlayer(true);
	}

	void Test_Callback(SCR_ChatPanel panel, string data)
	{
		MemoryStatsSnapshot snapshot = new MemoryStatsSnapshot();
		int statsCount = MemoryStatsSnapshot.GetStatsCount();
		for (int i = 0; i < statsCount; i++)
		{
			Print(MemoryStatsSnapshot.GetStatName(i));
			Print(snapshot.GetStatValue(i));
		}
	}

	void ExportMissionData_Callback(SCR_ChatPanel panel, string data)
	{
		PS_MissionDataManager.GetInstance().WriteToFile();
	}

	void LoadMap_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) return;

		playableController.LoadMission(data);
	}

	void AdvanceStage_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) return;

		playableController.AdvanceGameState(SCR_EGameModeState.NULL);
	}

	void PlayGameConfig_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "")
			return;
		Resource resource = BaseContainerTools.LoadContainer(data);
		if (!resource)
			return;
		GameStateTransitions.RequestScenarioChangeTransition(data, "");
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

	protected PS_FactionRespawnCount GetFactionRespawnCount(FactionKey factionKey)
	{
		if (m_mFactionRespawnCount.Contains(factionKey))
		{
			return m_mFactionRespawnCount[factionKey];
		}
		return null;
	}

	protected override void OnPlayerConnected(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		playableManager.SetPlayerName(playerId, name);

		// TODO: remove CallLater
		#ifdef WORKBENCH
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 500, false, playerId);
		#else
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId);
		#endif
		m_OnPlayerConnected.Invoke(playerId);
	}

	protected override bool HandlePlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		m_OnHandlePlayerKilled.Invoke(playerId, playerEntity, killerEntity, killer);

		return super.HandlePlayerKilled(playerId, playerEntity, killerEntity, killer);
	}

	// Update state for disconnected and start timer if need (DO NOT DELETE CONTROLED ENTITY)
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerState(playerId, PS_EPlayableControllerState.Disconected);
		if (m_iReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iReconnectTime, false, playerId);

		IEntity controlledEntity = playerController.GetControlledEntity();
		if (controlledEntity) {
			RplComponent rpl = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
			rpl.GiveExt(RplIdentity.Local(), false);
		}

		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}

		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);

		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually.
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

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
								CarControllerComponent carController = CarControllerComponent.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent));
								if (carController)
								{
									carController.Shutdown();
									carController.StopEngine(false);
								}
							}
						}

						return;
					}
				}
			}
		}
	}

	bool CanJoinFaction(FactionKey factionKeyPlayer, FactionKey currentFaction)
	{
		if (m_iFactionsBalance == -1)
			return true;
		if (factionKeyPlayer == currentFaction)
			return true;

		map<FactionKey, int> players = new map<FactionKey, int>();
		map<FactionKey, int> playables = new map<FactionKey, int>();
		array<PS_PlayableContainer> playableComponents = m_playableManager.GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playableComponents)
		{
			FactionKey factionKey = playable.GetFactionKey();

			if (!players.Contains(factionKey))
				players[factionKey] = 0;
			if (!playables.Contains(factionKey))
				playables[factionKey] = 0;

			playables[factionKey] = playables[factionKey] + 1;
			int playerId = m_playableManager.GetPlayerByPlayable(playable.GetRplId());
			if (playerId > 0)
				players[factionKey] = players[factionKey] + 1;
		}
		if (currentFaction != "")
			players[currentFaction] = players[currentFaction] - 1;

		float maxFaction = 0;
		foreach (FactionKey factionKey, int count : playables)
			if (maxFaction < count)
				maxFaction = count;

		// Scale
		int minFaction = 999;
		foreach (FactionKey factionKey, int count : players)
		{
			int scaledCount = players[factionKey] * (maxFaction / playables[factionKey]);
			if (minFaction > scaledCount)
				minFaction = scaledCount;
		}

		int currentCount = players[factionKeyPlayer];
		int diff = currentCount - minFaction;

		return diff <= m_iFactionsBalance;
	}

	// ------------------------------------------ Actions ------------------------------------------
	// Open lobby in game
	void Action_OpenLobby()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!m_bTeamSwitch && !PS_PlayersHelper.IsAdminOrServer()) return;

		MenuBase lobbyMenu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby);
		if (!lobbyMenu)
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
		#ifdef WORKBENCH
		IEntity WBCharacter = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (WBCharacter)
			return;
		#endif

		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		Resource resource = Resource.Load("{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et");
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

	void TryRespawn(RplId playableId, int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && playableId != RplId.Invalid() && playableManager.GetPlayableById(playableId))
		{
			PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId).GetPlayableComponent();
			if (!playableComponent)
				return;

			FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			PS_FactionRespawnCount factionRespawns = GetFactionRespawnCount(factionKey);
			if (!factionRespawns || factionRespawns.m_iCount == 0)
			{
				SwitchToInitialEntity(playerId);
				return;
			}
			ResourceName prefabToSpawn = playableComponent.GetNextRespawn(factionRespawns.m_iCount == -1);
			if (factionRespawns.m_iCount > 0)
				factionRespawns.m_iCount--;
			if (prefabToSpawn != "")
			{
				int time = factionRespawns.m_iTime;
				if (factionRespawns.m_bWaveMode)
					time = factionRespawns.m_iTime - Math.Mod(GetGame().GetWorld().GetWorldTime(), time);
				if (playerId > 0)
					playableComponent.OpenRespawnMenu(time);

				PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
				GetGame().GetCallqueue().CallLater(Respawn, time, false, playerId, respawnData);
				return;
			}
		}

		SwitchToInitialEntity(playerId);
	}

	void Respawn(int playerId, PS_RespawnData respawnData)
	{
		Resource resource = Resource.Load(respawnData.m_sPrefabName);
		EntitySpawnParams params = new EntitySpawnParams();
		Math3D.MatrixCopy(respawnData.m_aSpawnTransform, params.Transform);
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		SCR_AIGroup aiGroup = m_playableManager.GetPlayerGroupByPlayable(respawnData.m_Id);
		SCR_AIGroup playabelGroup = aiGroup.GetSlave();
		playabelGroup.AddAIEntityToGroup(entity);

		PS_PlayableComponent playableComponentNew = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		playableComponentNew.SetPlayable(true);

		GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, 4);
	}

	void SwitchToSpawnedEntity(int playerId, PS_RespawnData respawnData, IEntity entity, int frameCounter)
	{
		if (frameCounter > 0) // Await four frames
		{
			GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, frameCounter - 1);
			return;
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();

		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		RplId playableId = playableComponent.GetRplId();

		playableComponent.CopyState(respawnData);
		if (playerId > 0)
		{
			playableManager.SetPlayerPlayable(playerId, playableId);
			playableManager.ForceSwitch(playerId);
		}
	}

	void SwitchToInitialEntity(int playerId)
	{
		if (playerId <= 0)
			return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		playableManager.ApplyPlayable(playerId);
	}

	// If after m_iReconnectTime player still disconnected release playable
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
		m_OnGameStateChange.Invoke(state);
		switch (state)
		{
			case SCR_EGameModeState.BRIEFING: // Force move to voice rooms
				foreach (int playerId : playerIds)
				{
					RplId playableId = playableManager.GetPlayableByPlayer(playerId);
					if (playableId == RplId.Invalid())
					{
						playableManager.SetPlayerFactionKey(playerId, "");
						VoNRoomsManager.MoveToRoom(playerId, "", "#PS-VoNRoom_Global");
					}else{
						if (playableManager.IsPlayerGroupLeader(playerId) || m_bPublicCommandBriefing)
						{
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), "#PS-VoNRoom_Command");
						} else {
							string groupName = playableManager.GetGroupCallsignByPlayable(playableId).ToString();
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), groupName);
						}
					}
				}
				if (m_bHolsterWeapon)
					playableManager.HolsterWeapons();
				break;
		}
	}

	// Switch to next game state
	void AdvanceGameState(SCR_EGameModeState oldState)
	{
		if (!Replication.IsServer())
			return;

		SCR_EGameModeState state = GetState();
		if (oldState != SCR_EGameModeState.NULL && oldState != state) return;
		switch (state)
		{
			case SCR_EGameModeState.PREGAME:
				SetGameModeState(SCR_EGameModeState.SLOTSELECTION);
				break;
			case SCR_EGameModeState.SLOTSELECTION:
				if (m_bShowCutscene)
				{
					SetGameModeState(SCR_EGameModeState.CUTSCENE);
					GetGame().GetCallqueue().CallLater(AdvanceGameState, m_CutsceneManager.GetCutsceneTime() + 400, false, SCR_EGameModeState.CUTSCENE);
					if (RplSession.Mode() == RplMode.Dedicated)
						PS_CutsceneManager.GetInstance().RunCutscene(0);
				}
				else
					SetGameModeState(SCR_EGameModeState.BRIEFING);
				break;
			case SCR_EGameModeState.CUTSCENE:
				SetGameModeState(SCR_EGameModeState.BRIEFING);
				break;
			case SCR_EGameModeState.BRIEFING:
				StartGame();
				break;
			case SCR_EGameModeState.GAME:
				SetGameModeState(SCR_EGameModeState.DEBRIEFING);
				break;
			case SCR_EGameModeState.DEBRIEFING:
				SetGameModeState(SCR_EGameModeState.POSTGAME);
				break;
			case SCR_EGameModeState.POSTGAME:
				break;
		}
		OpenCurrentMenuOnClients();
	}

	void StartGame()
	{
		m_iReconnectTime = m_iReconnectTimeAfterBriefing;
		if (m_bReserveSlots)
			ReserveSlots();
		PS_PlayableManager.GetInstance().RemoveRedundantUnits();
		restrictedZonesTimer(m_iFreezeTime);
		StartGameMode();
	}

	void ReserveSlots()
	{
		if (!Replication.IsServer())
			return;

		PS_SlotsReserver slotsReserver = PS_SlotsReserver.Cast(FindComponent(PS_SlotsReserver));
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();

		array<string> GUIDs = {};
		map<RplId, ref PS_PlayableContainer> playables = playableManager.GetPlayables();
		foreach (RplId id, PS_PlayableContainer playable : playables)
		{
			int playerId = playableManager.GetPlayerByPlayable(id);
			if (playerId <= 0)
				continue;

			string GUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
			GUIDs.Insert(GUID);
		}

		slotsReserver.AddGUIDs(GUIDs);
		slotsReserver.SetEnabled(true);
	}

	// TODO: move it to component
	void restrictedZonesTimer(int freezeTime)
	{
		// reduce time by second
		int time = 1000;
		if (freezeTime < time) time = freezeTime;
		freezeTime -= time;

		m_fCurrentFreezeTime = freezeTime;
		Replication.BumpMe();

		// Show timer on clients synced to server
		if (RplSession.Mode() != RplMode.Dedicated) RPC_restrictedZonesTimer(freezeTime);
		Rpc(RPC_restrictedZonesTimer, freezeTime);

		// next second or end
		if (freezeTime <= 0)
		{
			removeRestrictedZones();
			if (m_bDisableBuildingModeAfterFreezeTime)
				DisableBuildingMode();
		}
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
	void DisableBuildingMode()
	{
		SCR_EditorManagerCore editorManagerCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(outPlayers);
		foreach (int player : outPlayers)
		{
			SCR_EditorManagerEntity editorManager = editorManagerCore.GetEditorManager(player);
			if (editorManager)
				editorManager.SetCanOpen(false, EEditorCanOpen.ALIVE);
		}
	}
	PS_FreezeTimeCounter m_hFreezeTimeCounter;

	// ------------------------------------------ Global flags ------------------------------------------
	bool IsFreezeTimeEnd()
	{
		return m_fCurrentFreezeTime <= 0;
	}
	
	bool IsFreezeTimeShootingForbiden()
	{
		return m_bFreezeTimeShootingForbiden;
	}

	bool IsAdminMode()
	{
		return m_bAdminMode;
	}

	bool GetFriendliesSpectatorOnly()
	{
		if (!GetGame().GetPlayerController()) return true;
		if (SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId())) return false;
		return m_bFriendliesSpectatorOnly;
	}

	bool GetDisablePlayablesStreaming()
	{
		return m_bDisablePlayablesStreaming;
	}

	bool IsChatDisabled()
	{
		return m_bDisableChat;
	}

	bool IsFactionLockMode()
	{
		return m_bFactionLock;
	}
	
	bool IsArmaVisionDisabled()
	{
		return m_bDisableZalupaVision;
	}
	
	bool GetDisableBuildingModeAfterFreezeTime()
	{
		return m_bDisableBuildingModeAfterFreezeTime;
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
		return m_iReconnectTime;
	}
	void SetReconnectTime(int availableReconnectTime)
	{
		RPC_SetReconnectTime(availableReconnectTime);
		Rpc(RPC_SetReconnectTime, availableReconnectTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetReconnectTime(int availableReconnectTime)
	{
		m_iReconnectTime = availableReconnectTime;
	}

	bool GetRemoveRedundantUnits()
	{
		return m_bRemoveRedundantUnits;
	}
	void SetRemoveRedundantUnits(bool killRedundantUnits)
	{
		RPC_SetRemoveRedundantUnits(killRedundantUnits);
		Rpc(RPC_SetRemoveRedundantUnits, killRedundantUnits);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetRemoveRedundantUnits(bool killRedundantUnits)
	{
		m_bRemoveRedundantUnits = killRedundantUnits;
	}

	bool GetCanOpenLobbyInGame()
	{
		return m_bTeamSwitch;
	}
	void SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		RPC_SetCanOpenLobbyInGame(canOpenLobbyInGame);
		Rpc(RPC_SetCanOpenLobbyInGame, canOpenLobbyInGame);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		m_bTeamSwitch = canOpenLobbyInGame;
	}

	// ------------------------------------------ JIP Replication ------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bFactionLock);
		writer.WriteInt(m_iFreezeTime);
		writer.WriteInt(m_iReconnectTime);

		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bFactionLock);
		reader.ReadInt(m_iFreezeTime);
		reader.ReadInt(m_iReconnectTime);

		return true;
	}
}

[BaseContainerProps()]
class PS_FactionRespawnCount
{
	[Attribute()]
	FactionKey m_sFactionKey;
	[Attribute()]
	int m_iCount;
	[Attribute()]
	int m_iTime;
	[Attribute()]
	bool m_bWaveMode;
}
