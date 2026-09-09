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
	
	[Attribute("0", UIWidgets.EditBox, "Time in milliseconds before characters are activated.", category: "Reforger Lobby (WIP)")]
	int m_iDisableTime;

	[Attribute("0", UIWidgets.CheckBox, "Disables text chat for alive players on game stage. Admins can always see text chat.", category: "Reforger Lobby")]
	protected bool m_bDisableChat;

	[RplProp()]
	protected float m_fCurrentFreezeTime = 1;
	// One-shot guard for FreezeTimerEnd: every group leader's PS_SquadsReadyWidget independently RPCs the server
	// when all squads are ready, so the end notification must fire only once per freeze period (reset in StartGame).
	protected bool m_bFreezeEndTriggered;
	[RplProp()]
	protected float m_fGameStartTime = 0;
	[RplProp()]
	protected float m_fGameStartElapsedTime = 0;

	[Attribute("0", UIWidgets.CheckBox, "Creates a whitelist on the server for players who have taken roles and also for players specified in $profile:PS_SlotsReserver_Config.json and kicks everyone else.", category: "Reforger Lobby")]
	protected bool m_bReserveSlots;

	[Attribute("", UIWidgets.Auto, "", category: "Reforger Lobby")]
	protected ref array<ref PS_FactionRespawnCount> m_aFactionRespawnCount;
	protected ref map<FactionKey, PS_FactionRespawnCount> m_mFactionRespawnCount = new map<FactionKey, PS_FactionRespawnCount>();

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableVanillaGroupMenu;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisablePlayablesStreaming;

	[Attribute("0", UIWidgets.CheckBox, "Stream the world around spectator cameras via a per-connection MPObserver (deferred + throttled). Enable this when DisablePlayablesStreaming is OFF so spectators still see the action without force-streaming every playable.", category: "Reforger Lobby")]
	protected bool m_bSpectatorStreamingObserver;

	[Attribute("1", UIWidgets.CheckBox, "BRIEFING preload: pre-stream each slotted player's surroundings during the briefing via a temporary per-connection MPObserver at their slot, removed shortly after GAME starts. Spreads the briefing->game streaming over the calm briefing instead of bursting at game start. Same static/temporary pattern vanilla uses to preload a spawn point (SCR_SpawnRequestComponent) - NOT the moving whole-map spectator observer that flooded. Turn OFF if a populated test shows any replication regression.", category: "Reforger Lobby")]
	protected bool m_bBriefingPreload;
	protected bool m_bPreloadHookSubscribed; // runtime: OnSlotChangePreload subscribed to slot-change events
	protected ref array<int> m_aPreloadObservers = {}; // RplIdentity values that currently hold a preload observer (leak tracking)
	
	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableGarbageSystem;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFriendliesSpectatorOnly;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFreezeTimeShootingForbiden;
	
	[Attribute("1", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableArmaVision;
	
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

	// Set in OnGameEnd to suppress OnPostCompPlayerDisconnected during the end-of-session
	// disconnect burst. SCR_EditorManagerCore::OnGameEnd nulls m_aEditorEntities, so
	// pushing disconnect events after that dereferences the null map -> ~100 VMEs.
	protected bool m_bGameEnded;

	// ------------------------------------------ Events ------------------------------------------
	
	override void EOnInit(IEntity owner)
	{
        super.EOnInit(owner);

		// Player faction callbacks can run before OnGameStart. Register the
		// loadout manager here so they never encounter a missing manager.
		if (GetGame().InPlayMode() && Replication.IsServer() && !GetGame().GetLoadoutManager())
			GetGame().SpawnEntity(SCR_LoadoutManager);
        
        if (!GetGame().InPlayMode() || !Replication.IsServer()){
            return;
        }
        World world = GetGame().GetWorld();
        world.FindSystem(SCR_GarbageSystem).Enable(!m_bDisableGarbageSystem);
	}

	// The lobby intentionally has no RespawnSystemComponent. The base methods
	// call m_pRespawnSystemComponent without a null check, so retain their
	// event dispatch but omit the unavailable respawn callbacks.
	override void OnPlayerAuditSuccess(int iPlayerID)
	{
		m_OnPlayerAuditSuccess.Invoke(iPlayerID);
		foreach (SCR_BaseGameModeComponent component : m_aAdditionalGamemodeComponents)
			component.OnPlayerAuditSuccess(iPlayerID);
	}

	protected override void OnPlayerRegistered(int playerId)
	{
		m_OnPlayerRegistered.Invoke(playerId);
		foreach (SCR_BaseGameModeComponent component : m_aAdditionalGamemodeComponents)
			component.OnPlayerRegistered(playerId);

		if (RplSession.Mode() == RplMode.Listen && playerId > 1)
			OnPlayerAuditSuccess(playerId);
	}
	
	override void OnGameEnd()
	{
		m_bGameEnded = true;
		// FIX (TIMER LEAK): Stop all self-rescheduling timers that live on the global callqueue.
		// On server restart the GameMode entity is destroyed, but GetGame().GetCallqueue()
		// survives — keeping the old instance alive and running stale callbacks.
		GetGame().GetCallqueue().Remove(ForceFramerate);
		GetGame().GetCallqueue().Remove(VoNReconcileTick);
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		super.OnGameEnd();
	}
	
	// ---- Anti-cheat diagnostics (server-only) ----
	// Format a single-line player identity for [PS_AntiCheat] server logs.
	// Returns: "player=ID name='Name' uuid='UUID' platform=STEAM roles=ADMIN"
	static string PS_AntiCheatPlayerIdentity(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return string.Format("player=%1", playerId);
		string name = pm.GetPlayerName(playerId);
		string uuid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		string platformStr = SCR_Global.GetPlatformName(pm.GetPlatformKind(playerId));
		EPlayerRole roles = pm.GetPlayerRoles(playerId);
		string roleStr = "NONE";
		if (roles & EPlayerRole.ADMINISTRATOR)
			roleStr = "ADMIN";
		return string.Format("player=%1 name='%2' uuid='%3' platform=%4 roles=%5",
			playerId, name, uuid, platformStr, roleStr);
	}

	override void OnGameStart()
	{
		super.OnGameStart();

		// Server startup: log build info once so it appears in every session's server log.
		if (Replication.IsServer())
		{
			ServerInfo serverInfo = GetGame().GetServerInfo();
			string srvName = "unknown";
			string srvScenario = "unknown";
			int srvPlayerLimit = -1;
			bool srvIsModded = false;
			if (serverInfo)
			{
				srvName = serverInfo.GetName();
				srvScenario = serverInfo.GetScenarioName();
				srvPlayerLimit = serverInfo.GetPlayerLimit();
				srvIsModded = serverInfo.IsModded();
			}
			PrintFormat("[PS_AntiCheat] SERVER START: experimentalBuild=%1 crossPlay=%2 serverName='%3' scenario='%4' playerLimit=%5 isModded=%6",
				GetGame().IsExperimentalBuild(),
				GetGame().IsCrossPlayEnabled(),
				srvName,
				srvScenario,
				srvPlayerLimit,
				srvIsModded);
		}

		// Disable vanilla team-kill auto-kick: the lobby mod handles its own kill tracking
		// (PS_KillListManager / broadcast kill feed) and admins manage punishments manually.
		SCR_AdditionalGameModeSettingsComponent additionalSettings = SCR_AdditionalGameModeSettingsComponent.GetInstance();
		if (additionalSettings && additionalSettings.IsTeamKillingPunished())
			additionalSettings.SetEnableTeamKillPunishment_S(false);

		// Vanilla SCR_PlayerLoadoutComponent logs "Loadout manager is missing in the world!"
		// on every player spawn when no SCR_LoadoutManager entity exists. Spawn one if the
		// world/prefab did not include a BaseLoadoutManagerComponent.
		if (!GetGame().GetLoadoutManager())
			GetGame().SpawnEntity(SCR_LoadoutManager);

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
		/*
		string loadSave = GameSessionStorage.s_Data.Get("SCR_SaveFileManager_FileNameToLoad");
		if (loadSave != "")
		{
			SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
			saveManager.Load(loadSave);
		}
		*/
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
			int currentFramerate;
			video.Get("MaxFps", currentFramerate);
			if (currentFramerate != m_iForceMenuFramerate)
			{
				video.Set("MaxFps", m_iForceMenuFramerate);
				GetGame().UserSettingsChanged();
			}
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
		time = time * 1000;
		m_iFreezeTime += time;
		m_fCurrentFreezeTime += time;
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(m_fCurrentFreezeTime);
		
		FreezeTimerAdvance_Notify();
	}
	
	void FreezeTimerEnd()
	{
		// Idempotent: every group leader's PS_SquadsReadyWidget independently detects "all squads ready" and RPCs
		// this to the server (RplRcver.Server), so without the guard FreezeTimerEnd_Notify - and the 5s end-countdown
		// restart - would fire once PER caller (the duplicate "freeze time end" notifications). Fire exactly once per
		// freeze period; m_bFreezeEndTriggered is reset in StartGame when a new freeze begins.
		if (m_bFreezeEndTriggered)
			return;
		m_bFreezeEndTriggered = true;

		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(5000);
		
		FreezeTimerEnd_Notify();
	}

	void EditorClosed()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

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

		// FIX BUG-44 (ref: PodvalLobbyCrashFix/Scripts/Game/Modded/CrashFix_PS_GameModeCoop.c): мертвый игрок,
		// закрывший GM-редактор, зависал в трупе — SwitchFromObserver выше уже снёс камеру/меню спектатора,
		// а ванильный fallthrough возвращал управление в уничтоженное тело. Пересоздаём спектатор сразу,
		// с позицией от трупа. Проверка по базовому SCR_DamageManagerComponent (покрывает и Character-наследника).
		SCR_DamageManagerComponent deadDmg = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (deadDmg && deadDmg.GetState() == EDamageState.DESTROYED)
		{
			playableController.SwitchToObserver(entity);
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
		invoker = chatPanelManager.GetCommandInvoker("cmc");
		invoker.Insert(CopyAllMarkersToClipboard_Callback);
		invoker = chatPanelManager.GetCommandInvoker("lmc");
		invoker.Insert(LoadAllMarkersToClipboard_Callback);
	}
	
	
	void CopyAllMarkersToClipboard_Callback(SCR_ChatPanel panel, string data)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		array<SCR_MapMarkerBase> markers = markerMgr.GetStaticMarkers();
		
		PS_MapMarkersBaseJson mapMarkers = new PS_MapMarkersBaseJson();
		foreach (SCR_MapMarkerBase marker : markers)
		{
			PS_MapMarkerBaseJson markerJson = marker.PS_GetMapMarkerBaseJson();
			mapMarkers.m_aMapMarkers.Insert(markerJson);
		}
		JsonSaveContext saveContext = new JsonSaveContext();
		saveContext.WriteValue("", mapMarkers);
		System.ExportToClipboard(saveContext.SaveToString());
	}
	
	
	void LoadAllMarkersToClipboard_Callback(SCR_ChatPanel panel, string data)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		
		if (GetState() != SCR_EGameModeState.BRIEFING)
			return;
		
		string json = System.ImportFromClipboard();
		
		JsonLoadContext loadContext = new JsonLoadContext();
		loadContext.LoadFromString(json);
		
		PS_MapMarkersBaseJson mapMarkers = new PS_MapMarkersBaseJson();
		loadContext.ReadValue("", mapMarkers);
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		foreach (PS_MapMarkerBaseJson markerJson : mapMarkers.m_aMapMarkers)
		{
			SCR_MapMarkerBase marker = markerJson.GetMapMarkerBase();
			marker.SetMarkerFactionFlags(0);
			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager)
			{
				Faction markerOwnerFaction = SCR_FactionManager.SGetPlayerFaction(GetGame().GetPlayerController().GetPlayerId());
				if (markerOwnerFaction)
					marker.AddMarkerFactionFlags(factionManager.GetFactionIndex(markerOwnerFaction));
			}
			
			markerMgr.InsertStaticMarker(marker, false, false);
		}
	}
	
	void FreezeTimerAdvance_Callback(SCR_ChatPanel panel, string data)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerAdvance(data.ToInt());
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
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		// Broadcast admin lock: /fte ends freeze time, so block other admins from
		// sending /adv (which would now advance to DEBRIEFING) for 5 seconds.
		int myId = playerController.GetPlayerId();
		float now = GetGame().GetWorld().GetWorldTime();
		Rpc(RPC_BroadcastAdvAdminLock, myId, now);
		RPC_BroadcastAdvAdminLock(myId, now);
		
		playableController.FreezeTimerEnd();
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

	// Admin lock: tracks which admin last sent /adv and when, to prevent a second admin
	// from advancing within 5s. Same admin can retry freely (they know they already advanced).
	// Each client maintains its own copy via RPC_BroadcastAdvAdminLock (Broadcast RPC).
	protected static int s_iLastAdvAdminPlayerId = 0;
	protected static float s_fLastAdvAdminTime = 0;
	void AdvanceStage_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) return;
		if (!playableController) return;

		// Admin lock: if a DIFFERENT admin used /adv within the last 5 seconds, block this one.
		// Same admin can retry freely — they already know they advanced.
		int myId = playerController.GetPlayerId();
		float now = GetGame().GetWorld().GetWorldTime();
		if (s_iLastAdvAdminPlayerId != 0 && s_iLastAdvAdminPlayerId != myId && (now - s_fLastAdvAdminTime) < 5000)
		{
			Print(string.Format("[PS_GameModeCoop] /adv blocked: admin %1 already advanced %2ms ago", s_iLastAdvAdminPlayerId, now - s_fLastAdvAdminTime));
			return;
		}

		// Broadcast the admin lock so all clients know this admin just used /adv.		Rpc(RPC_BroadcastAdvAdminLock, myId, now);
		RPC_BroadcastAdvAdminLock(myId, now);

		// Safety: during freeze time in GAME state, "/adv" acts as "/fte" (end freeze time)
		// instead of advancing to DEBRIEFING. Prevents admins from accidentally ending the match
		// when they only meant to end freeze time (common typo since both are short chat commands).
		if (GetState() == SCR_EGameModeState.GAME && !IsFreezeTimeEnd())
		{
			playableController.FreezeTimerEnd();
			return;
		}

		playableController.AdvanceGameState(SCR_EGameModeState.NULL);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_BroadcastAdvAdminLock(int adminPlayerId, float timestamp)
	{
		s_iLastAdvAdminPlayerId = adminPlayerId;
		s_fLastAdvAdminTime = timestamp;
	}

	void PlayGameConfig_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "")
			return;
		Resource resource = BaseContainerTools.LoadContainer(data);
		if (!resource)
			return;
		GameStateTransitions.RequestScenarioChangeTransition(data, "", "");
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
		// Anti-cheat: log every connection with full identity (server-only).
		if (Replication.IsServer())
			PrintFormat("[PS_AntiCheat] CONNECT: %1", PS_AntiCheatPlayerIdentity(playerId));

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		playableManager.SetPlayerName(playerId, name);

		// TODO: remove CallLater
		#ifdef WORKBENCH
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 500, false, playerId);
		#else
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId);
		#endif

		// Restore reconnecting players' faction/slot AFTER vanilla SCR_ReconnectComponent has run
		// (it applies on audit success, shortly after connect, and would otherwise leave the player
		// factionless -> wrong-side markers). No-op for genuinely fresh joins (no cached GUID data).
		GetGame().GetCallqueue().CallLater(playableManager.RestorePlayerReconnectData, 2500, false, playerId);

		// Briefing preload: a reconnecting / late-joining player arrives on a fresh connection, so (re)place
		// their preload observer at their slot AFTER the reconnect-restore above has re-applied it. Idempotent
		// + self-guarded (no-op outside briefing / when off / when un-slotted).
		if (Replication.IsServer() && m_bBriefingPreload)
			GetGame().GetCallqueue().CallLater(PreloadObserverForPlayer_S, 3000, false, playerId);

		// Start the menu-phase VoN reconcile safety-net (no-op if already running or in GAME). See VoNReconcileTick.
		if (Replication.IsServer())
			StartVoNReconcile();

		m_OnPlayerConnected.Invoke(playerId);
	}

	protected override bool HandlePlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		if (!m_OnHandlePlayerKilled)
			m_OnHandlePlayerKilled = new ScriptInvoker();
		m_OnHandlePlayerKilled.Invoke(playerId, playerEntity, killerEntity, killer);

		// Defer the broadcast by 0ms so it runs at the start of the next frame rather than
		// from the engine's AfterSimulate/strict-Rpl-cycle context (Rpc() / local broadcast
		// invoke are silently dropped when invoked from inside AfterSimulate during a kill).
		GetGame().GetCallqueue().CallLater(BroadcastKillEventData, 0, false, playerId, playerEntity, killerEntity, killer);

		// The lobby deliberately has no RespawnSystemComponent. Returning true would
		// enter SCR_BaseGameMode.OnPlayerKilled(), which blindly dereferences its
		// missing m_pRespawnSystemComponent. The event above already feeds the lobby's
		// kill export and broadcast; mark the vanilla path as handled.
		return false;
	}

	void BroadcastKillEventData(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		if (!Replication.IsServer())
			return;

		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		int killerPlayerId = killer.GetInstigatorPlayerID();

		PS_KillInfo killInfo = new PS_KillInfo();
		killInfo.m_iVictimPlayerId = playerId;
		killInfo.m_iKillerPlayerId = killerPlayerId;

		if (playerId > 0 && pm)
		{
			killInfo.m_sVictimName = pm.GetPlayerName(playerId);

			// Victim's squad/group name - resolved now, while the slot is still assigned (the slot is
			// released ~200ms later by TryRespawn -> SwitchToInitialEntity, after this 0ms-deferred broadcast).
			RplId victimPlayable = pm.GetPlayableByPlayer(playerId);
			if (victimPlayable != RplId.Invalid())
			{
				int callsign = pm.GetGroupCallsignByPlayable(victimPlayable);
				// Remembered faction - the live one is already cleared to "" by the death->spectator switch.
				SCR_Faction vFaction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(pm.GetPlayerFactionKeyRemembered(playerId)));
				if (vFaction && callsign >= 0)
					killInfo.m_sVictimSquad = PS_GroupHelper.GroupCallsignToGroupName(vFaction, callsign);
			}
		}
		if (killerPlayerId > 0 && pm)
		{
			killInfo.m_sKillerName = pm.GetPlayerName(killerPlayerId);
		}

		PS_KillListManager killListManager = PS_KillListManager.GetInstance();
		if (killListManager)
			killListManager.FillKillInfoWeaponAndDistance(playerEntity, killerEntity, killerPlayerId, killInfo);

		if (playerId > 0 && killerPlayerId > 0 && pm)
		{
			// Use the REMEMBERED faction: a just-killed victim is switched to the body-less spectator state in
			// the SAME frame, which clears their live faction to "" (PS_PlayableManager slot-apply) BEFORE this
			// 0ms-deferred broadcast runs. So GetPlayerFactionKey(victim) is empty here and same-faction kills
			// were wrongly counted as regular kills. The remembered map keeps the last non-empty faction.
			FactionKey victimFaction = pm.GetPlayerFactionKeyRemembered(playerId);
			FactionKey killerFaction = pm.GetPlayerFactionKeyRemembered(killerPlayerId);
			if (victimFaction != "" && victimFaction == killerFaction && playerId != killerPlayerId)
				killInfo.m_bIsTeamKill = true;
		}

		string killData = BuildKillData(killInfo);
		PrintFormat("[PS_GameModeCoop] BroadcastKillEventData victimId=%1 killerId=%2 dataLen=%3", killInfo.m_iVictimPlayerId, killInfo.m_iKillerPlayerId, killData.Length());
		Rpc(RPC_BroadcastKillEvent, killData);
		RPC_BroadcastKillEvent(killData);
	}

	protected string BuildKillData(PS_KillInfo killInfo)
	{
		// 9 fields: victimId|killerId|victimName|killerName|victimSquad|ammo|distance|hitZoneGroup|isTeamKill
		// The client parses this with Split("|", parts, true), which DROPS empty tokens, so empty string
		// fields are sent as "-" (KillDataField) to keep the field count stable - otherwise indices shift and
		// the whole kill is discarded. Numeric fields are never empty. PS_KillInfo treats "-" as empty.
		string killData;
		killData = killInfo.m_iVictimPlayerId.ToString();
		killData = killData + "|" + killInfo.m_iKillerPlayerId.ToString();
		killData = killData + "|" + KillDataField(killInfo.m_sVictimName);
		killData = killData + "|" + KillDataField(killInfo.m_sKillerName);
		killData = killData + "|" + KillDataField(killInfo.m_sVictimSquad);
		killData = killData + "|" + KillDataField(killInfo.m_sAmmoType);
		killData = killData + "|" + killInfo.m_fDistance.ToString();
		killData = killData + "|" + killInfo.m_eLastHitZoneGroup.ToString();
		string tk;
		if (killInfo.m_bIsTeamKill)
			tk = "1";
		else
			tk = "0";
		killData = killData + "|" + tk;
		return killData;
	}

	// Replace an empty string with a "-" placeholder and strip any "|" so it cannot break the field framing.
	protected string KillDataField(string value)
	{
		if (value == "")
			return "-";
		value.Replace("|", " ");
		return value;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_BroadcastKillEvent(string killData)
	{
		PS_KillListManager killListManager = PS_KillListManager.GetInstance();
		if (!killListManager)
		{
			Print("[PS_GameModeCoop] RPC_BroadcastKillEvent: PS_KillListManager instance is null");
			return;
		}
		killListManager.RPC_KillEvent(killData);
	}

	// Update state for disconnected and start timer if need (DO NOT DELETE CONTROLED ENTITY)
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Anti-cheat: log every disconnect with identity + reason (server-only).
		if (Replication.IsServer())
			PrintFormat("[PS_AntiCheat] DISCONNECT: %1 cause=%2 timeout=%3",
				PS_AntiCheatPlayerIdentity(playerId), cause, timeout);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		// Cancel any pending zombie RestorePlayerReconnectData for this playerId. If the player
		// disconnects BEFORE the 2500ms reconnect-restore delay fires, the uncancelled CallLater
		// would execute on the dead playerId — consuming the GUID cache and permanently assigning
		// the slot to a ghost ID, locking the real player out on their next reconnect.
		// Note: Callqueue.Remove() in Enfusion only accepts a function ref (no extra args), so
		// we flag the playerId as cancelled and RestorePlayerReconnectData checks it on entry.
		playableManager.CancelPendingReconnectRestore(playerId);
		// Cache faction/slot by GUID now, while the playerId-keyed state still exists, so the player
		// keeps their side (and map markers) when they reconnect under a new playerId.
		playableManager.StorePlayerReconnectData(playerId);
		playableManager.SetPlayerState(playerId, PS_EPlayableControllerState.Disconected);
		if (m_iReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iReconnectTime, false, playerId);

		// Body-less: delete this player's VoN proxy (a reconnecting player gets a fresh one).
		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (vonRoomsManager)
			vonRoomsManager.RemoveProxy_S(playerId);

		IEntity controlledEntity = playerController.GetControlledEntity();
		if (controlledEntity) {
			RplComponent rpl = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
			rpl.GiveExt(RplIdentity.Local(), false);
		}

		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);

		// NOTE: vanilla would call m_pRespawnSystemComponent.OnPlayerDisconnected_S here, but this
		// lobby disables the respawn system (it is a no-op stub). The stub's modded no-op override is
		// also bypassed at runtime by another addon re-modding SCR_RespawnSystemComponent, so the
		// vanilla body runs and dereferences a null m_SpawnLogic - ~124 server VMEs per session.
		// The respawn system does nothing here, so we simply do not call it.

		// Guard: SCR_EditorManagerCore::OnGameEnd nulls m_aEditorEntities, so pushing disconnect
		// events after that dereferences the null map → 32+ VMEs per session-end disconnect burst.
		// Both the additional-gamemode-component iteration AND the post-component invoker are gated
		// behind m_bGameEnded: the editor core is an additional gamemode component and its own
		// OnPlayerDisconnected dereferences the same null m_aEditorEntities. At end-of-session the
		// vanilla cleanup is unnecessary anyway (the world is tearing down).
		if (!m_bGameEnded)
		{
			foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
			{
				comp.OnPlayerDisconnected(playerId, cause, timeout);
			}

			SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(
				SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
			if (editorCore)
				m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
		}

		if (IsMaster())
		{
			if (controlledEntity)
			{
				if (SCR_ReconnectComponent.GetInstance())
				{
					if (SCR_ReconnectComponent.GetInstance().HandlePlayerDisconnect(playerId, cause))	// if conditions to allow reconnect pass, skip the entity delete
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

	// Parking position for initial (VoN) entities of players without a death position
	// (lobby phase and JIP spectators), also their streaming relevance center.
	// Planar XZ grid: network streaming relevance is 2D (x, z), so stacking players vertically
	// puts them all into the same streaming cell and every client there streams the whole
	// cluster of parked bodies at once. Keep every player on a unique XZ spot instead.
	// Keep the height within sane world bounds: extreme heights (100km+) break position replication.
	static vector GetInitialEntityPosition(int playerId)
	{
		return Vector(
			1000 * Math.Mod(playerId, 10),
			10000,
			1000 * Math.Floor(playerId / 10));
	}

	void SpawnInitialEntity(int playerId)
	{
		#ifdef WORKBENCH
		IEntity WBCharacter = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (WBCharacter)
			return;
		#endif

		// The player may have disconnected during the spawn delay (rampant on a busy server with
		// reconnect churn). Bail before spawning so we neither deref a null controller nor leak a body.
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		// Body-less: spawn the player's VoN proxy (menu/spectator voice host) instead of a
		// controlled InitialPlayer character. The player controls NOTHING in the lobby - the
		// proxy is an uncontrolled, replicated-to-all entity, not a streaming body.
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (VoNRoomsManager)
		{
			VoNRoomsManager.SpawnProxy_S(playerId);
			VoNRoomsManager.RestoreRoom(playerId);

			// Route EVERY fresh connection through the same phase logic reconnect uses, so an un-slotted
			// joiner lands in #PS-VoNRoom_Global (the shared factionless pool) instead of the raw "" channel
			// RestoreRoom defaults to. "" and Global have DIFFERENT encryption keys, so a "" joiner could
			// hear neither the Global crowd (briefing-unassigned, dead spectators) nor be heard by them.
			// For reconnects this is refined ~2.5s later by RestorePlayerReconnectData (into the restored
			// slot's faction channel, or kept Global if still un-slotted); for genuinely fresh joins that
			// restore is a no-op, so this call is the only thing that moves them out of "". Server-only:
			// it sets the faction key and moves rooms, both server-authoritative.
			if (Replication.IsServer())
			{
				AssignPhaseVoiceChannel(playerId);
				// Diagnostic: if this connection is a reconnect, log the immediate (pre-slot-restore) channel and
				// start the timer; RestorePlayerReconnectData logs the matching "slot restored after Xms" line.
				PS_PlayableManager pm = PS_PlayableManager.GetInstance();
				if (pm)
					pm.TraceReconnectConnect(playerId);
			}
		}
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
		if (!m_playableManager)
			return;
		SCR_AIGroup aiGroup = m_playableManager.GetPlayerGroupByPlayable(respawnData.m_Id);
		if (!aiGroup)
			return;
		SCR_AIGroup playabelGroup = aiGroup.GetSlave();
		if (!playabelGroup)
			return;
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

		// Body-less spectator: the player keeps their corpse as the controlled entity; tell the owning
		// client to open the spectator camera + menu (client-local) and move its VoN to the global
		// channel. See PS_PlayableControllerComponent.RPC_EnterSpectator.
		SendPlayerToSpectator_S(playerId);
	}

	// Server: move the player to the global VoN channel and trigger their client-side spectator camera.
	void SendPlayerToSpectator_S(int playerId)
	{
		if (!Replication.IsServer())
			return;

		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
		{
			vonMgr.MoveToRoom(playerId, "", "#PS-VoNRoom_Global");
			// Re-tune every machine's copy of this player's VoN proxy once their death/slot state has
			// replicated. The immediate ApplyRadioKey inside MoveToRoom can run on receivers before
			// PS_IsMenuSpeaker(playerId) reads true there (the corpse's IsDead replicates separately), which
			// would leave the proxy parked = nobody hears the spectator. RestoreRoom re-broadcasts the
			// channel, re-applying the key on every machine when the state has settled.
			GetGame().GetCallqueue().CallLater(vonMgr.RestoreRoom, 1500, false, playerId);
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;
		PS_PlayableControllerComponent ctrl = pc.PS_GetPLayableComponent();
		if (ctrl)
		{
			// Issue 3: the spectator keeps controlling their corpse, whose radios are still on the in-game
			// faction net - power them down so the dead player no longer hears living teammates' radio
			// chatter (they keep the menu/spectator net via their VoN proxy). Re-applied once after the
			// death/control state has settled, same reasoning as RestoreRoom above.
			ctrl.DisableBodyVoNRadios();
			GetGame().GetCallqueue().CallLater(ctrl.DisableBodyVoNRadios, 1500, false);
			ctrl.EnterSpectatorOwner();
		}
	}

	// If after m_iReconnectTime player still disconnected release playable
	void RemoveDisconnectedPlayer(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_EPlayableControllerState state = playableManager.GetPlayerState(playerId);
		if (state != PS_EPlayableControllerState.Disconected)
			return;
		// Don't release a slot a reconnecting player has already re-claimed: their GUID reservation
		// re-links the slot to a NEW playerId, while this stale id stays "Disconnected". Only release if
		// this id still actually holds the slot.
		RplId playable = playableManager.GetPlayableByPlayer(playerId);
		if (playable != RplId.Invalid() && playableManager.GetPlayerByPlayable(playable) != playerId)
			return;
		playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		// FIX (GHOST NAMES): do NOT MoveToRoom here. RemoveProxy_S already called
		// RemovePlayerFromChannel which fully removed this disconnected player from
		// m_mPlayersChannel on all machines. Calling MoveToRoom would re-add the entry
		// via RPC_SetPlayerChannel, re-creating the stale entry that causes ghost names
		// in the voice-chat UI for late-joining clients. On reconnect the player gets a
		// NEW playerId and AssignPhaseVoiceChannel handles the fresh channel assignment.
	}

	// Move a player to the VoN channel for the current briefing/lobby slot state (group / command / global).
	// Shared by the BRIEFING state-change loop AND reconnect: a reconnecting player gets a NEW playerId whose
	// channel state is empty, so without re-applying this they default to the "" channel - out of their group,
	// and any two such reconnects would share "" (cross-faction leak). Keyed by the SLOT faction (always set)
	// + the unique group id (GetGroupVonRoomName), never the colliding callsign.
	void AssignPhaseVoiceChannel(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (!playableManager || !vonRoomsManager)
			return;
		RplId playableId = playableManager.GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
		{
			playableManager.SetPlayerFactionKey(playerId, "");
			vonRoomsManager.MoveToRoom(playerId, "", "#PS-VoNRoom_Global");
			return;
		}
		FactionKey slotFactionKey = playableManager.GetSlotFactionForPlayer(playerId);
		if (slotFactionKey != playableManager.GetPlayerFactionKey(playerId) && slotFactionKey != "")
			playableManager.SetPlayerFactionKey(playerId, slotFactionKey);
		// FIX (DRIFT / MoveToRoom guard): pass empty faction key and let MoveToRoom's guard read
		// the AUTHORITATIVE slot faction at call time. During busy slot-selection (70+ players picking
		// rapidly), the faction read above can go stale before MoveToRoom executes. By always passing
		// "", the guard always re-reads from the slot, eliminating the ping-pong drift pattern.
		// The factionless Global path above is unaffected ("" + "#PS-VoNRoom_Global" = no guard).
		if (playableManager.IsPlayerGroupLeader(playerId) || m_bPublicCommandBriefing)
			vonRoomsManager.MoveToRoom(playerId, "", "#PS-VoNRoom_Command");
		else
			vonRoomsManager.MoveToRoom(playerId, "", playableManager.GetGroupVonRoomName(playableId));
	}

	protected bool m_bVoNReconcileTickRunning = false;
	// Server safety-net for the "widget fine, audio wrong" divergence, LOBBY / BRIEFING ONLY. The menu VoN channel
	// is written only by discrete events (pick / briefing / reconnect / death / manual join / group+role change),
	// so any change that slips past those could strand a player's audio on the wrong channel while the widget
	// recomputes fresh. Every few seconds this re-keys ONLY players whose channel drifted onto a FOREIGN faction's
	// room - it leaves same-faction manual room choices (join-other-squad) and the factionless Global/Local pools
	// alone, so it fixes cross-faction leaks (incl. guard-race orphans and any forced enemy-channel join via the
	// unvalidated move RPC) without breaking legitimate voice. The loop is driven by OnGameStateChanged: it runs in
	// menu phases and is fully STOPPED during GAME (no pending timer at all - zero cost), where voice is parked /
	// spectator-owned anyway.
	void StartVoNReconcile()
	{
		if (!Replication.IsServer() || m_bVoNReconcileTickRunning)
			return;
		if (GetState() == SCR_EGameModeState.GAME)
			return;
		m_bVoNReconcileTickRunning = true;
		GetGame().GetCallqueue().CallLater(VoNReconcileTick, 5000, false);
	}
	void StopVoNReconcile()
	{
		m_bVoNReconcileTickRunning = false;
		GetGame().GetCallqueue().Remove(VoNReconcileTick); // drop the pending tick so nothing fires during GAME
	}
	void VoNReconcileTick()
	{
		if (!Replication.IsServer())
			return;
		if (GetState() == SCR_EGameModeState.GAME)
		{
			m_bVoNReconcileTickRunning = false;
			return; // fully stop during GAME - do NOT reschedule (OnGameStateChanged(BRIEFING) restarts it)
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (playableManager && vonRoomsManager)
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int playerId : players)
			{
				FactionKey authFaction = playableManager.GetSlotFactionForPlayer(playerId);
				string channelKey = vonRoomsManager.GetPlayerChannel(playerId);
				if (authFaction == "")
				{
					// Un-slotted / ungrouped: legitimately factionless, belongs in the shared Global pool. If a
					// become-slotless transition (deselect / free / kick / disconnect) left them in a FACTION-scoped
					// room (#PS-VoNRoom_Faction, or a leftover group/command room), pull them back to Global.
					// IsForeignFactionChannel(...,"") is false for the factionless pools (Global / Local / Public /
					// ""), so a manual Public join or Local "deafen" is preserved.
					if (vonRoomsManager.IsForeignFactionChannel(channelKey, ""))
					{
						Print(string.Format("[PS_VoN] Reconcile: slotless player %1 stranded in faction room '%2' - re-keying to Global", playerId, channelKey), LogLevel.WARNING);
						AssignPhaseVoiceChannel(playerId);
					}
					continue;
				}
				if (vonRoomsManager.IsForeignFactionChannel(channelKey, authFaction))
				{
					Print(string.Format("[PS_VoN] Reconcile: player %1 drifted to '%2' but faction is '%3' - re-keying to prevent cross-faction leak", playerId, channelKey, authFaction), LogLevel.WARNING);
					AssignPhaseVoiceChannel(playerId);
				}
			}
		}

		GetGame().GetCallqueue().CallLater(VoNReconcileTick, 5000, false); // continue only while in a menu phase
	}

	override void OnGameStateChanged()
	{
		super.OnGameStateChanged();

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		SCR_EGameModeState state = GetState();
		m_OnGameStateChange.Invoke(state);

		// Run the VoN reconcile loop ONLY during menu phases (lobby / briefing); fully stop it in GAME so it
		// costs nothing while the match runs. This (re)starts it each time we return to a menu phase too.
		if (Replication.IsServer())
		{
			if (state == SCR_EGameModeState.GAME)
				StopVoNReconcile();
			else
				StartVoNReconcile();
		}

		// Tell the VoN audit tick whether we're in GAME phase. During GAME, the audit skips
		// parked (alive) proxies — vanilla radios handle voice for them and they cannot drift.
		// Spectator (menu speaker) proxies are still audited since they're LIVE on Global.
		// Runs on ALL machines (server + clients) since the audit runs everywhere.
		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
			vonMgr.SetVoNGamePhase(state == SCR_EGameModeState.GAME);

		switch (state)
		{
			case SCR_EGameModeState.BRIEFING: // Force move to voice rooms
				// Reserve slots from briefing onward: hold a disconnected player's slot indefinitely so a
				// reconnect in any phase from briefing returns them to their playable. Previously this hold
				// only switched on at game start (StartGame), so a disconnect during a long briefing could
				// release the slot before the player got back.
				if (Replication.IsServer())
					m_iReconnectTime = m_iReconnectTimeAfterBriefing;
				// Per-player channel assignment is shared with reconnect (AssignPhaseVoiceChannel) so a
				// reconnecting player lands in their group/command channel instead of the "" default.
				foreach (int playerId : playerIds)
					AssignPhaseVoiceChannel(playerId);
				if (m_bHolsterWeapon)
					playableManager.HolsterWeapons();

				// Briefing preload (opt-in, server): stream each slotted player's surroundings now,
				// staggered, so the briefing->game control switch finds them already streamed instead of
				// bursting all at once at game start. The OnPlayerPlayableChange hook keeps each observer on
				// the player's CURRENT slot as they re-pick / reconnect during briefing.
				if (Replication.IsServer() && m_bBriefingPreload)
				{
					if (!m_bPreloadHookSubscribed)
					{
						playableManager.GetOnPlayerPlayableChange().Insert(OnSlotChangePreload);
						m_bPreloadHookSubscribed = true;
					}
					int preloadDelay = 0;
					foreach (int preloadPlayerId : playerIds)
					{
						if (playableManager.GetPlayableByPlayer(preloadPlayerId) == RplId.Invalid())
							continue;
						GetGame().GetCallqueue().CallLater(PreloadObserverForPlayer_S, preloadDelay, false, preloadPlayerId);
						preloadDelay += 250; // stagger so clusters don't all start streaming together
					}
				}
				// Client reference: snapshot local observer count once the staggered server inserts finish.
				if (!Replication.IsServer() && m_bBriefingPreload)
					GetGame().GetCallqueue().CallLater(LogClientObserverCountLater, 12000, false, "briefing+12s");
				break;
			case SCR_EGameModeState.GAME:
				// Control of the slot character now provides relevance, so drop the temporary preload
				// observers a few seconds after the switch settles.
				if (Replication.IsServer() && m_bBriefingPreload)
					GetGame().GetCallqueue().CallLater(RemoveBriefingPreloadObservers, 5000, false);
				// Client reference: snapshot after the server's teardown should have run (should be back to baseline).
				if (!Replication.IsServer() && m_bBriefingPreload)
					GetGame().GetCallqueue().CallLater(LogClientObserverCountLater, 6500, false, "game+6.5s post-removal");
				break;
		}
	}

	// Briefing preload (server): put a temporary per-connection MPObserver at the player's slot so the
	// engine streams that area in during the calm briefing. Removed by RemoveBriefingPreloadObservers once
	// the player controls the slot character at GAME. Same static/temporary shape vanilla uses to preload a
	// spawn (SCR_SpawnRequestComponent) - NOT the moving whole-map spectator observer that flooded.
	void PreloadObserverForPlayer_S(int playerId)
	{
		if (!m_bBriefingPreload || GetState() != SCR_EGameModeState.BRIEFING)
			return; // off, or briefing ended during the stagger/delay window
		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
			return; // disconnected; the engine drops a closed connection's observers itself
		// Idempotent: drop any prior point (old slot) first so repeated calls - initial pass, slot change,
		// reconnect - never stack observers on one connection.
		int identity = pc.GetRplIdentity();
		RemovePreloadObserver_S(identity, playerId);
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		RplId playableId = playableManager.GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
			return; // no slot (spectator-to-be) -> nothing to preload
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(playableId));
		if (!rpl)
			return;
		IEntity character = rpl.GetEntity();
		if (!character)
			return;
		vector pos = character.GetOrigin();
		InsertPreloadObserver_S(identity, playerId, pos);
	}

	// Slot changed during briefing (a player re-picked, or a reconnect re-applied their slot): move that
	// player's preload observer to the new slot for their CURRENT connection.
	void OnSlotChangePreload(int playerId, RplId playableId)
	{
		PreloadObserverForPlayer_S(playerId);
	}

	void RemoveBriefingPreloadObservers()
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		int removed = 0;
		foreach (int playerId : playerIds)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!pc)
				continue;
			int identity = pc.GetRplIdentity();
			if (m_aPreloadObservers.Contains(identity))
			{
				RemovePreloadObserver_S(identity, playerId);
				removed++;
			}
		}
		int leaked = m_aPreloadObservers.Count();
		Print(string.Format("[PS_Preload][SERVER] GAME teardown: removed=%1 for connected players, LEAKED=%2 (inserted but player gone; engine reclaims a closed connection's observers), engineTotal=%3",
			removed, leaked, GetEngineMPObserverCount()), LogLevel.NORMAL);
		if (leaked > 0)
		{
			// Best-effort drop of orphaned (disconnected) entries so our tracking + the engine agree.
			foreach (int orphan : m_aPreloadObservers)
				RplComponent.RemoveMPObserver(orphan);
			m_aPreloadObservers.Clear();
		}
	}

	// --- Briefing-preload MPObserver bookkeeping + leak instrumentation (server) ---
	// m_aPreloadObservers tracks the connection identities that currently hold a preload observer, so the
	// GAME teardown can report any inserted-but-never-removed-by-us (= their player disconnected) = the leak
	// signal. Server actions tagged [PS_Preload][SERVER]; client reference counts tagged [CLIENT].
	void InsertPreloadObserver_S(int identity, int playerId, vector pos)
	{
		RplComponent.InsertMPObserver(identity, pos[0], pos[2]);
		if (!m_aPreloadObservers.Contains(identity))
			m_aPreloadObservers.Insert(identity);
		Print(string.Format("[PS_Preload][SERVER] +observer player=%1 conn=%2 at (%3, %4) | active=%5 engineTotal=%6",
			playerId, identity, pos[0], pos[2], m_aPreloadObservers.Count(), GetEngineMPObserverCount()), LogLevel.NORMAL);
	}

	void RemovePreloadObserver_S(int identity, int playerId)
	{
		RplComponent.RemoveMPObserver(identity);
		bool had = m_aPreloadObservers.Contains(identity);
		m_aPreloadObservers.RemoveItem(identity);
		if (had)
			Print(string.Format("[PS_Preload][SERVER] -observer player=%1 conn=%2 | active=%3",
				playerId, identity, m_aPreloadObservers.Count()), LogLevel.NORMAL);
	}

	int GetEngineMPObserverCount()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return -1;
		array<vector> observers = {};
		return world.GetMPObservers(observers);
	}

	// Client reference snapshot (CallLater'd from OnGameStateChanged): logs THIS machine's engine observer
	// count. InsertMPObserver is a SERVER op, so the authoritative create/delete/leak data is the [SERVER]
	// log; this is the client's local view for cross-reference.
	void LogClientObserverCountLater(string tag)
	{
		Print(string.Format("[PS_Preload][CLIENT] %1: local engine MPObservers=%2", tag, GetEngineMPObserverCount()), LogLevel.NORMAL);
	}

	// Switch to next game state
	// Debounce: if multiple admins send "/adv" in the same ~100ms window, both RPCs arrive at
	// the server before the first one's SetGameModeState takes effect. The second call would
	// re-enter the same case (e.g. BRIEFING → StartGame twice). Use a wall-clock cooldown
	// to drop the duplicate, matching the m_bFreezeEndTriggered pattern FreezeTimerEnd uses.
	protected float m_fLastAdvanceTime = 0;
	void AdvanceGameState(SCR_EGameModeState oldState)
	{
		if (!Replication.IsServer())
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fLastAdvanceTime < 100)
		{
			Print(string.Format("[PS_GameModeCoop] AdvanceGameState: blocked rapid duplicate call (dt=%1ms)", now - m_fLastAdvanceTime));
			return;
		}
		m_fLastAdvanceTime = now;

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
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.ResetGroupReady();
		// Delete admin-closed (-2) characters and locked vehicles IMMEDIATELY at the
		// BRIEFING → GAME transition, before freeze time starts. Unassigned slots are left
		// alive so players can still pick them during the freeze window.
		playableManager.RemoveRedundantUnits(true);
		m_bFreezeEndTriggered = false; // new freeze period - allow the one-shot end notification again
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
			m_fGameStartTime = GetGame().GetWorld().GetWorldTime();
			m_fGameStartElapsedTime = GetElapsedTime();
			Replication.BumpMe();
			removeRestrictedZones();
			if (m_bDisableBuildingModeAfterFreezeTime)
				DisableBuildingMode();
			// Remove unoccupied playables AFTER freeze time ends so players have the full
			// freeze window to pick their slots before unused units are deleted.
			// Admin-closed entities were already deleted at the BRIEFING → GAME transition (StartGame).
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
				playableManager.RemoveRedundantUnits();
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
			if (m_hSquadsReadyWidget)
			{
				m_hSquadsReadyWidget.Destroy();
				m_hSquadsReadyWidget = null;
			}
			return;
		}

		if (m_hFreezeTimeCounter == null)
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{EC8A548C3F53BE4F}UI/layouts/FreezeTime/FreezeTimeCounter.layout");
			m_hFreezeTimeCounter = PS_FreezeTimeCounter.Cast(widget.FindHandler(PS_FreezeTimeCounter));
		}

		// Create squads-ready widget on first freeze-time tick (client-side)
		if (m_hSquadsReadyWidget == null && RplSession.Mode() != RplMode.Dedicated)
			m_hSquadsReadyWidget = PS_SquadsReadyWidget.Create();

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
	ref PS_SquadsReadyWidget m_hSquadsReadyWidget;

	// ------------------------------------------ Global flags ------------------------------------------
	bool IsFreezeTimeEnd()
	{
		return m_fCurrentFreezeTime <= 0;
	}
	
	bool IsDisableTimeEnd()
	{
		return m_fCurrentFreezeTime <= (m_iFreezeTime - m_iDisableTime) && m_fCurrentFreezeTime != 1;
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
		// HARD-DISABLED (flood fix). Force-streaming every playable (rpl.EnableStreaming(false) in
		// PS_PlayableComponent.AddToListWrap) made all ~128 playables ALWAYS-RELEVANT to all ~78 clients
		// on the Podval server = the Replication Flooded/Stalled kick storm. Returning false unconditionally
		// puts playables back on default NDS distance culling (matching the working reference lobbies),
		// regardless of what the m_bDisablePlayablesStreaming attribute is set to.
		//
		// The attribute field is KEPT so QuickTvT's server prefab and the world layers that still set
		// m_bDisablePlayablesStreaming compile/load with no dangling property. To re-enable later, just
		// `return m_bDisablePlayablesStreaming;` again. Spectators still see the action via the separate
		// m_bSpectatorStreamingObserver (GetSpectatorStreamingObserver) per-connection observer path.
		return false;
	}

	bool GetSpectatorStreamingObserver()
	{
		// HARD-DISABLED. The per-connection MPObserver spectator streaming it gated was removed (see
		// PS_PlayableControllerComponent "Spectator streaming observer: REMOVED") - it was the only
		// spectator-streaming mechanism among the reference lobbies (Echo/LiteLobby use none) and the main
		// remaining Replication Flooded/Stalled lever. Returns false regardless of the attribute; the
		// m_bSpectatorStreamingObserver field is KEPT only so prefabs/world layers that set it still load.
		return false;
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
		return m_bDisableArmaVision;
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
	
	int GetDisableTime()
	{
		return m_iDisableTime;
	}
	
	float GetGameStartTime()
	{
		return m_fGameStartTime;
	}

	float GetGameStartElapsedTime()
	{
		return m_fGameStartElapsedTime;
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
