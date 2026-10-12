//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_MissionDataManagerClass: ScriptComponentClass
{
	
};


/**
 * @brief Сборщик и экспорт данных матча PlayableSelector.
 * @subsystem Lobby | Stats
 * @context Server
 * @entity PS_GameModeCoop
 * @depends PlayableSelector
 * @listens PS_GameModeCoop.GetOnHandlePlayerKilled, GetOnGameStateChange, GetOnPlayerAuditSuccess, SCR_DamageManagerComponent.GetOnDamage
 * @details Собирает слоты по серверным данным PlayableManager, сохраняет полный JSON в профиль сервера
 *          и отправляет облегчённый payload статистики на StatSender.
 */
class PS_MissionDataManager : ScriptComponent
{
	// Единственная точка доступа: менеджер живёт компонентом на гейммоде, глобального синглтона нет.
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
	ref map<int, bool> m_playerSaved = new map<int, bool>();
	PS_PlayableManager m_PlayableManager;
	PS_ObjectiveManager m_ObjectiveManager;
	PS_GameModeCoop m_GameModeCoop;
	FactionManager m_FactionManager;
	PlayerManager m_PlayerManager;
	ref PS_MissionDataConfig m_Data = new PS_MissionDataConfig();
	int m_iInitTimer = 50;
	// Slot load watchdog state: how many playables were seen on the previous tick, and for how many
	// ticks in a row that count has not grown anymore.
	int m_iLastSlotCount = -1;
	int m_iStableTicks = 0;
	bool m_bMissionDataExported;
	// Slot load state shared with the rest of the mod. Static on purpose: observers must be able to ask
	// "are the slots loaded yet?" without having to look the component up on the game mode entity.
	protected static bool s_bSlotLoadComplete;
	protected static int s_iSlotCount;
	
	/**
	 * @brief Готовы ли слоты: рост прекратился, данные фракций собраны.
	 * @details Для внешнего кода. Пример: ждать в цикле, пока
	 *          `if (!PS_MissionDataManager.IsSlotLoadComplete()) return;`.
	 */
	static bool IsSlotLoadComplete()
	{
		return s_bSlotLoadComplete;
	}
	
