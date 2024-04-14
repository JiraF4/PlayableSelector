//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_MissionDataManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
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
		RplComponent rplComponent = RplComponent.Cast(vehicle.FindComponent(RplComponent));
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
				vehicleData.EditableName = info.GetName();
		}
		if (factionAffiliationComponent)
		{
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			if (faction)
			{
				vehicleData.VehicleFactionKey = faction.GetFactionKey();
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
			
			GetGame().GetCallqueue().Call(SaveDamageEvent, playerId, rplId, damageContext.damageValue);
		}
	}
	
	void SaveDamageEvent(int playerId, RplId targetId, float value)
	{
		SCR_DamageManagerComponent damageManagerComponent = m_RplToDamageManager.Get(targetId);
		EDamageState state = damageManagerComponent.GetState();
		
		PS_MissionDataDamageEvent missionDataDamageEvent = new PS_MissionDataDamageEvent();
		missionDataDamageEvent.PlayerId = playerId;
		missionDataDamageEvent.TargetId = targetId;
		missionDataDamageEvent.DamageValue = value;
		missionDataDamageEvent.TargetState = state;
		missionDataDamageEvent.Time = GetGame().GetWorld().GetWorldTime();
		m_Data.DamageEvents.Insert(missionDataDamageEvent);
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
		missionDataPlayerKill.PlayerId = playerId;
		missionDataPlayerKill.Time = GetGame().GetWorld().GetWorldTime();
		missionDataPlayerKill.SystemTime = System.GetUnixTime();
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
			GetGame().GetCallqueue().Call(SaveObjectives);
			GetGame().GetCallqueue().Call(WriteToFile);
		}
	}
	
	void OnPlayerAuditSuccess(int playerId)
	{
		if (m_playerSaved.Contains(playerId))
			return;
		
		string GUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		string name = m_PlayerManager.GetPlayerName(playerId);
		
		PS_MissionDataPlayer player = new PS_MissionDataPlayer();
		player.PlayerId = playerId;
		player.GUID = GUID;
		player.Name = name;
		m_Data.Players.Insert(player);
		
		m_playerSaved.Insert(playerId, true);
	}

	// Save main mission data
	void InitData()
	{
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (missionHeader) {
			m_Data.MissionPath = missionHeader.GetHeaderResourcePath();
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
		m_Data.ReplayPath = replayWriter.m_sReplayFileName;
		#endif
		
		PS_MissionDescriptionManager missionDescriptionManager = PS_MissionDescriptionManager.GetInstance();
		array<PS_MissionDescription> descriptions = new array<PS_MissionDescription>();
		missionDescriptionManager.GetDescriptions(descriptions);
		foreach (PS_MissionDescription description : descriptions)
		{
			PS_MissionDataDescription descriptionData = new PS_MissionDataDescription();
			m_Data.Descriptions.Insert(descriptionData);
			
			descriptionData.Title = description.m_sTitle;
			descriptionData.DescriptionLayout = description.m_sDescriptionLayout;
			descriptionData.TextData = description.m_sTextData;
			descriptionData.VisibleForFactions = description.m_aVisibleForFactions;
			descriptionData.EmptyFactionVisibility = description.m_bEmptyFactionVisibility;
		}
		
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		array<PS_PlayableComponent> playables = playableManager.GetPlayablesSorted();
		
		PS_MissionDataGroup groupData = new PS_MissionDataGroup();
		PS_MissionDataFaction factionData = new PS_MissionDataFaction();
		
		map<Faction, PS_MissionDataFaction> factionsMap = new map<Faction, PS_MissionDataFaction>();
		map<SCR_AIGroup, PS_MissionDataGroup> groupsMap = new map<SCR_AIGroup, PS_MissionDataGroup>();
		foreach (PS_PlayableComponent playable : playables)
		{
			IEntity character = playable.GetOwner();
			AIControlComponent aiComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			AIAgent agent = aiComponent.GetAIAgent();
			SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!group)
				continue;
			
			SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
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
				
				groupData.Callsign = playableManager.GetGroupCallsignByPlayable(playable.GetId());
				groupData.CallsignName = callsign;
				groupData.Name = customName;
				
				groupsMap.Insert(group, groupData);
			}
			groupData = groupsMap.Get(group);
			
			array<AIAgent> outAgents = new array<AIAgent>();
			group.GetAgents(outAgents);
			SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
			SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
			
			PS_MissionDataPlayable missionDataPlayable = new PS_MissionDataPlayable();
			
			SCR_CharacterDamageManagerComponent damageManagerComponent = playable.GetCharacterDamageManagerComponent();
			
			damageManagerComponent.GetOnDamage().Insert(OnDamaged);
			m_RplToDamageManager.Insert(playable.GetId(), damageManagerComponent);
			missionDataPlayable.EntityId = playable.GetId();
			missionDataPlayable.GroupOrder = outAgents.Find(agent);
			missionDataPlayable.Name = WidgetManager.Translate("%1", playable.GetName());
			missionDataPlayable.RoleName = WidgetManager.Translate("%1", uiInfo.GetName());
			m_EntityToRpl.Insert(character.GetID(), playable.GetId());
			
			groupData.Playables.Insert(missionDataPlayable);
		}
	}
	
	void SavePlayers()
	{
		array<PS_PlayableComponent> playables = m_PlayableManager.GetPlayablesSorted();
		
		foreach (PS_PlayableComponent playable : playables)
		{
			IEntity character = playable.GetOwner();
			RplId playableId = playable.GetId();
			int playerId = m_PlayableManager.GetPlayerByPlayable(playableId);
			
			PS_MissionDataPlayerToEntity playerToEntity = new PS_MissionDataPlayerToEntity();
			playerToEntity.PlayerId = playerId;
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
				missionDataFactionResult.ResultName = objectiveLevel.GetName();
				missionDataFactionResult.ResultScore = objectiveLevel.GetScore();
			}
			foreach (PS_Objective objective : objectivesOut)
			{
				if (objective.GetFactionKey() != factionKey)
					continue;
				
				PS_MissionDataObjective missionDataObjective = new PS_MissionDataObjective();
				
				missionDataObjective.Name = objective.GetTitle();
				missionDataObjective.Completed = objective.GetCompleted();
				missionDataObjective.Score = objective.GetScore();
				
				missionDataFactionResult.Objectives.Insert(missionDataObjective);
			}
			m_Data.FactionResults.Insert(missionDataFactionResult);
		}
	}
	
	void WriteToFile()
	{
		SCR_JsonSaveContext missionSaveContext = new SCR_JsonSaveContext();
		missionSaveContext.WriteValue("", m_Data);
		string fileName = string.Format("$profile:Sessions\\PS_MissionData_%1.json", System.GetUnixTime().ToString());
		missionSaveContext.SaveToFile(fileName);
	}
}