//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_MissionDataManagerClass: ScriptComponentClass
{
	
};


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
	ref map<int, bool> m_playerSaved = new map<int, bool>();
	ref set<RplId> m_DeadEntities = new set<RplId>();
	PS_PlayableManager m_PlayableManager;
	PS_ObjectiveManager m_ObjectiveManager;
	PS_GameModeCoop m_GameModeCoop;
	FactionManager m_FactionManager;
	PlayerManager m_PlayerManager;
	ref PS_MissionDataConfig m_Data = new PS_MissionDataConfig();
	int m_iInitTimer = 20;
	
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().CallLater(LateInit, 0, false);
		GetGame().GetCallqueue().CallLater(AwaitFullInit, 0, true);
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
		if (damageManagerComponent)
		{
			m_RplToDamageManager.Insert(vehicleId, damageManagerComponent);
			damageManagerComponent.GetOnDamage().Insert(OnDamaged);
		}
		
		m_Data.Vehicles.Insert(vehicleData);
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
		
		m_GameModeCoop.GetOnHandlePlayerKilled().Insert(OnPlayerKilled);
		m_GameModeCoop.GetOnGameStateChange().Insert(OnGameStateChanged);
		m_GameModeCoop.GetOnPlayerAuditSuccess().Insert(OnPlayerAuditSuccess);
		if (RplSession.Mode() != RplMode.Dedicated) 
			OnPlayerAuditSuccess(GetGame().GetPlayerController().GetPlayerId());
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
		m_iInitTimer--; // Wait 20 frames, I belive everything can init in 20 frames. maybe...
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
			SavePlayers();
		if (state == SCR_EGameModeState.DEBRIEFING)
		{
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

		m_Data.Token = config.Token;

		JsonSaveContext missionSaveContext = new JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);

		RestContext context = GetGame().GetRestApi().GetContext(config.Address);
		if (!context)
		{
			Print("PS_MissionDataManager: StatSender REST context is null", LogLevel.WARNING);
			return;
		}
		string answer = context.POST_now("", missionSaveContext.SaveToString());
		Print(string.Format("PS_MissionDataManager: website answer(%1)", answer));
	}
	
	void OnPlayerAuditSuccess(int playerId)
	{
		if (m_playerSaved.Contains(playerId))
			return;
		
		string GUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
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
		TimeAndWeatherManagerEntity timeAndWeatherManagerEntity = world.GetTimeAndWeatherManager();
		float time = timeAndWeatherManagerEntity.GetTimeOfTheDay();
		WeatherState weatherState = timeAndWeatherManagerEntity.GetCurrentWeatherState();
		m_Data.MissionWeather = weatherState.GetStateName();
		m_Data.MissionDayTime = time;
		m_Data.MissionWeatherIcon = weatherState.GetIconPath();
		
	
		PS_MissionDataStateChangeEvent missionDataStateChangeEvent = new PS_MissionDataStateChangeEvent();
		missionDataStateChangeEvent.State = SCR_EGameModeState.PREGAME;
		missionDataStateChangeEvent.Time = GetGame().GetWorld().GetWorldTime();
		missionDataStateChangeEvent.SystemTime = System.GetUnixTime();
		m_Data.StateEvents.Insert(missionDataStateChangeEvent);
		
		#ifdef PS_REPLAYS
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		string ReplayPath = replayWriter.m_sReplayFileName;
		ReplayPath.Replace("$profile:Replays/", "");
		m_Data.ReplayPath = ReplayPath;
		#endif
		
		PS_MissionDescriptionManager missionDescriptionManager = PS_MissionDescriptionManager.GetInstance();
		array<PS_MissionDescription> descriptions = new array<PS_MissionDescription>();
		missionDescriptionManager.GetDescriptions(descriptions);
		foreach (PS_MissionDescription description : descriptions)
		{
			PS_MissionDataDescription descriptionData = new PS_MissionDataDescription();
			m_Data.Descriptions.Insert(descriptionData);
			
			descriptionData.Title = WidgetManager.Translate("%1", description.m_sTitle);
			descriptionData.DescriptionLayout = description.m_sDescriptionLayout;
			descriptionData.TextData = WidgetManager.Translate("%1", description.m_sTextData);
			descriptionData.VisibleForFactions = description.m_aVisibleForFactions;
			descriptionData.EmptyFactionVisibility = description.m_bEmptyFactionVisibility;
		}
		
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
		
		PS_MissionDataGroup groupData = new PS_MissionDataGroup();
		PS_MissionDataFaction factionData = new PS_MissionDataFaction();
		
		map<Faction, PS_MissionDataFaction> factionsMap = new map<Faction, PS_MissionDataFaction>();
		map<SCR_AIGroup, PS_MissionDataGroup> groupsMap = new map<SCR_AIGroup, PS_MissionDataGroup>();
		foreach (PS_PlayableContainer playable : playables)
		{
			IEntity character = playable.GetPlayableComponent().GetOwner();
			AIControlComponent aiComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			AIAgent agent = aiComponent.GetAIAgent();
			SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!group)
				continue;
			
			SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
			if (!faction)
				continue;
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
			
			SCR_CharacterDamageManagerComponent damageManagerComponent = playable.GetPlayableComponent().GetCharacterDamageManagerComponent();
			
			damageManagerComponent.GetOnDamage().Insert(OnDamaged);
			m_RplToDamageManager.Insert(playable.GetRplId(), damageManagerComponent);
			missionDataPlayable.EntityId = playable.GetRplId();
			missionDataPlayable.GroupOrder = outAgents.Find(agent);
			missionDataPlayable.Name = WidgetManager.Translate("%1", playable.GetName());
			missionDataPlayable.RoleName = WidgetManager.Translate("%1", playable.GetRoleName());
			m_EntityToRpl.Insert(character.GetID(), playable.GetRplId());
			
			groupData.Playables.Insert(missionDataPlayable);
		}
	}
	
	void SavePlayers()
	{
		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		
		foreach (PS_PlayableContainer playable : playables)
		{
			IEntity character = playable.GetPlayableComponent().GetOwner();
			RplId playableId = playable.GetRplId();
			int playerId = m_PlayableManager.GetPlayerByPlayable(playableId);
			
			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.m_iPlayerId = playerId;
			playerToEntity.EntityId = playableId;
			
			m_Data.PlayersToPlayables.Insert(playerToEntity);
		}
	}
	
	void SaveObjectives()
	{
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
		string time = System.GetUnixTime().ToString();
		m_Data.SessionName = string.Format("PS_MissionData_%1.json", time);

		JsonSaveContext missionSaveContext = new JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);
		string fileName = string.Format("$profile:Sessions\\PS_MissionData_%1.json", time);
		missionSaveContext.SaveToFile(fileName);
	}
}