	/**
	 * @brief Число playable-слотов на момент подтверждения загрузки (0, если ещё не подтверждена).
	 */
	static int GetLoadedSlotCount()
	{
		return s_iSlotCount;
	}
	
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
			GetGame().GetCallqueue().Remove(AwaitSlotLoad);
		}

		if (m_GameModeCoop)
		{
			if (m_GameModeCoop.GetOnHandlePlayerKilled())
				m_GameModeCoop.GetOnHandlePlayerKilled().Remove(OnPlayerKilled);
			if (m_GameModeCoop.GetOnGameStateChange())
				m_GameModeCoop.GetOnGameStateChange().Remove(OnGameStateChanged);
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

		super.OnDelete(owner);
	}
	void RegisterVehicle(Vehicle vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return;
		RplComponent rplComponent = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!rplComponent)
			return; // not replicated (e.g. decoration), nothing to track
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(vehicle.FindComponent(FactionAffiliationComponent));
		SCR_DamageManagerComponent damageManagerComponent = SCR_DamageManagerComponent.Cast(vehicle.FindComponent(SCR_DamageManagerComponent));
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
		// Dedupe against an already tracked damage manager: a repeated registration would subscribe
		// OnDamaged twice and record every damage event two times.
		if (damageManagerComponent && !m_RplToDamageManager.Contains(vehicleId) && damageManagerComponent.GetOnDamage())
		{
			m_RplToDamageManager.Insert(vehicleId, damageManagerComponent);
			damageManagerComponent.GetOnDamage().Insert(OnDamaged);
		}
		
		m_Data.Vehicles.Insert(vehicleData);
		if (!m_EntityToRpl.Contains(vehicle.GetID()))
			m_EntityToRpl.Insert(vehicle.GetID(), vehicleId);
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
		m_GameModeCoop = PS_GameModeCoop.Cast(GetOwner());
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_ObjectiveManager = PS_ObjectiveManager.GetInstance();
		m_FactionManager = GetGame().GetFactionManager();
		
		GetGame().GetCallqueue().CallLater(DelayedInit, 1000, false);
		
		if (!m_GameModeCoop)
		{
			Print("PS_MissionDataManager: PS_GameModeCoop not found on the owner entity - stats export disabled", LogLevel.WARNING);
			return;
		}
		
		m_GameModeCoop.GetOnHandlePlayerKilled().Insert(OnPlayerKilled);
		m_GameModeCoop.GetOnGameStateChange().Insert(OnGameStateChanged);
		m_GameModeCoop.GetOnPlayerAuditSuccess().Insert(OnPlayerAuditSuccess);
		if (RplSession.Mode() != RplMode.Dedicated)
		{
			// A listen server can run without a local player, in which case there is nobody to register.
			PlayerController playerController = GetGame().GetPlayerController();
			if (playerController)
				OnPlayerAuditSuccess(playerController.GetPlayerId());
		}
	}
	
	void DelayedInit()
	{
		if (!Replication.IsServer())
			return;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		
		array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			PS_PlayableComponent playableComp = playable.GetPlayableComponent();
			if (!playableComp)
				continue;
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
		m_iInitTimer--; // Wait 50 frames, I believe everything can init in 50 frames. maybe...
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
			// A new round in the same session may export again.
			m_bMissionDataExported = false;
			SavePlayers();
		}
		if (state == SCR_EGameModeState.DEBRIEFING && !m_bMissionDataExported)
		{
			m_bMissionDataExported = true;
			GetGame().GetCallqueue().Call(DefineScenarioType);
			GetGame().GetCallqueue().Call(SaveObjectives);
			GetGame().GetCallqueue().Call(WriteToFile);
			GetGame().GetCallqueue().Call(SendToWebsite);
		}
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

	// POST the collected mission data to the StatSender endpoint. No-op (graceful) when the server has no
	// $profile:StatSender_Config.json, so a standalone PlayableSelector simply writes the JSON file and
	// skips the upload. Moved here from QuickTvT so the whole stats pipeline lives in PlayableSelector.
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
		if (m_Data.Token == "")
		{
			Print("PS_MissionDataManager: StatSender Token is empty - skipping website upload", LogLevel.WARNING);
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

		RestApi restApi = GetGame().GetRestApi();
		if (!restApi)
		{
			Print("PS_MissionDataManager: REST API is unavailable - skipping website upload", LogLevel.WARNING);
			return;
		}
		RestContext context = restApi.GetContext(config.Address);
		if (!context)
		{
			Print("PS_MissionDataManager: StatSender REST context is null", LogLevel.WARNING);
			return;
		}
		// POST the trimmed payload built above - re-serializing m_Data here would restore the large
		// DamageEvents/Vehicles arrays and defeat both the 1 MB guard and the comment's intent.
		string answer = context.POST_now("", payload);
		Print(string.Format("PS_MissionDataManager: website upload initiated size(%2 bytes) answer(%1)", answer, payloadSize), LogLevel.NORMAL);
	}
	
	void OnPlayerAuditSuccess(int playerId)
	{
		if (m_playerSaved.Contains(playerId))
			return;
		if (!m_PlayerManager)
			return;
		
		// No BackendApi on a server without BattlEye/auth - keep the row, but leave the GUID empty.
		string GUID = "";
		BackendApi backendApi = GetGame().GetBackendApi();
		if (backendApi)
			GUID = backendApi.GetPlayerIdentityId(playerId);
		string name = m_PlayerManager.GetPlayerName(playerId);
		
		PS_MissionDataPlayer player = new PS_MissionDataPlayer();
		player.m_iPlayerId = playerId;
		player.GUID = GUID;
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
		if (missionHeader) {
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
				m_Data.MissionDayTime = timeAndWeatherManagerEntity.GetTimeOfTheDay();
				WeatherState weatherState = timeAndWeatherManagerEntity.GetCurrentWeatherState();
				if (weatherState)
				{
					m_Data.MissionWeather = weatherState.GetStateName();
					m_Data.MissionWeatherIcon = weatherState.GetIconPath();
				}
			}
		}
		
		// Seed the timeline only if no state transition has been recorded yet: OnGameStateChanged starts
		// firing before this frame budget runs out, so an unconditional insert would land at the end.
		if (world && m_Data.StateEvents.IsEmpty())
		{
			PS_MissionDataStateChangeEvent missionDataStateChangeEvent = new PS_MissionDataStateChangeEvent();
			missionDataStateChangeEvent.State = SCR_EGameModeState.PREGAME;
			missionDataStateChangeEvent.Time = world.GetWorldTime();
			missionDataStateChangeEvent.SystemTime = System.GetUnixTime();
			m_Data.StateEvents.Insert(missionDataStateChangeEvent);
		}
		
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
				m_Data.Descriptions.Insert(descriptionData);
				
				descriptionData.Title = WidgetManager.Translate("%1", description.m_sTitle);
				descriptionData.DescriptionLayout = description.m_sDescriptionLayout;
				descriptionData.TextData = WidgetManager.Translate("%1", description.m_sTextData);
				descriptionData.VisibleForFactions = description.m_aVisibleForFactions;
				descriptionData.EmptyFactionVisibility = description.m_bEmptyFactionVisibility;
			}
		}

		// Slot load watchdog: poll the playable count once a second; once it has not grown for 3 s the
		// slot load is considered complete and the faction data is collected exactly once.
		s_bSlotLoadComplete = false;
		s_iSlotCount = 0;
		GetGame().GetCallqueue().CallLater(AwaitSlotLoad, 1000, true);
	}
	
	/**
	 * @brief Одноразовый сторож загрузки слотов.
	 * @subsystem Lobby | Stats
	 * @context Server
	 * @integration PS_PlayableManager: источник числа слотов; вызывает CollectFactions ровно один раз.
	 * @details Раз в секунду сверяет число playable-контейнеров. Как только рост прекратился на 3 секунды
	 *          подряд - снимает себя с callqueue и собирает данные фракций. Стартует в конце InitData;
	 *          m_iLastSlotCount = -1 гарантирует, что первый тик считается ростом.
	 * @note По завершении выставляет s_bSlotLoadComplete / s_iSlotCount - читать через
	 *       IsSlotLoadComplete() и GetLoadedSlotCount().
	 */
	void AwaitSlotLoad()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		
		int slotCount = playableManager.GetPlayablesSorted().Count();
		if (slotCount > m_iLastSlotCount)
			m_iStableTicks = 0;
		else
			m_iStableTicks++;
		m_iLastSlotCount = slotCount;
		
		if (m_iStableTicks < 3)
			return;
		
		GetGame().GetCallqueue().Remove(AwaitSlotLoad);
		CollectFactions();
		// Flag after CollectFactions on purpose: "loaded" must mean the faction data is actually in m_Data,
		// not just that the watchdog fired.
		s_iSlotCount = slotCount;
		s_bSlotLoadComplete = true;
		Print(string.Format("PS_MissionDataManager: Slot load complete, %1 slots stable for 3 s", slotCount), LogLevel.NORMAL);
	}
	
	void CollectFactions()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		
		array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
		
		PS_MissionDataGroup groupData = new PS_MissionDataGroup();
		PS_MissionDataFaction factionData = new PS_MissionDataFaction();
		
		map<Faction, PS_MissionDataFaction> factionsMap = new map<Faction, PS_MissionDataFaction>();
		map<SCR_AIGroup, PS_MissionDataGroup> groupsMap = new map<SCR_AIGroup, PS_MissionDataGroup>();
		int addedPlayables = 0;
		int skippedWithoutFaction = 0;
		
		foreach (PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;
			PS_PlayableComponent playableComp = playable.GetPlayableComponent();
			if (!playableComp)
				continue;
			
			IEntity character = playableComp.GetOwner();
			if (!character)
				continue;
			
			AIControlComponent aiComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			if (!aiComponent)
				continue;
			AIAgent agent = aiComponent.GetAIAgent();
			if (!agent)
				continue;
			SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!group)
				continue;
			
			SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
			if (!faction)
			{
				skippedWithoutFaction++;
				continue;
			}
			if (!factionsMap.Contains(faction))
			{
				factionData = new PS_MissionDataFaction();
				m_Data.Factions.Insert(factionData);
				
				factionData.Name = WidgetManager.Translate("%1", faction.GetFactionName());
				factionData.Key = WidgetManager.Translate("%1", faction.GetFactionKey());
				
				Color color = faction.GetFactionColor();
				factionData.FactionColor = string.Format("%1,%2,%3,%4", color.A(), color.R(), color.G(), color.B());
				color = faction.GetOutlineFactionColor();
				factionData.FactionOutlineColor = string.Format("%1,%2,%3,%4", color.A(), color.R(), color.G(), color.B());
				
				factionsMap.Insert(faction, factionData);
			}
			factionData = factionsMap.Get(faction);
			if (!groupsMap.Contains(group))
			{
				groupData = new PS_MissionDataGroup();
				factionData.Groups.Insert(groupData);
				
				string customName = group.GetCustomName();
				string company, platoon, squad, t, format;
				group.GetCallsigns(company, platoon, squad, t, format);
				string callsign;
				callsign = WidgetManager.Translate(format, company, platoon, squad, "");
				
				groupData.Callsign = playableManager.GetGroupCallsignByPlayable(playable.GetRplId());
				groupData.CallsignName = callsign;
				groupData.Name = WidgetManager.Translate("%1", customName);
				
				groupsMap.Insert(group, groupData);
			}
			groupData = groupsMap.Get(group);
			
			array<AIAgent> outAgents = new array<AIAgent>();
			group.GetAgents(outAgents);
			
			PS_MissionDataPlayable missionDataPlayable = new PS_MissionDataPlayable();
			
			SCR_CharacterDamageManagerComponent damageManagerComponent = playableComp.GetCharacterDamageManagerComponent();
			if (damageManagerComponent && !m_RplToDamageManager.Contains(playable.GetRplId()))
			{
				damageManagerComponent.GetOnDamage().Insert(OnDamaged);
				m_RplToDamageManager.Insert(playable.GetRplId(), damageManagerComponent);
			}
			missionDataPlayable.EntityId = playable.GetRplId();
			missionDataPlayable.GroupOrder = outAgents.Find(agent);
			missionDataPlayable.Name = WidgetManager.Translate("%1", playable.GetName());
			missionDataPlayable.RoleName = WidgetManager.Translate("%1", playable.GetRoleName());
			m_EntityToRpl.Insert(character.GetID(), playable.GetRplId());
			
			groupData.Playables.Insert(missionDataPlayable);
			addedPlayables++;
		}
		
		Print(string.Format("PS_MissionDataManager: Collected %1 factions and %2/%3 playables; %4 skipped without a resolvable faction", m_Data.Factions.Count(), addedPlayables, playables.Count(), skippedWithoutFaction), LogLevel.NORMAL);
	}
	
	void SavePlayers()
	{
		if (!m_PlayableManager)
		{
			Print("PS_MissionDataManager: PS_PlayableManager is null - player-to-slot associations not saved", LogLevel.WARNING);
			return;
		}
		
		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		int savedCount = 0;
		int skippedFree = 0;
		
		foreach (PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;
			RplId playableId = playable.GetRplId();
			int playerId = m_PlayableManager.GetPlayerByPlayable(playableId);
			// Free and reserved slots report -1/-2; writing those in produces fake players on the site.
			if (playerId <= 0)
			{
				skippedFree++;
				continue;
			}
			
			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.m_iPlayerId = playerId;
			playerToEntity.EntityId = playableId;
			
			m_Data.PlayersToPlayables.Insert(playerToEntity);
			savedCount++;
		}
		
		Print(string.Format("PS_MissionDataManager: Saved %1 player-to-slot associations; %2 slots free/reserved, %3 slots total", savedCount, skippedFree, playables.Count()), LogLevel.NORMAL);
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
				if (objective.GetFactionKey() != factionKey)
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
		if (!m_Data)
			return;
		
		if (m_Data.MissionName == "")
			DefineScenarioType();
		
		string time = System.GetUnixTime().ToString();
		m_Data.SessionName = string.Format("PS_MissionData_%1.json", time);
		// SaveToFile fails silently on a fresh profile where the folder was never created.
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