//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_MissionDataManagerClass: ScriptComponentClass
{
	
};


/**
 * @brief Сборщик и экспорт данных матча.
 * @subsystem Lobby | Stats
 * @context Server
 * @entity PS_GameModeCoop
 * @depends PlayableSelector
 * @listens PS_GameModeCoop.GetOnHandlePlayerKilled, GetOnGameStateChange, GetOnPlayerConnected, GetOnPlayerAuditSuccess, SCR_DamageManagerComponent.GetOnDamage
 * @details Собирает слоты по серверным данным PlayableManager, сохраняет полный JSON в профиль сервера и асинхронно отправляет payload статистики.
 */
class PS_MissionDataManager : ScriptComponent
{
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_MissionDataManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_MissionDataManager.Cast(gameMode.FindComponent(PS_MissionDataManager));
		else
			return null;
	}
	
	ref map<EntityID, RplId> m_EntityToRpl = new map<EntityID, RplId>();
	ref map<RplId, SCR_DamageManagerComponent> m_RplToDamageManager = new map<RplId, SCR_DamageManagerComponent>();
	ref map<RplId, bool> m_RegisteredVehicles = new map<RplId, bool>();
	ref map<int, bool> m_playerSaved = new map<int, bool>();
	ref set<RplId> m_DeadEntities = new set<RplId>();
	PS_PlayableManager m_PlayableManager;
	PS_ObjectiveManager m_ObjectiveManager;
	PS_GameModeCoop m_GameModeCoop;
	FactionManager m_FactionManager;
	PlayerManager m_PlayerManager;
	ref PS_MissionDataConfig m_Data = new PS_MissionDataConfig();
	int m_iInitTimer = 100;
	protected ref RestCallback m_WebsiteCallback;
	protected bool m_bMissionExportScheduled;
	
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().CallLater(LateInit, 0, false);
		GetGame().GetCallqueue().CallLater(AwaitFullInit, 0, true);
	}
	
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(AwaitFullInit);
			GetGame().GetCallqueue().Remove(LateInit);
			GetGame().GetCallqueue().Remove(FinalizeMissionExport);
		}
		m_WebsiteCallback = null;

		if (m_GameModeCoop)
		{
			if (m_GameModeCoop.GetOnHandlePlayerKilled())
				m_GameModeCoop.GetOnHandlePlayerKilled().Remove(OnPlayerKilled);
			if (m_GameModeCoop.GetOnGameStateChange())
				m_GameModeCoop.GetOnGameStateChange().Remove(OnGameStateChanged);
			if (m_GameModeCoop.GetOnPlayerConnected())
				m_GameModeCoop.GetOnPlayerConnected().Remove(OnPlayerConnected);
			if (m_GameModeCoop.GetOnPlayerAuditSuccess())
				m_GameModeCoop.GetOnPlayerAuditSuccess().Remove(OnPlayerAuditSuccess);
		}

		foreach (RplId targetId, SCR_DamageManagerComponent damageManager : m_RplToDamageManager)
		{
			if (damageManager && damageManager.GetOnDamage())
				damageManager.GetOnDamage().Remove(OnDamaged);
		}
		m_RplToDamageManager.Clear();
		m_EntityToRpl.Clear();
		m_RegisteredVehicles.Clear();

		super.OnDelete(owner);
	}
	void RegisterVehicle(Vehicle vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		RplComponent rplComponent = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!rplComponent)
			return; // not replicated (e.g. decoration), nothing to track

		RplId vehicleId = rplComponent.Id();
		if (m_RegisteredVehicles.Contains(vehicleId))
			return;

		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(vehicle.FindComponent(FactionAffiliationComponent));
		SCR_DamageManagerComponent damageManagerComponent = SCR_DamageManagerComponent.Cast(vehicle.FindComponent(SCR_DamageManagerComponent));

		PS_MissionDataVehicle vehicleData = new PS_MissionDataVehicle();
		vehicleData.EntityId = vehicleId;
		vehicleData.PrefabPath = vehicle.GetPrefabData().GetPrefabName();
		if (editableVehicleComponent)
		{
			SCR_UIInfo info = editableVehicleComponent.GetInfo();
			if (info)
				vehicleData.EditableName = WidgetManager.Translate("%1", info.GetName());
		}
		if (factionAffiliationComponent)
		{
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			if (faction)
				vehicleData.VehicleFactionKey = WidgetManager.Translate("%1", faction.GetFactionKey());
		}
		if (damageManagerComponent && !m_RplToDamageManager.Contains(vehicleId) && damageManagerComponent.GetOnDamage())
		{
			damageManagerComponent.GetOnDamage().Insert(OnDamaged);
			m_RplToDamageManager.Insert(vehicleId, damageManagerComponent);
		}

		m_Data.Vehicles.Insert(vehicleData);
		if (!m_EntityToRpl.Contains(vehicle.GetID()))
			m_EntityToRpl.Insert(vehicle.GetID(), vehicleId);
		m_RegisteredVehicles.Insert(vehicleId, true);
	}
	void OnDamaged(BaseDamageContext damageContext)
	{
		if (!damageContext)
			return;
		IEntity target = damageContext.hitEntity;
		Instigator instigator = damageContext.instigator;
		if (target && instigator)
		{
			int playerId = instigator.GetInstigatorPlayerID();
			if (playerId == -1)
				return;

			EntityID entityID = target.GetID();
			if (!m_EntityToRpl.Contains(entityID))
				return;
			RplId rplId = m_EntityToRpl.Get(entityID);

			IEntity instigatorEntity = instigator.GetInstigatorEntity();
			// Fire/incendiary ticks keep the instigator's playerId but can lose the entity (player
			// despawned/died) - GetInstigatorEntity() then returns null. Guard before GetOrigin().
			if (!instigatorEntity)
				return;
			vector instigatorOrigin = instigatorEntity.GetOrigin();
			vector targetOrigin = target.GetOrigin();
			float distance = vector.Distance(instigatorOrigin, targetOrigin);

			GetGame().GetCallqueue().Call(SaveDamageEventWithDistance, playerId, rplId, damageContext.damageValue, distance);
		}
	}

	void SaveDamageEventWithDistance(int playerId, RplId targetId, float value, float distance)
	{
		SCR_DamageManagerComponent damageManagerComponent = m_RplToDamageManager.Get(targetId);
		if (!damageManagerComponent)
			return;
		EDamageState state = damageManagerComponent.GetState();
		float time = GetGame().GetWorld().GetWorldTime();
		distance = Math.Round(distance);

		PS_MissionDataDamageEvent missionDataDamageEvent = new PS_MissionDataDamageEvent();
		missionDataDamageEvent.m_iPlayerId = playerId;
		missionDataDamageEvent.TargetId = targetId;
		missionDataDamageEvent.DamageValue = value;
		missionDataDamageEvent.TargetState = state;
		missionDataDamageEvent.Time = time;
		missionDataDamageEvent.Distance = distance;
		m_Data.DamageEvents.Insert(missionDataDamageEvent);

		// Kills are recorded SOLELY by OnPlayerKilled now. This damage path used to ALSO insert a kill on
		// DESTROYED, but with the victim id set to the entity RplId (NOT a playerId) - so every player kill landed
		// in m_Data.Kills twice: once here (killer resolves to a player, victim doesn't) and once in
		// OnPlayerKilled. That is what doubled the "kills" column on the website (and why deaths stayed single -
		// this path's victim never resolved). OnPlayerKilled has the correct victim+killer playerIds and now also
		// carries the distance, so this insert is removed. (Note: this also dropped non-player/vehicle destructions
		// from the kill log - they were unresolved RplId rows anyway; say so if the site needs vehicle kills.)
	}
	
	void LateInit()
	{
		if (!Replication.IsServer())
			return;

		m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_ObjectiveManager = PS_ObjectiveManager.GetInstance();
		m_FactionManager = GetGame().GetFactionManager();

		if (!m_GameModeCoop)
		{
			Print("PS_MissionDataManager: PS_GameModeCoop not found on owner!", LogLevel.WARNING);
			return;
		}

		if (m_GameModeCoop.GetOnHandlePlayerKilled())
			m_GameModeCoop.GetOnHandlePlayerKilled().Insert(OnPlayerKilled);
		if (m_GameModeCoop.GetOnGameStateChange())
			m_GameModeCoop.GetOnGameStateChange().Insert(OnGameStateChanged);
		if (m_GameModeCoop.GetOnPlayerConnected())
			m_GameModeCoop.GetOnPlayerConnected().Insert(OnPlayerConnected);
		if (m_GameModeCoop.GetOnPlayerAuditSuccess())
			m_GameModeCoop.GetOnPlayerAuditSuccess().Insert(OnPlayerAuditSuccess);

		if (m_PlayerManager)
		{
			array<int> playerIds = {};
			m_PlayerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
				RegisterPlayer(playerId);
		}

		if (RplSession.Mode() != RplMode.Dedicated)
		{
			PlayerController playerController = GetGame().GetPlayerController();
			if (playerController)
				RegisterPlayer(playerController.GetPlayerId());
		}
	}
	void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		int killerId = killer.GetInstigatorPlayerID();
		
		PS_MissionDataPlayerKill missionDataPlayerKill = new PS_MissionDataPlayerKill();
		missionDataPlayerKill.InstigatorId = killerId;
		missionDataPlayerKill.m_iPlayerId = playerId;
		missionDataPlayerKill.Time = GetGame().GetWorld().GetWorldTime();
		missionDataPlayerKill.SystemTime = System.GetUnixTime();
		// Distance (killer -> victim at death), carried over from the now-removed damage-path kill so the kill log
		// keeps it. killerEntity is null for non-instigated deaths -> leave distance at default in that case.
		if (playerEntity && killerEntity)
			missionDataPlayerKill.Distance = Math.Round(vector.Distance(killerEntity.GetOrigin(), playerEntity.GetOrigin()));
		m_Data.Kills.Insert(missionDataPlayerKill);
	}
	
	void AwaitFullInit()
	{
		m_iInitTimer--; // Wait 100 frames, I belive everything can init in 100 frames. maybe...
		if (m_iInitTimer <= 0)
		{
			GetGame().GetCallqueue().Remove(AwaitFullInit);
			InitData();
		}
	}
	
	void OnGameStateChanged(SCR_EGameModeState state)
	{
		if (!Replication.IsServer() || !m_Data)
			return;

		PS_MissionDataStateChangeEvent missionDataStateChangeEvent = new PS_MissionDataStateChangeEvent();
		missionDataStateChangeEvent.State = state;
		missionDataStateChangeEvent.Time = GetGame().GetWorld().GetWorldTime();
		missionDataStateChangeEvent.SystemTime = System.GetUnixTime();
		m_Data.StateEvents.Insert(missionDataStateChangeEvent);

		if (state == SCR_EGameModeState.SLOTSELECTION)
		{
			CollectFactions();
		}
		else if (state == SCR_EGameModeState.GAME)
		{
			CollectFactions();
			SavePlayers();
		}
		else if (state == SCR_EGameModeState.DEBRIEFING)
		{
			// Capture synchronously at the stage boundary before any later teardown can affect live entities.
			CollectFactions();
			SavePlayers();
			if (!m_bMissionExportScheduled)
			{
				m_bMissionExportScheduled = true;
				GetGame().GetCallqueue().Call(FinalizeMissionExport);
			}
		}
	}

	/**
	 * @brief Выполняет финальный сбор и экспорт статистики при завершении матча.
	 * @integration PS_GameModeCoop: вызывается при переходе в DEBRIEFING.
	 * @details Обновляет доступные данные, сохраняет полный локальный JSON и затем отправляет облегчённый payload.
	 */
	void FinalizeMissionExport()
	{
		if (!Replication.IsServer() || !m_Data)
			return;

		DefineScenarioType();
		SaveObjectives();
		WriteToFile();
		SendToWebsite();
	}
	void DefineScenarioType()
	{
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (!missionHeader)
			return;
		m_Data.WorldPath = missionHeader.GetWorldPath();
		m_Data.MissionName = missionHeader.m_sName;
		m_Data.MissionAuthor = missionHeader.m_sAuthor;
		m_Data.MissionDescription = missionHeader.m_sDescription;
		m_Data.ScenarioType = missionHeader.m_sGameMode;
	}

	// POST the collected mission data to the StatSender endpoint. No-op (graceful) when the server has no
	// $profile:StatSender_Config.json, so a standalone PlayableSelector simply writes the JSON file and
	// skips the upload. Moved here from QuickTvT so the whole stats pipeline lives in PlayableSelector.
	/**
	 * @brief Асинхронно отправляет статистику матча в StatSender.
	 * @integration StatSender: JSON POST на адрес из серверной конфигурации.
	 * @fallback При отсутствии конфигурации или обязательных полей локальный JSON остаётся доступен.
	 */
	void SendToWebsite()
	{
		if (!m_Data)
			return;

		StatSender_Config config = new StatSender_Config();
		if (!config.LoadFromFile("$profile:StatSender_Config.json"))
		{
			Print("PS_MissionDataManager: StatSender_Config.json not found - skipping website upload", LogLevel.NORMAL);
			return;
		}
		if (config.Address == "")
		{
			Print("PS_MissionDataManager: StatSender Address is empty - skipping website upload", LogLevel.WARNING);
			return;
		}

		m_Data.Token = config.Token;
		if (m_Data.MissionName == "" || m_Data.Factions.IsEmpty() || m_Data.Token == "" || m_Data.ScenarioType == "")
		{
			Print(string.Format("PS_MissionDataManager: Pre-validation failed (MissionName='%1', Factions=%2, Token='%3', ScenarioType='%4') - aborting website upload",
				m_Data.MissionName, m_Data.Factions.Count(), m_Data.Token != "", m_Data.ScenarioType), LogLevel.ERROR);
			return;
		}

		// MissionSessionReader does not consume these large arrays. Keep them in the local file,
		// but serialize empty arrays for HTTP so the request stays below Enfusion's 1 MB limit.
		ref array<ref PS_MissionDataDamageEvent> backupDamage = m_Data.DamageEvents;
		ref array<ref PS_MissionDataVehicle> backupVehicles = m_Data.Vehicles;
		m_Data.DamageEvents = new array<ref PS_MissionDataDamageEvent>();
		m_Data.Vehicles = new array<ref PS_MissionDataVehicle>();

		JsonSaveContext missionSaveContext = new JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);
		string payload = missionSaveContext.SaveToString();

		m_Data.DamageEvents = backupDamage;
		m_Data.Vehicles = backupVehicles;

		int payloadSize = payload.Length();
		Print(string.Format("PS_MissionDataManager: Prepared website payload (%1 bytes)", payloadSize), LogLevel.NORMAL);
		if (payloadSize >= 1000000)
		{
			Print(string.Format("PS_MissionDataManager: ERROR - Payload exceeds 1MB limit (%1 bytes), skipping upload", payloadSize), LogLevel.ERROR);
			return;
		}

		if (!GetGame().GetRestApi())
		{
			Print("PS_MissionDataManager: REST API is unavailable - skipping website upload", LogLevel.WARNING);
			return;
		}
		RestContext context = GetGame().GetRestApi().GetContext(config.Address);
		if (!context)
		{
			Print(string.Format("PS_MissionDataManager: RestContext is null for %1", config.Address), LogLevel.WARNING);
			return;
		}

		context.SetHeaders("Content-Type,application/json");
		m_WebsiteCallback = new RestCallback();
		m_WebsiteCallback.SetOnSuccess(OnWebsiteUploadDone);
		m_WebsiteCallback.SetOnError(OnWebsiteUploadDone);
		int requestId = context.POST(m_WebsiteCallback, "", payload);
		Print(string.Format("PS_MissionDataManager: Asynchronous website upload initiated (Request ID: %1, size: %2 bytes)", requestId, payloadSize), LogLevel.NORMAL);
	}

	protected void OnWebsiteUploadDone(RestCallback callback)
	{
		m_WebsiteCallback = null;
		if (!callback)
			return;

		int httpCode = callback.GetHttpCode();
		if (httpCode == 200 || httpCode == 201)
			Print(string.Format("PS_MissionDataManager: Website upload succeeded (HTTP %1). Response: %2", httpCode, callback.GetData()), LogLevel.NORMAL);
		else
			Print(string.Format("PS_MissionDataManager: Website upload failed (HTTP %1, RestResult %2). Response: %3", httpCode, callback.GetRestResult(), callback.GetData()), LogLevel.WARNING);
	}

	void OnPlayerConnected(int playerId)
	{
		RegisterPlayer(playerId);
	}

	void OnPlayerAuditSuccess(int playerId)
	{
		RegisterPlayer(playerId);
	}

	void RegisterPlayer(int playerId)
	{
		if (!Replication.IsServer() || !m_Data || playerId <= 0)
			return;
		if (!m_PlayerManager)
			m_PlayerManager = GetGame().GetPlayerManager();
		if (!m_PlayerManager)
			return;

		string name = "";
		if (m_PlayableManager)
			name = m_PlayableManager.GetPlayerName(playerId);
		if (name == "")
			name = m_PlayerManager.GetPlayerName(playerId);

		string guid = "";
		if (GetGame().GetBackendApi())
			guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);

		foreach (PS_MissionDataPlayer existingPlayer : m_Data.Players)
		{
			if (!existingPlayer)
				continue;
			if (existingPlayer.m_iPlayerId != playerId)
				continue;

			if (guid != "" && existingPlayer.GUID == "")
				existingPlayer.GUID = guid;
			if (name != "" && !name.StartsWith("Player") && (existingPlayer.Name == "" || existingPlayer.Name.StartsWith("Player")))
				existingPlayer.Name = name;
			if (!m_playerSaved.Contains(playerId))
				m_playerSaved.Insert(playerId, true);
			return;
		}

		if (guid != "")
		{
			foreach (PS_MissionDataPlayer existingByGuid : m_Data.Players)
			{
				if (!existingByGuid || existingByGuid.GUID != guid)
					continue;
				existingByGuid.m_iPlayerId = playerId;
				if (name != "" && !name.StartsWith("Player"))
					existingByGuid.Name = name;
				if (!m_playerSaved.Contains(playerId))
					m_playerSaved.Insert(playerId, true);
				return;
			}
		}

		if (m_playerSaved.Contains(playerId))
			return;

		PS_MissionDataPlayer player = new PS_MissionDataPlayer();
		player.m_iPlayerId = playerId;
		player.GUID = guid;
		player.Name = name;
		m_Data.Players.Insert(player);
		m_playerSaved.Insert(playerId, true);
	}
	// Save main mission data
	void InitData()
	{
		string localization = "ru_ru";
		WidgetManager.SetLanguage(localization);
		
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (missionHeader)
		{
			//m_Data.MissionPath = missionHeader.GetHeaderResourcePath();
			m_Data.WorldPath = missionHeader.GetWorldPath();
			m_Data.MissionName = missionHeader.m_sName;
			m_Data.MissionAuthor = missionHeader.m_sAuthor;
			m_Data.MissionDescription = missionHeader.m_sDescription;
		}
		
		ChimeraWorld world = GetGame().GetWorld();
		if (world)
		{
			TimeAndWeatherManagerEntity timeAndWeatherManagerEntity = world.GetTimeAndWeatherManager();
			if (timeAndWeatherManagerEntity)
			{
				float time = timeAndWeatherManagerEntity.GetTimeOfTheDay();
				WeatherState weatherState = timeAndWeatherManagerEntity.GetCurrentWeatherState();
				if (weatherState)
				{
					m_Data.MissionWeather = weatherState.GetStateName();
					m_Data.MissionWeatherIcon = weatherState.GetIconPath();
				}
				m_Data.MissionDayTime = time;
			}
		}
		
		PS_MissionDataStateChangeEvent missionDataStateChangeEvent = new PS_MissionDataStateChangeEvent();
		missionDataStateChangeEvent.State = SCR_EGameModeState.SLOTSELECTION;
		missionDataStateChangeEvent.Time = 0;
		missionDataStateChangeEvent.SystemTime = System.GetUnixTime();
		m_Data.StateEvents.Insert(missionDataStateChangeEvent);
		
		#ifdef PS_REPLAYS
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter)
		{
			string replayPath = replayWriter.m_sReplayFileName;
			replayPath.Replace("$profile:Replays/", "");
			m_Data.ReplayPath = replayPath;
		}
		#endif
		
		PS_MissionDescriptionManager missionDescriptionManager = PS_MissionDescriptionManager.GetInstance();
		if (missionDescriptionManager)
		{
			array<PS_MissionDescription> descriptions = new array<PS_MissionDescription>();
			missionDescriptionManager.GetDescriptions(descriptions);
			foreach (PS_MissionDescription description : descriptions)
			{
				if (!description)
					continue;

				PS_MissionDataDescription descriptionData = new PS_MissionDataDescription();
				descriptionData.Title = WidgetManager.Translate("%1", description.m_sTitle);
				descriptionData.DescriptionLayout = description.m_sDescriptionLayout;
				descriptionData.TextData = WidgetManager.Translate("%1", description.m_sTextData);
				descriptionData.VisibleForFactions = description.m_aVisibleForFactions;
				descriptionData.EmptyFactionVisibility = description.m_bEmptyFactionVisibility;
				m_Data.Descriptions.Insert(descriptionData);
			}
		}
		
		CollectFactions();
	}
	
	/**
	 * @brief Собирает полный снимок фракций, групп и playable-слотов.
	 * @integration PlayableSelector: PS_PlayableManager.GetPlayablesSorted и кэшированный FactionKey контейнера.
	 * @fallback Если персонаж или AI-группа ещё не готовы, слот помещается в резервную группу своей фракции.
	 */
	void CollectFactions()
	{
		if (!Replication.IsServer())
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
		if (!playables || playables.IsEmpty())
			return;
		if (!m_FactionManager)
			m_FactionManager = GetGame().GetFactionManager();

		m_Data.Factions.Clear();
		map<Faction, PS_MissionDataFaction> factionsMap = new map<Faction, PS_MissionDataFaction>();
		map<SCR_AIGroup, PS_MissionDataGroup> groupsMap = new map<SCR_AIGroup, PS_MissionDataGroup>();
		map<string, PS_MissionDataGroup> fallbackGroupsMap = new map<string, PS_MissionDataGroup>();
		int addedPlayables = 0;
		int skippedWithoutFaction = 0;

		foreach (PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;

			RplId playableId = playable.GetRplId();
			Faction faction;
			FactionKey factionKey = playable.GetFactionKey();
			if (m_FactionManager && factionKey != "")
				faction = m_FactionManager.GetFactionByKey(factionKey);

			PS_PlayableComponent playableComp = playable.GetPlayableComponent();
			IEntity character;
			if (playableComp)
				character = playableComp.GetOwner();
			if (!faction && character)
			{
				SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
				if (chimeraCharacter)
					faction = chimeraCharacter.GetFaction();
			}
			if (!faction)
			{
				skippedWithoutFaction++;
				continue;
			}

			PS_MissionDataFaction factionData;
			if (!factionsMap.Contains(faction))
			{
				factionData = new PS_MissionDataFaction();
				factionData.Name = WidgetManager.Translate("%1", faction.GetFactionName());
				factionData.Key = WidgetManager.Translate("%1", faction.GetFactionKey());
				Color color = faction.GetFactionColor();
				factionData.FactionColor = string.Format("%1,%2,%3,%4", color.A(), color.R(), color.G(), color.B());
				SCR_Faction scrFaction = SCR_Faction.Cast(faction);
				if (scrFaction)
				{
					color = scrFaction.GetOutlineFactionColor();
					factionData.FactionOutlineColor = string.Format("%1,%2,%3,%4", color.A(), color.R(), color.G(), color.B());
				}
				m_Data.Factions.Insert(factionData);
				factionsMap.Insert(faction, factionData);
			}
			else
			{
				factionData = factionsMap.Get(faction);
			}

			SCR_AIGroup group;
			group = playableManager.GetPlayerGroupByPlayable(playableId);
			if (!group && character)
			{
				AIControlComponent aiComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
				if (aiComponent)
				{
					AIAgent agent = aiComponent.GetAIAgent();
					if (agent)
						group = SCR_AIGroup.Cast(agent.GetParentGroup());
				}
			}

			PS_MissionDataGroup groupData;
			if (group)
			{
				if (!groupsMap.Contains(group))
				{
					groupData = new PS_MissionDataGroup();
					string customName = group.GetCustomName();
					string company, platoon, squad, t, format;
					group.GetCallsigns(company, platoon, squad, t, format);
					groupData.Callsign = playableManager.GetGroupCallsignByPlayable(playableId);
					groupData.CallsignName = WidgetManager.Translate(format, company, platoon, squad, "");
					groupData.Name = WidgetManager.Translate("%1", customName);
					factionData.Groups.Insert(groupData);
					groupsMap.Insert(group, groupData);
				}
				else
				{
					groupData = groupsMap.Get(group);
				}
			}
			else
			{
				string fallbackKey = faction.GetFactionKey() + "_DefaultGroup";
				if (!fallbackGroupsMap.Contains(fallbackKey))
				{
					groupData = new PS_MissionDataGroup();
					groupData.Callsign = 0;
					groupData.CallsignName = "";
					groupData.Name = WidgetManager.Translate("%1", faction.GetFactionName());
					factionData.Groups.Insert(groupData);
					fallbackGroupsMap.Insert(fallbackKey, groupData);
				}
				else
				{
					groupData = fallbackGroupsMap.Get(fallbackKey);
				}
			}

			if (character)
			{
				SCR_DamageManagerComponent damageManagerComponent = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
				if (damageManagerComponent && !m_RplToDamageManager.Contains(playableId) && damageManagerComponent.GetOnDamage())
				{
					damageManagerComponent.GetOnDamage().Insert(OnDamaged);
					m_RplToDamageManager.Insert(playableId, damageManagerComponent);
				}
				if (!m_EntityToRpl.Contains(character.GetID()))
					m_EntityToRpl.Insert(character.GetID(), playableId);
			}

			PS_MissionDataPlayable missionDataPlayable = new PS_MissionDataPlayable();
			missionDataPlayable.EntityId = playableId;
			missionDataPlayable.GroupOrder = groupData.Playables.Count();
			missionDataPlayable.Name = WidgetManager.Translate("%1", playable.GetName());
			missionDataPlayable.RoleName = WidgetManager.Translate("%1", playable.GetRoleName());
			groupData.Playables.Insert(missionDataPlayable);
			addedPlayables++;
		}

		Print(string.Format("PS_MissionDataManager: Collected %1 factions and %2/%3 playables; %4 skipped without a resolvable faction", m_Data.Factions.Count(), addedPlayables, playables.Count(), skippedWithoutFaction), LogLevel.NORMAL);
	}
	void SavePlayers()
	{
		if (!m_PlayableManager)
			m_PlayableManager = PS_PlayableManager.GetInstance();
		if (!m_PlayableManager)
			return;

		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		if (!playables || playables.IsEmpty())
			return;

		if (!m_PlayerManager)
			m_PlayerManager = GetGame().GetPlayerManager();
		if (m_PlayerManager)
		{
			array<int> connectedPlayerIds = {};
			m_PlayerManager.GetPlayers(connectedPlayerIds);
			foreach (int connectedPlayerId : connectedPlayerIds)
				RegisterPlayer(connectedPlayerId);
		}

		m_Data.PlayersToPlayables.Clear();
		map<int, bool> processedPlayers = new map<int, bool>();
		foreach (PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;

			RplId playableId = playable.GetRplId();
			int playerId = m_PlayableManager.GetPlayerByPlayable(playableId);
			if (playerId <= 0)
				playerId = m_PlayableManager.GetPlayerByPlayableRemembered(playableId);
			if (playerId <= 0 || processedPlayers.Contains(playerId))
				continue;

			RegisterPlayer(playerId);
			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.m_iPlayerId = playerId;
			playerToEntity.EntityId = playableId;
			m_Data.PlayersToPlayables.Insert(playerToEntity);
			processedPlayers.Insert(playerId, true);
		}

		foreach (PS_MissionDataPlayer missionPlayer : m_Data.Players)
		{
			if (!missionPlayer)
				continue;
			int playerId = missionPlayer.m_iPlayerId;
			if (playerId <= 0 || processedPlayers.Contains(playerId))
				continue;

			RplId rememberedPlayable = m_PlayableManager.GetPlayableByPlayerRemembered(playerId);
			if (rememberedPlayable == RplId.Invalid())
				continue;

			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.m_iPlayerId = playerId;
			playerToEntity.EntityId = rememberedPlayable;
			m_Data.PlayersToPlayables.Insert(playerToEntity);
			processedPlayers.Insert(playerId, true);
		}

		Print(string.Format("PS_MissionDataManager: Saved %1 player-to-slot associations", m_Data.PlayersToPlayables.Count()), LogLevel.NORMAL);
	}
	void SaveObjectives()
	{
		if (!m_ObjectiveManager)
			m_ObjectiveManager = PS_ObjectiveManager.GetInstance();
		if (!m_FactionManager)
			m_FactionManager = GetGame().GetFactionManager();
		if (!m_Data || !m_ObjectiveManager || !m_FactionManager)
			return;

		array<PS_Objective> objectivesOut = {};
		m_ObjectiveManager.GetObjectives(objectivesOut);
		array<Faction> factionsOut = {};
		m_FactionManager.GetFactionsList(factionsOut);
		m_Data.FactionResults.Clear();

		foreach (Faction faction : factionsOut)
		{
			if (!faction)
				continue;

			FactionKey factionKey = faction.GetFactionKey();
			PS_MissionDataFactionResult factionResult = new PS_MissionDataFactionResult();
			factionResult.ResultFactionKey = factionKey;
			PS_ObjectiveLevel objectiveLevel = m_ObjectiveManager.GetFactionScoreLevel(factionKey);
			if (objectiveLevel)
			{
				factionResult.ResultName = WidgetManager.Translate("%1", objectiveLevel.GetName());
				factionResult.ResultScore = objectiveLevel.GetScore();
			}

			foreach (PS_Objective objective : objectivesOut)
			{
				if (!objective || objective.GetFactionKey() != factionKey)
					continue;

				PS_MissionDataObjective objectiveData = new PS_MissionDataObjective();
				objectiveData.Name = WidgetManager.Translate("%1", objective.GetTitle());
				objectiveData.Completed = objective.GetCompleted();
				objectiveData.Score = objective.GetScore();
				factionResult.Objectives.Insert(objectiveData);
			}
			m_Data.FactionResults.Insert(factionResult);
		}

		Print(string.Format("PS_MissionDataManager: Saved objective results for %1 factions", m_Data.FactionResults.Count()), LogLevel.NORMAL);
	}
	void WriteToFile()
	{
		if (!m_Data)
			return;

		if (m_Data.MissionName == "")
			DefineScenarioType();

		string time = System.GetUnixTime().ToString();
		m_Data.SessionName = string.Format("PS_MissionData_%1.json", time);
		FileIO.MakeDirectory("$profile:Sessions");

		string fileName = string.Format("$profile:Sessions/%1", m_Data.SessionName);
		JsonSaveContext missionSaveContext = new JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);
		bool saved = missionSaveContext.SaveToFile(fileName);
		if (saved)
			Print(string.Format("PS_MissionDataManager: Full session data saved locally to %1", fileName), LogLevel.NORMAL);
		else
			Print(string.Format("PS_MissionDataManager: Failed to save session data to %1", fileName), LogLevel.ERROR);
	}

}
