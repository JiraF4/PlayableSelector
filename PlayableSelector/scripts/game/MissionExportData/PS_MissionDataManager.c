//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_MissionDataManagerClass: ScriptComponentClass
{
	
};


/**
 * @brief Менеджер сбора и экспорта данных сессии (статистика, игроки, слоты, задачи).
 * @subsystem Lobby | Stats
 * @context Server
 * @entity PS_GameMode_Lobby.et / PS_GameModeCoop
 * @depends PlayableSelector
 * @listens PS_GameModeCoop.GetOnHandlePlayerKilled, GetOnGameStateChange, GetOnPlayerAuditSuccess
 * @details Формирует итоговые структуры матча и выполняет локальное сохранение в JSON, а также асинхронный экспорт через REST API на веб-сайт статистики.
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
	
	ref map<int, bool> m_playerSaved = new map<int, bool>();
	ref set<RplId> m_DeadEntities = new set<RplId>();
	PS_PlayableManager m_PlayableManager;
	PS_ObjectiveManager m_ObjectiveManager;
	PS_GameModeCoop m_GameModeCoop;
	FactionManager m_FactionManager;
	PlayerManager m_PlayerManager;
	ref PS_MissionDataConfig m_Data = new PS_MissionDataConfig();
	int m_iInitTimer = 40;

	// Strong reference: движок Enfusion удаляет асинхронный RestCallback, если на него нет сильной ссылки
	protected ref RestCallback m_WebsiteCallback;
	
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().CallLater(LateInit, 0, false);
		GetGame().GetCallqueue().CallLater(AwaitFullInit, 0, true);
	}
	
	override void OnDelete(IEntity owner)
	{
		if (m_GameModeCoop)
		{
			if (m_GameModeCoop.GetOnHandlePlayerKilled())
				m_GameModeCoop.GetOnHandlePlayerKilled().Remove(OnPlayerKilled);
			if (m_GameModeCoop.GetOnGameStateChange())
				m_GameModeCoop.GetOnGameStateChange().Remove(OnGameStateChanged);
			if (m_GameModeCoop.GetOnPlayerAuditSuccess())
				m_GameModeCoop.GetOnPlayerAuditSuccess().Remove(OnPlayerAuditSuccess);
		}
		super.OnDelete(owner);
	}
	
	void RegisterVehicle(Vehicle vehicle)
	{
		if (!Replication.IsServer())
			return;
		RplComponent rplComponent = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!rplComponent)
			return; // not replicated (e.g. decoration), nothing to track
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(vehicle.FindComponent(FactionAffiliationComponent));
		RplId vehicleId = rplComponent.Id();
		
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
			{
				vehicleData.VehicleFactionKey = WidgetManager.Translate("%1", faction.GetFactionKey());
			}
		}
		
		m_Data.Vehicles.Insert(vehicleData);
	}
	
	void LateInit()
	{
		m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_ObjectiveManager = PS_ObjectiveManager.GetInstance();
		m_FactionManager = GetGame().GetFactionManager();
		
		m_GameModeCoop.GetOnHandlePlayerKilled().Insert(OnPlayerKilled);
		m_GameModeCoop.GetOnGameStateChange().Insert(OnGameStateChanged);
		m_GameModeCoop.GetOnPlayerAuditSuccess().Insert(OnPlayerAuditSuccess);
		if (RplSession.Mode() != RplMode.Dedicated) 
			OnPlayerAuditSuccess(GetGame().GetPlayerController().GetPlayerId());
	}
	
	void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		int killerId = killer.GetInstigatorPlayerID();
		
		PS_MissionDataPlayerKill missionDataPlayerKill = new PS_MissionDataPlayerKill();
		missionDataPlayerKill.InstigatorId = killerId;
		missionDataPlayerKill.m_iPlayerId = playerId;
		missionDataPlayerKill.Time = GetGame().GetWorld().GetWorldTime();
		missionDataPlayerKill.SystemTime = System.GetUnixTime();
		if (playerEntity && killerEntity)
			missionDataPlayerKill.Distance = Math.Round(vector.Distance(killerEntity.GetOrigin(), playerEntity.GetOrigin()));
		m_Data.Kills.Insert(missionDataPlayerKill);
	}
	
	void AwaitFullInit()
	{
		m_iInitTimer--; // Wait 40 frames, I belive everything can init in 40 frames. maybe...
		if (m_iInitTimer <= 0)
		{
			GetGame().GetCallqueue().Remove(AwaitFullInit);
			InitData();
		}
	}
	
	void OnGameStateChanged(SCR_EGameModeState state)
	{
		PS_MissionDataStateChangeEvent missionDataStateChangeEvent = new PS_MissionDataStateChangeEvent();
		missionDataStateChangeEvent.State = state;
		missionDataStateChangeEvent.Time = GetGame().GetWorld().GetWorldTime();
		missionDataStateChangeEvent.SystemTime = System.GetUnixTime();
		m_Data.StateEvents.Insert(missionDataStateChangeEvent);
		
		if (state == SCR_EGameModeState.GAME)
		{
			if (m_Data.Factions.IsEmpty() || (m_PlayableManager && GetTotalPlayablesCount() < m_PlayableManager.GetPlayables().Count()))
				CollectFactions();

			SavePlayers();
		}
		else if (state == SCR_EGameModeState.DEBRIEFING)
		{
			GetGame().GetCallqueue().Call(FinalizeMissionExport);
		}
	}

	/**
	 * @brief Финализация экспорта статистики при переходе в дебрифинг.
	 * @issue BUG-09
	 * @cause Раздельные вызовы в Callqueue прерывались при возникновении ошибки в промежуточном методе.
	 * @solution Единый безопасный поток: метаданные сценария -> задачи -> проверка фракций -> локальный файл -> асинхронный веб-экспорт.
	 */
	void FinalizeMissionExport()
	{
		DefineScenarioType();
		SaveObjectives();
		if (m_Data.Factions.IsEmpty() || (m_PlayableManager && GetTotalPlayablesCount() < m_PlayableManager.GetPlayables().Count()))
			CollectFactions();
		if (m_Data.PlayersToPlayables.IsEmpty())
			SavePlayers();

		string time = System.GetUnixTime().ToString();
		m_Data.SessionName = string.Format("PS_MissionData_%1.json", time);

		WriteToFile();
		SendToWebsite();
	}

	// Fill the scenario/world identifiers from the mission header for the website payload.
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

	/**
	 * @brief Асинхронная отправка данных миссии на веб-сайт статистики.
	 * @issue REST-1MB / FREEZE-01
	 * @cause Синхронный POST_now блокировал сервер на несколько секунд, а превышение 1 МБ сбрасывало запрос движком.
	 * @solution Асинхронный POST через RestCallback с предварительной валидацией и отсутствием блокировки Game Thread.
	 */
	void SendToWebsite()
	{
		StatSender_Config config = new StatSender_Config();
		bool isLoaded = config.LoadFromFile("$profile:StatSender_Config.json");
		if (!isLoaded)
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

		// Пре-валидация в соответствии с требованиями MissionSessionReader.php (preValidate)
		if (m_Data.MissionName == "" || m_Data.Factions.IsEmpty() || m_Data.Token == "" || m_Data.ScenarioType == "")
		{
			Print(string.Format("PS_MissionDataManager: Pre-validation failed (MissionName='%1', Factions=%2, Token='%3', ScenarioType='%4') - aborting website upload",
				m_Data.MissionName, m_Data.Factions.Count(), m_Data.Token != "", m_Data.ScenarioType), LogLevel.ERROR);
			return;
		}

		JsonSaveContext missionSaveContext = new JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);
		string payload = missionSaveContext.SaveToString();

		int payloadSize = payload.Length();
		Print(string.Format("PS_MissionDataManager: Prepared website payload (%1 bytes)", payloadSize), LogLevel.NORMAL);

		if (payloadSize >= 1000000)
		{
			Print(string.Format("PS_MissionDataManager: ERROR - Payload exceeds 1MB limit (%1 bytes), dropping to prevent engine crash", payloadSize), LogLevel.ERROR);
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

		int reqId = context.POST(m_WebsiteCallback, "", payload);
		Print(string.Format("PS_MissionDataManager: Asynchronous website upload initiated (Request ID: %1, size: %2 bytes)", reqId, payloadSize), LogLevel.NORMAL);
	}

	protected void OnWebsiteUploadDone(RestCallback cb)
	{
		m_WebsiteCallback = null;
		if (!cb)
			return;

		int httpCode = cb.GetHttpCode();
		if (httpCode == 200 || httpCode == 201)
		{
			Print(string.Format("PS_MissionDataManager: Website upload succeeded (HTTP %1). Response: %2", httpCode, cb.GetData()), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("PS_MissionDataManager: Website upload failed (HTTP %1, RestResult %2). Response: %3", httpCode, cb.GetRestResult(), cb.GetData()), LogLevel.WARNING);
		}
	}
	
	void OnPlayerAuditSuccess(int playerId)
	{
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid != "")
		{
			foreach (PS_MissionDataPlayer existingPlayer : m_Data.Players)
			{
				if (existingPlayer.GUID == guid)
				{
					existingPlayer.m_iPlayerId = playerId;
					string currentName = m_PlayerManager.GetPlayerName(playerId);
					if (currentName != "" && !currentName.StartsWith("Player"))
						existingPlayer.Name = currentName;
					if (!m_playerSaved.Contains(playerId))
						m_playerSaved.Insert(playerId, true);
					return;
				}
			}
		}

		if (m_playerSaved.Contains(playerId))
			return;
		
		string name = m_PlayerManager.GetPlayerName(playerId);
		
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
	
	void SavePlayables()
	{
		CollectFactions();
	}
	
	int GetTotalPlayablesCount()
	{
		int count = 0;
		foreach (PS_MissionDataFaction faction : m_Data.Factions)
		{
			if (!faction || !faction.Groups)
				continue;
			foreach (PS_MissionDataGroup group : faction.Groups)
			{
				if (!group || !group.Playables)
					continue;
				count += group.Playables.Count();
			}
		}
		return count;
	}

	/**
	 * @brief Полный сбор структуры фракций, отделений и слотов миссии для экспорта статистики.
	 * @subsystem Lobby | Core
	 * @context Server
	 * @depends PlayableSelector
	 * @details Опрашивает PS_PlayableManager и формирует полную иерархию фракций и отделений.
	 *          Использует устойчивые метаданные PS_PlayableContainer и привязку групп,
	 *          не завися от состояния AIAgent и физического существования живого персонажа.
	 */
	void CollectFactions()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;

		array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
		if (!playables || playables.IsEmpty())
			return;

		m_Data.Factions.Clear();

		map<Faction, PS_MissionDataFaction> factionsMap = new map<Faction, PS_MissionDataFaction>();
		map<SCR_AIGroup, PS_MissionDataGroup> groupsMap = new map<SCR_AIGroup, PS_MissionDataGroup>();
		map<string, PS_MissionDataGroup> fallbackGroupsMap = new map<string, PS_MissionDataGroup>();

		foreach (PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;

			RplId playableId = playable.GetRplId();

			// 1. Определение фракции слота через кэшированный FactionKey контейнера
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
				SCR_ChimeraCharacter chimeraChar = SCR_ChimeraCharacter.Cast(character);
				if (chimeraChar)
					faction = chimeraChar.GetFaction();
			}

			if (!faction)
				continue;

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
					color = scrFaction.GetOutlineFactionColor();
				factionData.FactionOutlineColor = string.Format("%1,%2,%3,%4", color.A(), color.R(), color.G(), color.B());

				m_Data.Factions.Insert(factionData);
				factionsMap.Insert(faction, factionData);
			}
			else
			{
				factionData = factionsMap.Get(faction);
			}

			// 2. Определение группы слота через PlayableManager
			SCR_AIGroup group = playableManager.GetPlayerGroupByPlayable(playableId);
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
					string callsign = WidgetManager.Translate(format, company, platoon, squad, "");

					groupData.Callsign = playableManager.GetGroupCallsignByPlayable(playableId);
					groupData.CallsignName = callsign;
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
				// Резервная группа для слотов без группы SCR_AIGroup (слот гарантированно сохраняется)
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


			// 4. Добавление слота в группу
			PS_MissionDataPlayable missionDataPlayable = new PS_MissionDataPlayable();
			missionDataPlayable.EntityId = playableId;
			missionDataPlayable.GroupOrder = groupData.Playables.Count();
			missionDataPlayable.Name = WidgetManager.Translate("%1", playable.GetName());
			missionDataPlayable.RoleName = WidgetManager.Translate("%1", playable.GetRoleName());

			groupData.Playables.Insert(missionDataPlayable);
		}

		Print(string.Format("PS_MissionDataManager: Successfully collected %1 factions and %2 playables", m_Data.Factions.Count(), GetTotalPlayablesCount()), LogLevel.NORMAL);
	}
	
	/**
	 * @brief Сохранение соответствия игроков занятым слотам для экспорта статистики.
	 * @subsystem Lobby | Core
	 * @context Server
	 * @depends PlayableSelector
	 * @details Сохраняет текущих и запомненных игроков (погибших/дисконнектившихся) по каждому слоту,
	 *          а также проверяет всех зарегистрированных игроков m_Data.Players на наличие запомненного слота,
	 *          гарантируя, что все участники матча попадут в PlayersToPlayables.
	 */
	void SavePlayers()
	{
		if (!m_PlayableManager)
			return;

		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		if (!playables)
			return;

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

			if (playerId <= 0)
				continue;

			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.m_iPlayerId = playerId;
			playerToEntity.EntityId = playableId;

			m_Data.PlayersToPlayables.Insert(playerToEntity);
			processedPlayers.Insert(playerId, true);
		}

		// Дополнительная проверка по всем участникам: если игрок заходил в игру и брал слот,
		// но его слот освободился при смерти, восстанавливаем его по личному запомненному слоту
		foreach (PS_MissionDataPlayer missionPlayer : m_Data.Players)
		{
			if (!missionPlayer)
				continue;

			int pid = missionPlayer.m_iPlayerId;
			if (pid <= 0 || processedPlayers.Contains(pid))
				continue;

			RplId rememberedPlayable = m_PlayableManager.GetPlayableByPlayerRemembered(pid);
			if (rememberedPlayable != RplId.Invalid())
			{
				PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
				playerToEntity.m_iPlayerId = pid;
				playerToEntity.EntityId = rememberedPlayable;

				m_Data.PlayersToPlayables.Insert(playerToEntity);
				processedPlayers.Insert(pid, true);
			}
		}
	}
	
	void SaveObjectives()
	{
		if (!m_ObjectiveManager || !m_FactionManager)
			return;

		array<PS_Objective> objectivesOut = {};
		m_ObjectiveManager.GetObjectives(objectivesOut);
		array<Faction> outFactions = {};
		m_FactionManager.GetFactionsList(outFactions);
		foreach (Faction faction : outFactions)
		{
			if (!faction)
				continue;

			FactionKey factionKey = faction.GetFactionKey();
			
			PS_MissionDataFactionResult missionDataFactionResult = new PS_MissionDataFactionResult();
			missionDataFactionResult.ResultFactionKey = factionKey;
			PS_ObjectiveLevel objectiveLevel = m_ObjectiveManager.GetFactionScoreLevel(factionKey);
			if (objectiveLevel)
			{
				missionDataFactionResult.ResultName = WidgetManager.Translate("%1", objectiveLevel.GetName());
				missionDataFactionResult.ResultScore = objectiveLevel.GetScore();
			}
			foreach (PS_Objective objective : objectivesOut)
			{
				if (!objective || objective.GetFactionKey() != factionKey)
					continue;
				
				PS_MissionDataObjective missionDataObjective = new PS_MissionDataObjective();
				
				missionDataObjective.Name = WidgetManager.Translate("%1", objective.GetTitle());
				missionDataObjective.Completed = objective.GetCompleted();
				missionDataObjective.Score = objective.GetScore();
				
				missionDataFactionResult.Objectives.Insert(missionDataObjective);
			}
			m_Data.FactionResults.Insert(missionDataFactionResult);
		}
	}
	
	void WriteToFile()
	{
		if (m_Data.SessionName == "")
		{
			string time = System.GetUnixTime().ToString();
			m_Data.SessionName = string.Format("PS_MissionData_%1.json", time);
		}

		// Guarantee $profile:Sessions exists on Linux/Windows
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