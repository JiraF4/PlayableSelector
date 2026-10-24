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
		PS_FreezeManager freezeManager = PS_FreezeManager.Cast(FindComponent(PS_FreezeManager));
		if (freezeManager)
			freezeManager.GetOnFreezeEnded().Remove(OnFreezeTimeEnded_Internal);
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

		PS_InputLock.ResetStatic();

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
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.FreezeTimerAdvance_S(time);
	}
	
	void FreezeTimerEnd()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.FreezeTimerEnd_S();
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
		invoker = chatPanelManager.GetCommandInvoker("hf");
		invoker.Insert(HardFreeze_Callback);
		invoker = chatPanelManager.GetCommandInvoker("hardfreeze");
		invoker.Insert(HardFreeze_Callback);
		invoker = chatPanelManager.GetCommandInvoker("hft");
		invoker.Insert(HardFreeze_Callback);
		invoker = chatPanelManager.GetCommandInvoker("hfe");
		invoker.Insert(HardFreezeEnd_Callback);
		invoker = chatPanelManager.GetCommandInvoker("hfte");
		invoker.Insert(HardFreezeEnd_Callback);
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
	
	void FreezeTimerEnd_Callback(SCR_ChatPanel panel, string data)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerEnd();
	}

	void HardFreeze_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		if (GetState() != SCR_EGameModeState.GAME)
			return;

		data = data.Trim();
		if (data == "")
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
			invoker.Invoke(null, "#PS-HardFreeze_Help");
			return;
		}

		int seconds = data.ToInt();
		if (seconds == 0 && data != "0")
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
			invoker.Invoke(null, "#PS-HardFreeze_InvalidArg");
			return;
		}

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (seconds < 0 && freezeManager && !freezeManager.IsHardFreezeActive())
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
			invoker.Invoke(null, "#PS-HardFreeze_NotActive");
			return;
		}

		playableController.HardFreeze(seconds);
	}

	/**
	 * @brief Обработка /hfe: досрочное снятие административной паузы с 5-секундным буфером
	 */
	void HardFreezeEnd_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		if (!PS_PlayersHelper.IsAdminOrServer())
			return;

		if (GetState() != SCR_EGameModeState.GAME)
			return;

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && !freezeManager.IsHardFreezeActive())
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			if (!chatPanelManager)
				return;

			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
			if (invoker)
				invoker.Invoke(null, "#PS-HardFreeze_NotActive");
			return;
		}

		playableController.HardFreeze(0);
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
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (playableController)
			playableController.AdvanceGameState(SCR_EGameModeState.NULL);
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
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.RemoveRestrictedZones_S();
	}

	protected PS_FactionRespawnCount GetFactionRespawnCount(FactionKey factionKey)
	{
		if (m_mFactionRespawnCount.Contains(factionKey))
		{
			return m_mFactionRespawnCount[factionKey];
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! @workaround Enfusion Steam persona duplication when testing multiple local peers
	//! Disambiguates duplicate player names on connect by appending index or '(Peer N)' tag.
	protected string ResolveUniquePlayerName_S(int playerId, string baseName)
	{
		if (baseName.IsEmpty())
			return baseName;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return baseName;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return baseName;

		// Detect if this is local/peer testing (no backend GUID)
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		bool isPeer = guid.IsEmpty();
		string tag = "Peer ";
		if (!isPeer)
			tag = "";

		array<int> playerIds = new array<int>();
		pm.GetPlayers(playerIds);

		array<int> usedIndices = new array<int>();
		bool firstNeedsRename = false;
		int firstId = -1;
		string prefix = baseName + " (" + tag;

		foreach (int existingId : playerIds)
		{
			if (existingId == playerId)
				continue;

			string existingName = playableManager.GetPlayerName(existingId);
			if (existingName.IsEmpty())
				existingName = pm.GetPlayerName(existingId);

			if (existingName == baseName)
			{
				usedIndices.Insert(1);
				firstNeedsRename = true;
				firstId = existingId;
			}
			else if (existingName.IndexOf(prefix) == 0)
			{
				int prefixLen = prefix.Length();
				int closeParen = existingName.IndexOf(")");
				if (closeParen > prefixLen)
				{
					int idx = existingName.Substring(prefixLen, closeParen - prefixLen).ToInt();
					if (idx > 0)
						usedIndices.Insert(idx);
				}
			}
		}

		if (usedIndices.IsEmpty())
			return baseName;

		if (firstNeedsRename && firstId > 0)
		{
			string newFirstName = string.Format("%1 (%21)", baseName, tag);
			playableManager.SetPlayerName(firstId, newFirstName);
		}

		int assignedIdx = 1;
		while (usedIndices.Contains(assignedIdx))
		{
			assignedIdx++;
		}

		string resolvedName = string.Format("%1 (%2%3)", baseName, tag, assignedIdx);
		Print(string.Format("[PS_Lobby] Disambiguated duplicate player name: player %1 ('%2') -> '%3'", playerId, baseName, resolvedName), LogLevel.NORMAL);
		return resolvedName;
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterPlayerName_S(int playerId)
	{
		if (!Replication.IsServer())
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		string name = pm.GetPlayerName(playerId);
		if (name.IsEmpty())
		{
			GetGame().GetCallqueue().CallLater(RegisterPlayerName_S, 200, false, playerId);
			return;
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;

		name = ResolveUniquePlayerName_S(playerId, name);
		playableManager.SetPlayerName(playerId, name);
	}

	protected override void OnPlayerConnected(int playerId)
	{
		// Anti-cheat: log every connection with full identity (server-only).
		if (Replication.IsServer())
			PrintFormat("[PS_AntiCheat] CONNECT: %1", PS_AntiCheatPlayerIdentity(playerId));

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();

		if (Replication.IsServer())
			RegisterPlayerName_S(playerId);

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

		if (Replication.IsServer())
		{
			PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
			if (vonRoomsManager)
			{
				vonRoomsManager.SetPlayerParked_S(playerId, false);
				vonRoomsManager.SetPlayerChannel_S(playerId, PS_VoNRoomsManager.GetLobbyChannelKey());
			}
		}

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

		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (vonRoomsManager && playerId > 0 && GetGame().GetPlayerManager().GetPlayerController(playerId))
			vonRoomsManager.SetPlayerParked_S(playerId, false);

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
		// 10 fields: keep the legacy 9-field prefix and append weaponName for mixed-version parsing.
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
		killData = killData + "|" + KillDataField(killInfo.m_sWeaponName);
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
		SCR_PlayerController playerController;
		if (playerManager)
			playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));

		// Reconnect state and VoN cleanup must survive an already-destroyed controller.
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
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
		}

		// Body-less: delete this player's VoN proxy (a reconnecting player gets a fresh one).
		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (vonRoomsManager)
			vonRoomsManager.RemoveProxy_S(playerId);

		IEntity controlledEntity;
		if (playerController)
		{
			controlledEntity = playerController.GetControlledEntity();
			if (controlledEntity)
			{
				RplComponent rpl = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
				if (rpl)
					rpl.GiveExt(RplIdentity.Local(), false);
			}
		}

		if (m_OnPlayerDisconnected)
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
			if (Replication.IsServer())
			{
				VoNRoomsManager.QueueSlotChannelUpdate_S(playerId);
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
			vonMgr.SetPlayerParked_S(playerId, false);
			vonMgr.SetPlayerChannel_S(playerId, PS_VoNRoomsManager.GetLobbyChannelKey());
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;
		PS_PlayableControllerComponent ctrl = pc.PS_GetPLayableComponent();
		if (ctrl)
		{
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
		// Disconnected IDs stay absent from channel state; reconnect receives a new player ID.
	}

	// Resolve the final slot state on authority after coalescing assignment events.
	void AssignPhaseVoiceChannel(int playerId)
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerManager().GetPlayerController(playerId))
			return;
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager vonRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (!playableManager || !vonRoomsManager)
			return;
		RplId playableId = playableManager.GetPlayableByPlayer(playerId);
		FactionKey slotFactionKey = playableManager.GetSlotFactionForPlayer(playerId);
		if (playableId == RplId.Invalid() || slotFactionKey == "")
		{
			playableManager.SetPlayerFactionKey(playerId, "");
			vonRoomsManager.SetPlayerChannel_S(playerId, PS_VoNRoomsManager.GetLobbyChannelKey());
			return;
		}
		if (slotFactionKey != playableManager.GetPlayerFactionKey(playerId))
			playableManager.SetPlayerFactionKey(playerId, slotFactionKey);
		if (GetState() == SCR_EGameModeState.BRIEFING && playableManager.IsPlayerGroupLeader(playerId))
			vonRoomsManager.SetPlayerChannel_S(playerId, PS_VoNRoomsManager.GetCommandChannelKey(slotFactionKey));
		else
		{
			string groupChannel = playableManager.GetGroupVonRoomName(playableId);
			if (groupChannel == "")
				groupChannel = PS_VoNRoomsManager.GetFactionChannelKey(slotFactionKey);
			vonRoomsManager.SetPlayerChannel_S(playerId, groupChannel);
		}
	}

	override void OnGameStateChanged()
	{
		super.OnGameStateChanged();

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		SCR_EGameModeState state = GetState();
		m_OnGameStateChange.Invoke(state);

		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (Replication.IsServer() && vonMgr)
		{
			vonMgr.UpdateAllParked_S();
			// Possession can settle after the state-change invokers complete.
			GetGame().GetCallqueue().CallLater(vonMgr.UpdateAllParked_S, 0, false);
		}

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
	protected int m_iLastAdvAdminPlayerId = -1;
	protected float m_fLastAdvAdminTime = -10000.0;
	protected const float ADMIN_INTERLOCK_TIMEOUT_MS = 5000.0;

	// Manual requests share a server-owned interlock; automatic transitions use AdvanceGameState.
	void AdvanceGameState_S(int callerPlayerId, SCR_EGameModeState state)
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerManager())
			return;
		if (callerPlayerId <= 0 || !SCR_Global.IsAdmin(callerPlayerId))
			return;

		SCR_EGameModeState currentState = GetState();
		switch (currentState)
		{
			case SCR_EGameModeState.PREGAME:
			case SCR_EGameModeState.SLOTSELECTION:
			case SCR_EGameModeState.CUTSCENE:
			case SCR_EGameModeState.BRIEFING:
			case SCR_EGameModeState.GAME:
			case SCR_EGameModeState.DEBRIEFING:
				break;
			default:
				return;
		}
		if (state != SCR_EGameModeState.NULL && state != currentState)
			return;
		if (currentState == SCR_EGameModeState.SLOTSELECTION && m_bShowCutscene && !m_CutsceneManager)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fLastAdvanceTime < 100.0)
			return;

		if (m_iLastAdvAdminPlayerId > 0 && callerPlayerId != m_iLastAdvAdminPlayerId && now - m_fLastAdvAdminTime < ADMIN_INTERLOCK_TIMEOUT_MS)
		{
			PrintFormat("[PS_GameModeCoop] /adv blocked: admin %1 locked by admin %2 (%3ms ago)",
				callerPlayerId, m_iLastAdvAdminPlayerId, now - m_fLastAdvAdminTime);
			return;
		}

		if (currentState == SCR_EGameModeState.GAME && !IsFreezeTimeEnd())
		{
			TryAdminFreezeEnd_S(callerPlayerId);
			return;
		}

		m_iLastAdvAdminPlayerId = callerPlayerId;
		m_fLastAdvAdminTime = now;
		AdvanceGameState(state);
	}

	void TryAdminFreezeEnd_S(int callerPlayerId)
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerManager())
			return;
		if (callerPlayerId <= 0 || !SCR_Global.IsAdmin(callerPlayerId))
			return;
		if (GetState() != SCR_EGameModeState.GAME)
			return;

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager || freezeManager.GetFreezePhase() != PS_EFreezePhase.ORDINARY)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (m_iLastAdvAdminPlayerId > 0 && callerPlayerId != m_iLastAdvAdminPlayerId && now - m_fLastAdvAdminTime < ADMIN_INTERLOCK_TIMEOUT_MS)
		{
			PrintFormat("[PS_GameModeCoop] /fte blocked: admin %1 locked by admin %2 (%3ms ago)",
				callerPlayerId, m_iLastAdvAdminPlayerId, now - m_fLastAdvAdminTime);
			return;
		}

		m_iLastAdvAdminPlayerId = callerPlayerId;
		m_fLastAdvAdminTime = now;
		FreezeTimerEnd();
	}

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
		// Delete admin-closed (-2) characters and locked vehicles IMMEDIATELY at the
		// BRIEFING → GAME transition, before freeze time starts. Unassigned slots are left
		// alive so players can still pick them during the freeze window.
		if (playableManager)
			playableManager.RemoveRedundantUnits(true);
		StartGameMode();

		PS_FreezeManager freezeManager = PS_FreezeManager.Cast(FindComponent(PS_FreezeManager));
		if (Replication.IsServer() && freezeManager)
		{
			freezeManager.GetOnFreezeEnded().Remove(OnFreezeTimeEnded_Internal);
			freezeManager.GetOnFreezeEnded().Insert(OnFreezeTimeEnded_Internal);
			freezeManager.StartFreeze_S();
		}
	}

	void OnFreezeTimeEnded_Internal()
	{
		if (!Replication.IsServer())
			return;

		m_fGameStartTime = GetGame().GetWorld().GetWorldTime();
		m_fGameStartElapsedTime = GetElapsedTime();
		Replication.BumpMe();
		if (m_bDisableBuildingModeAfterFreezeTime)
			DisableBuildingMode();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
			playableManager.RemoveRedundantUnits();
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

	// ------------------------------------------ Global flags ------------------------------------------
	bool IsFreezeTimeEnd()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			return freezeManager.IsFreezeTimeEnd();

		return true;
	}
	
	bool IsDisableTimeEnd()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			return freezeManager.IsDisableTimeEnd();

		return true;
	}

	bool IsHardFreezeActive()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			return freezeManager.IsHardFreezeActive();

		return false;
	}

	int GetDisableTime()
	{
		return m_iDisableTime;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		float prevElapsed = m_fTimeElapsed;
		super.EOnFrame(owner, timeSlice);

		if (IsHardFreezeActive() && GetState() == SCR_EGameModeState.GAME)
		{
			m_fTimeElapsed = prevElapsed;
		}
	}

	override protected void SetLocalControls(bool enabled)
	{
		bool held = IsHardFreezeActive();
		if (PS_InputLock.IsLocked() || held)
			enabled = false;

		super.SetLocalControls(enabled);
		PS_InputLock.EnforceView(held);
	}

	void ~PS_GameModeCoop()
	{
		PS_InputLock.ResetStatic();
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
		m_iFreezeTime = freezeTime;
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.SetFreezeTime_S(freezeTime);

		RPC_SetFreezeTime(freezeTime);
		Rpc(RPC_SetFreezeTime, freezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFreezeTime(int freezeTime)
	{
		m_iFreezeTime = freezeTime;
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
