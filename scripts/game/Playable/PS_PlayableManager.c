void PS_ScriptInvokerFactionChangeMethod(int playerId, FactionKey factionKey, FactionKey factionKeyOld);
typedef func PS_ScriptInvokerFactionChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionChangeMethod> PS_ScriptInvokerFactionChange;

void PS_ScriptInvokerPlayableMethod(RplId id, PS_PlayableContainer playableComponent);
typedef func PS_ScriptInvokerPlayableMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableMethod> PS_ScriptInvokerPlayable;

void PS_ScriptInvokerPinChangeMethod(int playerId, bool pin);
typedef func PS_ScriptInvokerPinChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPinChangeMethod> PS_ScriptInvokerPinChange;

void PS_ScriptInvokerPlayerStateChangeMethod(int playerId, PS_EPlayableControllerState state);
typedef func PS_ScriptInvokerPlayerStateChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerStateChangeMethod> PS_ScriptInvokerPlayerStateChange;

void PS_ScriptInvokerPlayerPlayableChangeMethod(int playerId, RplId playbleId);
typedef func PS_ScriptInvokerPlayerPlayableChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerPlayableChangeMethod> PS_ScriptInvokerPlayerPlayableChange;

void PS_ScriptInvokerPlayableChangeGroupMethod(RplId id, PS_PlayableContainer playableComponent, SCR_AIGroup aiGroup);
typedef func PS_ScriptInvokerPlayableChangeGroupMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableChangeGroupMethod> PS_ScriptInvokerPlayableChangeGroup;

void PS_ScriptInvokerFactionReadyChangeMethod(FactionKey factionKey, int readyValue);
typedef func PS_ScriptInvokerFactionReadyChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionReadyChangeMethod> PS_ScriptInvokerFactionReadyChangeGroup;

[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManagerClass : ScriptComponentClass
{

}

class PS_PlayableManager : ScriptComponent
{
	// Map of our playables
	ref map<RplId, ref PS_PlayableContainer> m_aPlayables = new map<RplId, ref PS_PlayableContainer>(); // We NOW sync it!
	ref array<PS_PlayableContainer> m_aPlayablesSorted = {};
	ref map<RplId, ref PS_PlayableVehicleContainer> m_mPlayableVehicles = new map<RplId, ref PS_PlayableVehicleContainer>();

	// Maps for saving players staff, player controllers local to client
	ref map<int, PS_EPlayableControllerState> m_playersStates = new map<int, PS_EPlayableControllerState>();
	ref map<int, RplId> m_playersPlayableRemembered = new map<int, RplId>();
	ref map<int, RplId> m_playersPlayable = new map<int, RplId>();
	ref map<RplId, int> m_playablePlayersRemembered = new map<RplId, int>();
	ref map<RplId, int> m_playablePlayers = new map<RplId, int>(); // reversed m_playersPlayable for fast search
	ref map<int, bool> m_playersPin = new map<int, bool>(); // is player pined
	ref map<int, FactionKey> m_playersFaction = new map<int, FactionKey>(); // player factions
	ref map<int, FactionKey> m_playersFactionRemembered = new map<int, FactionKey>(); // player factions persistant
	ref map<RplId, int> m_playablePlayerGroupId = new map<RplId, int>(); // playable to player group
	ref map<int, string> m_playersLastName = new map<int, string>(); // playerid to player name (persistant)
	ref map<FactionKey, int> m_mFactionReady = new map<FactionKey, int>(); // faction ready state
	ref map<RplId, string> m_mPlayablePrefabs = new map<RplId, string>();
	ref map<RplId, string> m_mPlayableNames = new map<RplId, string>();

	// Invokers
	ref ScriptInvokerInt m_eOnPlayerConnected = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerConnected()
	{
		return m_eOnPlayerConnected;
	}
	ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
	{
		return m_eOnPlayerDisconnected;
	}
	ref PS_ScriptInvokerFactionChange m_eOnFactionChange = new PS_ScriptInvokerFactionChange(); // int playerId, FactionKey factionKey, FactionKey factionKeyOld
	PS_ScriptInvokerFactionChange GetOnFactionChange()
	{
		return m_eOnFactionChange;
	}
	ref PS_ScriptInvokerPlayable m_eOnPlayableRegistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableRegistered()
	{
		return m_eOnPlayableRegistered;
	}
	ref PS_ScriptInvokerPlayable m_eOnPlayableUnregistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableUnregistered()
	{
		return m_eOnPlayableUnregistered;
	}
	ref PS_ScriptInvokerPinChange m_eOnPlayerPinChange = new PS_ScriptInvokerPinChange();
	PS_ScriptInvokerPinChange GetOnPlayerPinChange()
	{
		return m_eOnPlayerPinChange;
	}
	ref PS_ScriptInvokerPlayerStateChange m_eOnPlayerStateChange = new PS_ScriptInvokerPlayerStateChange();
	PS_ScriptInvokerPlayerStateChange GetOnPlayerStateChange()
	{
		return m_eOnPlayerStateChange;
	}
	ref PS_ScriptInvokerPlayerPlayableChange m_eOnPlayerPlayableChange = new PS_ScriptInvokerPlayerPlayableChange();
	PS_ScriptInvokerPlayerPlayableChange GetOnPlayerPlayableChange()
	{
		return m_eOnPlayerPlayableChange;
	}
	ref PS_ScriptInvokerPlayableChangeGroup m_eOnPlayableChangeGroup = new PS_ScriptInvokerPlayableChangeGroup();
	PS_ScriptInvokerPlayableChangeGroup GetOnPlayableChangeGroup()
	{
		return m_eOnPlayableChangeGroup;
	}
	ref ScriptInvokerInt m_eOnStartTimerCounterChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnStartTimerCounterChanged()
	{
		return m_eOnStartTimerCounterChanged;
	}
	ref PS_ScriptInvokerFactionReadyChangeGroup m_eFactionReadyChanged = new PS_ScriptInvokerFactionReadyChangeGroup();
	PS_ScriptInvokerFactionReadyChangeGroup GetOnFactionReadyChanged()
	{
		return m_eFactionReadyChanged;
	}

	//Global cache
	protected PS_GameModeCoop m_GameModeCoop;
	protected ScriptCallQueue m_CallQueue;
	protected PlayerManager m_PlayerManager;

	protected SCR_PlayerController m_CurrentPlayerController;
	static protected PS_PlayableControllerComponent s_CurrentPlayableController;

	protected static PS_PlayableManager s_Instance;

	[RplProp()]
	int m_iMaxPlayersCount = 1; // Max players count from server config
	[RplProp(onRplName: "OnStartTimerCounterChanged")]
	
	// TODO: Remove?
	int m_iStartTimerCounter = -1;
	void StartTime()
	{
		m_iStartTimerCounter -= 1;
		Replication.BumpMe();
		OnStartTimerCounterChanged();
		if (m_iStartTimerCounter == 0)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gameModeCoop.AdvanceGameState(SCR_EGameModeState.SLOTSELECTION);
			m_CallQueue.Remove(StartTime);
		}
	}
	void OnStartTimerCounterChanged()
	{
		m_eOnStartTimerCounterChanged.Invoke(m_iStartTimerCounter);
	}
	
	bool m_bFactionsReadySended; // Is faction ready message already sended?

	// Is it required?
	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}

	// --------------------------------------------------------------------------------------------
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_PlayableManager GetInstance()
	{
		return s_Instance;
	}

	// --------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		s_Instance = this;

		//Cache
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_CallQueue = GetGame().GetCallqueue();
		m_PlayerManager = GetGame().GetPlayerManager();

		if (Replication.IsServer())
			m_bRplLoaded = true;
		if (RplSession.Mode() == RplMode.Dedicated)
			ForceGetSessionMaxPlayersCount();
	
		// Register events
		m_GameModeCoop.GetOnPlayerConnected().Insert(OnPlayerConnected);
		m_GameModeCoop.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		m_GameModeCoop.GetOnPlayerRoleChange().Insert(OnPlayerRoleChange);
		m_CallQueue.Call(LateInit, owner);
	}
	protected void LateInit(IEntity owner)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		m_CurrentPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_CurrentPlayerController)
		{
			m_CallQueue.Call(LateInit, owner);
			return;
		}
		s_CurrentPlayableController = m_CurrentPlayerController.PS_GetPLayableComponent();
	}
	// --------------------------------------------------------------------------------------------
	// Read max players count from server config
	protected void ForceGetSessionMaxPlayersCount()
	{
		DSSession dSSession = GetGame().GetBackendApi().GetDSSession();
		if (dSSession)
		{
			m_iMaxPlayersCount = dSSession.PlayerLimit();
			Replication.BumpMe();
		}
		else
			m_CallQueue.Call(ForceGetSessionMaxPlayersCount); // Loading take some time, awaiting valid config
	}

	// --------------------------------------------------------------------------------------------
	// ----------------------------------- Main entry point ---------------------------------------
	// --------------------------------------------------------------------------------------------
	// Get control on selected playable entity, or initial and become spectator if no playable provided
	// Executed only on server
	void ApplyPlayable(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();

		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);

		// If entity dead, switch to spectator after some delay
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = GetPlayableById(playableId);
			if (playableContainer.GetDamageState() == EDamageState.DESTROYED)
			{
				m_CallQueue.CallLater(DelayedSwitchToInitialEntity, 1000, false, playerId);
			}
		}

		IEntity entity;
		if (playableId == RplId.Invalid()) { // switch to null entity
			// Remove group and faction
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);
			SetPlayerFactionKey(playerId, "");

			// Create new entity if need
			entity = playableController.GetInitialEntity();
			if (!entity)
			{
				Resource resource = Resource.Load("{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et");
				EntitySpawnParams params = new EntitySpawnParams();
				entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
				playableController.SetInitialEntity(entity);
			}

			// Apply entity
			playerController.SetInitialMainEntity(entity);
			return;
		} else
			entity = GetPlayableById(playableId).GetPlayableComponent().GetOwner();

		// Delete initial entity if exists
		IEntity initialEntity = playableController.GetInitialEntity();
		if (initialEntity)
			m_CallQueue.Call(SCR_EntityHelper.DeleteEntityAndChildren, initialEntity);

		// Apply entity
		playerController.SetInitialMainEntity(entity);

		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		SetPlayerFactionKey(playerId, faction.GetFactionKey());

		// Requred delay, since entity take one frame to apply controls
		m_CallQueue.CallLater(ChangeGroup, 0, false, playerId, playableId);
	}
	// --------------------------------------------------------------------------------------------
	protected void DelayedSwitchToInitialEntity(int playerId)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.SwitchToInitialEntity(playerId);
	}

	// --------------------------------------------------------------------------------------------
	// Change player group via playable
	void ChangeGroup(int playerId, RplId playableId)
	{
		// Get updated player
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();

		// Get playable container
		SCR_AIGroup playerGroup = GetPlayerGroupByPlayable(playableId);
		SCR_ChimeraCharacter leaderCharacter = null;
		if (playerGroup)
			leaderCharacter = SCR_ChimeraCharacter.Cast(playerGroup.GetLeaderEntity());
		PS_PlayableContainer playableContainerLeader;
		if (leaderCharacter)
			playableContainerLeader = leaderCharacter.PS_GetPlayable().GetPlayableContainer();

		// Join group
		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		if (playerGroup)
			playerControllerGroupComponent.PS_AskJoinGroup(playerGroup.GetGroupID());

		// Another workaround
		// Thanks bohem, no one zoomer will be harmed if you remove all text from game.
		// Maybe also multiplayer from arma? It can harm people, very dangerous.
		if (playerGroup && playerGroup.GetNameAuthorID() == -1)
			playerGroup.SetCustomName(playerGroup.GetCustomName(), playerId);


		// Switch leader if need
		if (playableContainerLeader)
			if (playableContainerLeader.GetRplId() > playableId)
				groupsManagerComponent.SetGroupLeader(playerGroup.GetGroupID(), playerId);
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------- Registration -----------------------------------------
	// --------------------------------------------------------------------------------------------
	// Register playable container to global list, replicated across clients
	// Save playable name and prefab for later use
	// - SERVER SIDE: Create new group for players if requred
	void RegisterPlayable(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetRplId();
		if (m_aPlayables.Contains(playableId)) // Already registered
			return;
		SCR_ChimeraCharacter playableCharacter = playableComponent.GetCharacter();
		if (!playableCharacter.PS_GetChimeraAIControlComponent())
			return;

		// Save and replicate data
		PS_PlayableContainer container = playableComponent.GetPlayableContainer();
		RPC_RegisterPlayable(container);
		Rpc(RPC_RegisterPlayable, container);
		SetPlayablePrefab(playableId, playableComponent.GetOwner().GetPrefabData().GetPrefabName());
		SetPlayableName(playableId, playableComponent.GetName());

		// Server side
		if (Replication.IsServer())
		{
			AIControlComponent aiControl = playableCharacter.PS_GetChimeraAIControlComponent();
			SCR_AIGroup playableGroup = SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
			SCR_AIGroup playerGroup;

			if (!playableGroup) // Has no group -> broken unit.
				return;

			// If no player, group create new
			if (!playableGroup.m_PlayersGroup)
			{
				SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
				playerGroup = groupsManagerComponent.CreateNewPlayableGroup(playableGroup.GetFaction());

				// Setup link, command system override slave group
				// TODO: somehow move from PSCore
				playerGroup.m_BotsGroup = playableGroup;
				playableGroup.m_PlayersGroup = playerGroup;

				playerGroup.SetMaxMembers(playableGroup.m_aUnitPrefabSlots.Count());
				playerGroup.SetCustomName(playableGroup.GetCustomName(), -1);
				playableGroup.SetCanDeleteIfNoPlayer(false);
				playerGroup.SetCanDeleteIfNoPlayer(false);
				playableGroup.SetDeleteWhenEmpty(false);
				playerGroup.SetDeleteWhenEmpty(false);
			} else {
				playerGroup = playableGroup.m_PlayersGroup;
			}
			SetPlayablePlayerGroupId(playableId, playerGroup.GetGroupID()); // Save link to map for fast search
			m_CallQueue.Call(UpdateGroupCallsign, playableId, playerGroup, playableGroup) // Delay for group init
		}
	}
	protected void UpdateGroupCallsign(RplId playableId, SCR_AIGroup playerGroup, SCR_AIGroup playableGroup)
	{
		// Assign manualy set callsigns
		PS_GroupCallsignAssigner groupCallsignAssigner = PS_GroupCallsignAssigner.Cast(playableGroup.FindComponent(PS_GroupCallsignAssigner));
		int company, platoon, squad;
		if (groupCallsignAssigner) {
			groupCallsignAssigner.GetCallsign(company, platoon, squad);
		} else {
			SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playableGroup.FindComponent(SCR_CallsignGroupComponent));
			callsignComponent.GetCallsignIndexes(company, platoon, squad);
		}
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		callsignComponent.ReAssignGroupCallsign(company, platoon, squad);

		m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup) // Delay for callsign init
	}
	protected void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		// Get group callsign
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		int company, platoon, squad;
		callsignComponent.GetCallsignIndexes(company, platoon, squad);
		int groupCallsign = 1000000 * company + 1000 * platoon + 1 * squad;

		// Create VoN group channels
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), groupCallsign.ToString());
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Faction");
	}
	// Execute on both client and server
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_RegisterPlayable(PS_PlayableContainer container)
	{
		m_aPlayables[container.GetRplId()] = container;
		m_CallQueue.Remove(UpdatePlayablesSortedDelayed);
		m_CallQueue.Remove(UpdatePlayablesSorted);
		m_CallQueue.Call(UpdatePlayablesSortedDelayed); // List updated resort
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke, container.GetRplId(), container); // 2 frames delayed event invoke, give group time to fully initialize
	}
	protected void OnPlayableRegisteredLateInvoke(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke2, playableId, playableComponent);
	}
	protected void OnPlayableRegisteredLateInvoke2(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_eOnPlayableRegistered.Invoke(playableId, playableComponent);
	}

	// --------------------------------------------------------------------------------------------
	// Remove plyable from list global list replicated
	void UnRegisterPlayable(RplId playableId)
	{
		RPC_UnRegisterPlayable(playableId);
		Rpc(RPC_UnRegisterPlayable, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_UnRegisterPlayable(RplId playableId)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		PS_PlayableContainer playableContainer = m_aPlayables[playableId];
		m_aPlayables.Remove(playableId);

		UpdatePlayablesSorted(); // List updated resort
		m_eOnPlayableUnregistered.Invoke(playableId, playableContainer);
		playableContainer.m_eOnUnregister.Invoke();
	}

	// --------------------------------------------------------------------------------------------
	// Register vehicle to global list replicated
	// TODO: attach any entity
	void RegisterGroupVehicle(RplId rplId, SCR_AIGroup group, IEntity vehicle)
	{
		if (!Replication.IsServer())
			return;
		if (!group.m_PlayersGroup)
		{
			m_CallQueue.Call(RegisterGroupVehicle, rplId, group, vehicle);
			return;
		}
		int groupCallsign = group.GetCallsignNum();
		PS_PlayableVehicleContainer playableVehicleContainer = new PS_PlayableVehicleContainer();
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		SCR_VehicleFactionAffiliationComponent vehicleFactionAffiliationComponent = SCR_VehicleFactionAffiliationComponent.Cast(vehicle.FindComponent(SCR_VehicleFactionAffiliationComponent));
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		ResourceName prefab = vehicle.GetPrefabData().GetPrefabName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetResourceName();
		playableVehicleContainer.Init(rplId, prefab, uIInfo.GetIconPath(), groupCallsign, group.m_PlayersGroup.GetGroupID(), vehicleFactionAffiliationComponent.GetDefaultFactionKey());
		Rpc(RPC_RegisterGroupVehicle, playableVehicleContainer);
		RPC_RegisterGroupVehicle(playableVehicleContainer);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_RegisterGroupVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		m_mPlayableVehicles[playableVehicleContainer.m_iRplId] = playableVehicleContainer;
	}

	// --------------------------------------------------------------------------------------------
	// Remove vehicle from  global list list replicated
	void UnRegisterGroupVehicle(RplId rplId)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RPC_UnRegisterGroupVehicle, rplId);
		RPC_UnRegisterGroupVehicle(rplId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_UnRegisterGroupVehicle(RplId rplId)
	{
		m_mPlayableVehicles.Remove(rplId);
	}

	// --------------------------------------------------------------------------------------------
	// --------------------------------------- Accessors ------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Get playable from global list, or null if no
	// - Synced on clients
	PS_PlayableContainer GetPlayableById(RplId PlayableId)
	{
		PS_PlayableContainer playableComponent;
		m_aPlayables.Find(PlayableId, playableComponent);
		return playableComponent;
	}

	// --------------------------------------------------------------------------------------------
	// Get global map of playables
	// - Synced on clients
	map<RplId, ref PS_PlayableContainer> GetPlayables()
	{
		return m_aPlayables;
	}

	// --------------------------------------------------------------------------------------------
	// Get cached list of playables sorted by CallSign -> Rank -> RplId
	// - Synced on clients
	array<PS_PlayableContainer> GetPlayablesSorted()
	{
		return m_aPlayablesSorted;
	}

	// --------------------------------------------------------------------------------------------
	// Get global map of vehicles
	// - Synced on clients
	map<RplId, ref PS_PlayableVehicleContainer> GetPlayableVehicles()
	{
		return m_mPlayableVehicles;
	}

	// --------------------------------- Player faction key ---------------------------------------
	// Get player Factionkey or empty string if no player found
	// - Synced on clients
	FactionKey GetPlayerFactionKey(int playerId)
	{
		if (!m_playersFaction.Contains(playerId))
			return "";
		return m_playersFaction[playerId];
	}
	// Get last player not null Factionkey or empty string if no player found
	// - Synced on clients
	FactionKey GetPlayerFactionKeyRemembered(int playerId)
	{
		if (!m_playersFactionRemembered.Contains(playerId))
			return "";
		return m_playersFactionRemembered[playerId];
	}
	// Set player FactionKey
	// - Execute ONLY on server
	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		// Replicate update
		RPC_SetPlayerFactionKey(playerId, factionKey);
		Rpc(RPC_SetPlayerFactionKey, playerId, factionKey);

		// Update vanilla faction
		PlayerController playerController = m_PlayerManager.GetPlayerController(playerId);
		if (!playerController)
			return;
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected protected void RPC_SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		FactionKey factionKeyOld = GetPlayerFactionKey(playerId);
		m_playersFaction[playerId] = factionKey;
		if (factionKey != "")
			m_playersFactionRemembered[playerId] = factionKey; // Remember last not null
		m_eOnFactionChange.Invoke(playerId, factionKey, factionKeyOld);
	}

	// ------------------------------------- Player state -----------------------------------------
	// Get current player sloting state. TODO: proper naming
	// - Synced on clients
	PS_EPlayableControllerState GetPlayerState(int playerId)
	{
		PS_EPlayableControllerState state = PS_EPlayableControllerState.NotReady;
		m_playersStates.Find(playerId, state);
		return state;
	}
	// Set player sloting state
	// - Execute ONLY on server
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		RPC_SetPlayerState(playerId, state);
		Rpc(RPC_SetPlayerState, playerId, state);
		
		// Try start counter
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameModeCoop.GetState();
		if (gameModeState == SCR_EGameModeState.SLOTSELECTION)
		{
			m_CallQueue.Remove(StartTime);
			bool adminExist = !gameModeCoop.IsAdminMode();
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int otherPlayerId : players)
			{
				if (!adminExist)
					adminExist = SCR_Global.IsAdmin(otherPlayerId);

				PS_EPlayableControllerState playerState = m_playersStates[otherPlayerId];
				if (playerState != PS_EPlayableControllerState.Ready)
				{
					if (m_iStartTimerCounter != -1)
					{
						m_iStartTimerCounter = -1;
						Replication.BumpMe();
						OnStartTimerCounterChanged();
					}
					return;
				}
			}

			if (adminExist)
			{
				m_iStartTimerCounter = 3;
				Replication.BumpMe();
				OnStartTimerCounterChanged();
				m_CallQueue.CallLater(StartTime, 1000, true);
			}
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		m_playersStates[playerId] = state;
		m_eOnPlayerStateChange.Invoke(playerId, state);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
			if (playableContainer)
				playableContainer.GetOnPlayerStateChange().Invoke(state);
		}
	}

	// ------------------------------------ player name -------------------------------------------
	// Get cached player name or empty string if no player found
	// - Synced on clients
	string GetPlayerName(int playerId)
	{
		if (!m_playersLastName.Contains(playerId))
			return "";
		return m_playersLastName[playerId];
	}
	// Set cached player name
	// - Execute ONLY on server
	void SetPlayerName(int playerId, string playerName)
	{
		RPC_SetPlayerName(playerId, playerName);
		Rpc(RPC_SetPlayerName, playerId, playerName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerName(int playerId, string playerName)
	{
		m_playersLastName[playerId] = playerName;
		m_eOnPlayerConnected.Invoke(playerId);
	}

	// -------------------------------- Faction ready state ---------------------------------------
	// Get faction ready state
	// - Synced on clients
	int GetFactionReady(FactionKey factionKey)
	{
		return m_mFactionReady[factionKey];
	}
	// Set faction ready state
	// - Execute ONLY on server
	void SetFactionReady(FactionKey factionKey, int readyValue)
	{
		RPC_SetFactionReady(factionKey, readyValue);
		Rpc(RPC_SetFactionReady, factionKey, readyValue);

		// All factions ready message already sended
		if (m_bFactionsReadySended)
			return;

		// Check is all factions ready
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		m_bFactionsReadySended = true;
		foreach (int playerId : players)
		{
			factionKey = GetPlayerFactionKey(playerId);
			if (factionKey == "")
				continue;
			if (m_mFactionReady[factionKey])
				continue;
			m_bFactionsReadySended = false;
			break;
		}

		// Send message to admins, if all factions ready
		if (m_bFactionsReadySended)
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("tmsg");
			invoker.Invoke(null, "Factions ready");
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetFactionReady(FactionKey factionKey, int readyValue)
	{
		m_mFactionReady[factionKey] = readyValue;
		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
	}

	// ------------------------------- Playable prefab name ---------------------------------------
	// Get playable prefab name (ResourceName) by PlayableRplId
	// - Synced on clients
	string GetPlayablePrefab(RplId playableId)
	{
		if (!m_mPlayablePrefabs.Contains(playableId))
			return "";
		return m_mPlayablePrefabs[playableId];
	}
	// Set playable cached prefab name (ResourceName)
	// - Execute ONLY on server
	void SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		Rpc(RPC_SetPlayablePrefab, playableId, prefab);
		RPC_SetPlayablePrefab(playableId, prefab);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		m_mPlayablePrefabs[playableId] = prefab;
	}

	// ------------------------------------ Playable name -----------------------------------------
	// Get playable cached name by playable id
	// - Synced on clients
	string GetPlayableName(RplId playableId)
	{
		if (!m_mPlayableNames.Contains(playableId))
			return "";
		return m_mPlayableNames[playableId];
	}
	// Set playable cached name by playable id
	// - Execute ONLY on server
	void SetPlayableName(RplId playableId, string name)
	{
		Rpc(RPC_SetPlayableName, playableId, name);
		RPC_SetPlayableName(playableId, name);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayableName(RplId playableId, string name)
	{
		m_mPlayableNames[playableId] = name;
	}

	// ---------------------- playable -> player / player -> playable links -----------------------
	// Player -> Playable link
	// Get player id by playable id or -1 if no player found
	// - Synced on clients
	int GetPlayerByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayers.Contains(PlayableId))
			return -1;
		return m_playablePlayers[PlayableId];
	}
	// Get last not -1 player id by playable id or -1 if no player found
	// - Synced on clients
	int GetPlayerByPlayableRemembered(RplId PlayableId)
	{
		if (!m_playersPlayableRemembered.Contains(PlayableId))
			return -1;
		return m_playersPlayableRemembered[PlayableId];
	}
	// Set player -> playable / playable -> player links, by playable id to player id
	// - Execute ONLY on server
	void SetPlayablePlayer(RplId playableId, int playerId)
	{
		RPC_SetPlayablePlayer(playableId, playerId);
		Rpc(RPC_SetPlayablePlayer, playableId, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePlayer(RplId playableId, int playerId)
	{
		// Reset previous player - playable -> player link
		if (playerId > 0) {
			RplId oldPlayable = GetPlayableByPlayer(playerId);
			if (oldPlayable != RplId.Invalid())
			{
				m_playablePlayers[oldPlayable] = -1;
			}
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(playerId, -1);
		}

		// Update both maps
		m_playersPlayable[playerId] = playableId;
		int oldPlayerId = m_playablePlayers[playableId];
		m_playablePlayers[playableId] = playerId;
		if (oldPlayerId > 0 && oldPlayerId != playerId)
			m_playersPlayable[oldPlayerId] = -1;
		
		// Remember last valid
		if (playableId != RplId.Invalid()) {
			m_playablePlayersRemembered[playerId] = playableId;
		}
		
		// Invoke if player valid
		if (playerId > 0)
		{
			m_playersPlayableRemembered[playableId] = playerId; // Remember last valid
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
		}
		
		// Invoke container event
		PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
		if (playableContainer)
			playableContainer.InvokeOnPlayerChanged(oldPlayerId, playerId);
	}

	// Playable -> Player link
	// Get playable id by player id or RplId.Invalid() if no playable found
	// - Synced on clients
	RplId GetPlayableByPlayer(int playerId)
	{
		if (!m_playersPlayable.Contains(playerId))
			return RplId.Invalid();
		return m_playersPlayable[playerId];
	}
	// Get last not RplId.Invalid() playable id byt player id or RplId.Invalid() if no playable found
	// - Synced on clients
	RplId GetPlayableByPlayerRemembered(int playerId)
	{
		if (!m_playablePlayersRemembered.Contains(playerId))
			return RplId.Invalid();
		return m_playablePlayersRemembered[playerId];
	}
	// Set playable -> player / player -> playable links, by player id to playable id
	// - Execute ONLY on server
	void SetPlayerPlayable(int playerId, RplId playableId)
	{
		RPC_SetPlayerPlayable(playerId, playableId);
		Rpc(RPC_SetPlayerPlayable, playerId, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerPlayable(int playerId, RplId playableId)
	{
		// Reset previous playable - playable -> player link
		RplId oldPlayable = GetPlayableByPlayer(playerId);
		if (oldPlayable != RplId.Invalid()) {
			m_playablePlayers[oldPlayable] = -1;
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(playerId, -1);
		}

		// Update both maps
		m_playersPlayable[playerId] = playableId;
		int oldPlayerId = m_playablePlayers[playableId];
		m_playablePlayers[playableId] = playerId;

		// Remember last valid
		if (playableId != RplId.Invalid()) {
			m_playablePlayersRemembered[playerId] = playableId;
		}

		// Invoke if playable valid
		if (playerId > 0)
		{
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
			m_playersPlayableRemembered[playableId] = playerId; // Remember last valid
		}

		// Invoke container event
		PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
		if (playableComponent)
			playableComponent.InvokeOnPlayerChanged(oldPlayerId, playerId);
	}
	
	// ------------------------------ Current playable controller ----------------------------------
	static PS_PlayableControllerComponent GetPlayableController()
	{
		return s_CurrentPlayableController;
	}

	// ----------------------------- Playable to players group link --------------------------------
	// Get players group by playable id or null if no group found
	// - Synced on clients
	SCR_AIGroup GetPlayerGroupByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayerGroupId.Contains(PlayableId))
			return null;

		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(m_playablePlayerGroupId[PlayableId]);
	}
	// Get players group int callsign by playable id or -1 if no group found
	// - Synced on clients
	int GetGroupCallsignByPlayable(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group)
			return -1;

		return group.GetCallsignNum();
	}
	// Set playable players group id
	// - Execute ONLY on server
	void SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		RPC_SetPlayablePlayerGroupId(PlayableId, groupId);
		Rpc(RPC_SetPlayablePlayerGroupId, PlayableId, groupId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		m_playablePlayerGroupId[PlayableId] = groupId;
		UpdatePlayablesSorted(); // Group added resort list (TODO: check is it required)
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_eOnPlayableChangeGroup.Invoke(PlayableId, GetPlayableById(PlayableId), groupsManagerComponent.FindGroup(groupId));
	}

	// --------------------------- Vehicle to players group link ----------------------------------
	// Get players group by vehicle container or null if no group found
	// - Synced on clients
	SCR_AIGroup GetPlayerGroupByVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(playableVehicleContainer.m_iGroupId);
	}
	
	// -------------------------------- Vehicle lock state ----------------------------------------
	void SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		RPC_SetPlayableVehicleLocked(vehicleId, lock);
		Rpc(RPC_SetPlayableVehicleLocked, vehicleId, lock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		if (!m_mPlayableVehicles.Contains(vehicleId))
			return;
		m_mPlayableVehicles[vehicleId].SetLock(lock);
	}

	// ---------------------------------- Player pin state ----------------------------------------
	// Get player pinned state
	// - Synced on clients
	bool GetPlayerPin(int playerId)
	{
		if (!m_playersPin.Contains(playerId))
			return false;
		return m_playersPin[playerId];
	}
	// Set player pin state
	// - Execute ONLY on server
	void SetPlayerPin(int playerId, bool pined)
	{
		RPC_SetPlayerPin(playerId, pined);
		Rpc(RPC_SetPlayerPin, playerId, pined);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerPin(int playerId, bool pined)
	{
		m_playersPin[playerId] = pined;
		m_eOnPlayerPinChange.Invoke(playerId, pined);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerPinChange().Invoke(pined);
		}
	}
	
	// ------------------------------- Max server players count -----------------------------------
	// Get max players server count
	// - Synced on clients
	int GetMaxPlayers()
	{
		return m_iMaxPlayersCount;
	}
	
	// --------------------------------------------------------------------------------------------
	// ----------------------------------- Client requests ----------------------------------------
	// --------------------------------------------------------------------------------------------
	// Send message to player when he got kicked frop playable slot
	void NotifyKick(int playerId)
	{
		RPC_NotifyKick(playerId);
		Rpc(RPC_NotifyKick, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyKick(int playerId)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController && playerId == playerController.GetPlayerId())
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("lmsg");
			invoker.Invoke(null, "#PS-Lobby_RoleKick");
		}
	}
	
	// --------------------------------------------------------------------------------------------
	// Force switch player state to game, for proper playable apply
	// Or else player can stack in other menu
	void ForceSwitch(int playerId)
	{
		// Send it ot everyone (TODO: Use owner)
		RPC_ForceSwitch(playerId);
		Rpc(RPC_ForceSwitch, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ForceSwitch(int playerId)
	{
		// Check is it our id
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController || playerController.GetPlayerId() != playerId)
			return;

		// Do full GAME state enter logic
		s_CurrentPlayableController.SwitchToMenu(SCR_EGameModeState.GAME);
	}


	// --------------------------------------------------------------------------------------------
	// ------------------------------------------ Events ------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Already replicated to clients by vanilla, just raise custom event
	protected void OnPlayerConnected(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerConnected().Invoke(playerId);
	}

	// --------------------------------------------------------------------------------------------
	// Manually replicate and invoke player disconnect event on all clients and server
	protected void OnPlayerDisconnected(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		Rpc(RPC_OnPlayerDisconnected, playerId, cause, timeout);
		RPC_OnPlayerDisconnected(playerId, cause, timeout);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerDisconnected().Invoke(playerId);
		m_eOnPlayerDisconnected.Invoke(playerId, cause, timeout);
	}

	// --------------------------------------------------------------------------------------------
	// Player got/lost admin role 
	protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerRoleChange().Invoke(playerId, roleFlags);
	}

	// --------------------------------------------------------------------------------------------
	// Manually replicate and invoke damage state change event on all clients and server
	void OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		Rpc(RPC_OnPlayableDamageStateChanged, playableId, damageState);
		RPC_OnPlayableDamageStateChanged(playableId, damageState);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		m_aPlayables[playableId].OnDamageStateChanged(damageState);
	}


	// --------------------------------------------------------------------------------------------
	// ------------------------------------ Util global -------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Remove playable entities without link to player
	void RemoveRedundantUnits()
	{
		for (int i = 0; i < m_aPlayables.Count(); i++)
		{
			PS_PlayableContainer playable = m_aPlayables.GetElement(i);
			if (GetPlayerByPlayable(playable.GetRplId()) == -2 || (GetPlayerByPlayable(playable.GetRplId()) <= 0 && m_GameModeCoop.GetRemoveRedundantUnits()))
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetPlayableComponent().GetOwner());
				if (character)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(character);
					m_CallQueue.Call(RemoveRedundantUnits);
					return;
				}
			}
		}
		
		foreach (RplId vehicleId, PS_PlayableVehicleContainer playableVehicleContainer : m_mPlayableVehicles)
		{
			if (playableVehicleContainer.GetLock())
			{
				IEntity entity = IEntity.Cast(Replication.FindItem(playableVehicleContainer.GetRplId()));
				if (entity)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
					m_CallQueue.Call(RemoveRedundantUnits);
					return;
				}
			}
		}
	}
	// --------------------------------------------------------------------------------------------
	// Holster weapon on all playables (TODO: check weapon stuck bug)
	void HolsterWeapons()
	{
		if (!Replication.IsServer())
			return;

		foreach (RplId id, PS_PlayableContainer playable : m_aPlayables)
		{
			playable.GetPlayableComponent().HolsterWeapon();
		}
	}

	// --------------------------------------------------------------------------------------------
	// Sort playables cached list by CallSign -> Rank -> RplId
	protected void UpdatePlayablesSorted()
	{
		array<PS_PlayableContainer> playablesSorted = {};
		map<RplId, ref PS_PlayableContainer> playables = GetPlayables();

		// Rerange playables from global list
		foreach (RplId playableId, PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;
			int callSign = GetGroupCallsignByPlayable(playable.GetRplId());
			bool isInserted = false;
			for (int s = 0; s < playablesSorted.Count(); s++)
			{
				PS_PlayableContainer playableS = playablesSorted[s];
				int callSignS = GetGroupCallsignByPlayable(playableS.GetRplId());

				bool rplIdGreater = playableS.GetRplId() > playable.GetRplId();
				bool rankEquival = playable.GetCharacterRank() == playableS.GetCharacterRank();
				bool rankGreater = playable.GetCharacterRank() > playableS.GetCharacterRank();
				bool callSignEquival = callSignS == callSign;
				bool callSignGreater = callSignS > callSign;

				// CallSign -> Rank -> RplId
				if ((((rplIdGreater && rankEquival) || rankGreater) && callSignEquival) || callSignGreater) {
					playablesSorted.InsertAt(playable, s);
					isInserted = true;
					break;
				}
			}
			if (!isInserted) {
				playablesSorted.Insert(playable);
			}
		}

		// Update cached list
		m_aPlayablesSorted = playablesSorted;
	}
	// One frame delay
	protected void UpdatePlayablesSortedDelayed()
	{
		m_CallQueue.Remove(UpdatePlayablesSorted);
		m_CallQueue.Call(UpdatePlayablesSorted);
	}

	// --------------------------------------------------------------------------------------------
	// ---------------------------------------- Util ----------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Check is player control group leader playable
	bool IsPlayerGroupLeader(int thisPlayerId)
	{
		if (thisPlayerId == -1)
			return false;

		RplId thisPlayableId = GetPlayableByPlayer(thisPlayerId);
		if (thisPlayableId == RplId.Invalid())
			return false;

		int thisGroupCallsign = GetGroupCallsignByPlayable(thisPlayableId);

		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			RplId playableId = playable.GetRplId();
			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (playerId == thisPlayerId)
				return true;
			if (GetPlayerFactionKey(playerId) != GetPlayerFactionKey(thisPlayerId))
				continue;
			int groupCallsign = GetGroupCallsignByPlayable(playableId);
			if (thisGroupCallsign != groupCallsign)
				continue;
			return false;
		}

		return true;
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------ Replication -------------------------------------------
	// --------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save maps
		PS_ReplicationHelper.WriteMapIntInt(writer, m_playersStates);
		PS_ReplicationHelper.WriteMapIntRplId(writer, m_playersPlayable);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayers);
		PS_ReplicationHelper.WriteMapIntBool(writer, m_playersPin);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFaction);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFactionRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayerGroupId);
		PS_ReplicationHelper.WriteMapIntString(writer, m_playersLastName);
		PS_ReplicationHelper.WriteMapIntRplId(writer, m_playersPlayableRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayersRemembered);
		PS_ReplicationHelper.WriteMapFactionKeyInt(writer, m_mFactionReady);
		PS_ReplicationHelper.WriteMapRplIdString(writer, m_mPlayablePrefabs);
		PS_ReplicationHelper.WriteMapRplIdString(writer, m_mPlayableNames);

		// Save containers
		int playablesCount = m_aPlayables.Count();
		writer.WriteInt(playablesCount);
		foreach (RplId id, PS_PlayableContainer container : m_aPlayables)
		{
			container.Save(writer);
		}

		int playableVehiclessCount = m_mPlayableVehicles.Count();
		writer.WriteInt(playableVehiclessCount);
		foreach (RplId id, PS_PlayableVehicleContainer container : m_mPlayableVehicles)
		{
			container.Save(writer);
		}

		return true;
	}

	// --------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load maps
		PS_ReplicationHelper.ReadMapIntInt(reader, m_playersStates);
		PS_ReplicationHelper.ReadMapIntRplId(reader, m_playersPlayable);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayers);
		PS_ReplicationHelper.ReadMapIntBool(reader, m_playersPin);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFaction);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFactionRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayerGroupId);
		PS_ReplicationHelper.ReadMapIntString(reader, m_playersLastName);
		PS_ReplicationHelper.ReadMapIntRplId(reader, m_playersPlayableRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayersRemembered);
		PS_ReplicationHelper.ReadMapFactionKeyInt(reader, m_mFactionReady);
		PS_ReplicationHelper.ReadMapRplIdString(reader, m_mPlayablePrefabs);
		PS_ReplicationHelper.ReadMapRplIdString(reader, m_mPlayableNames);

		// Load containers
		int playablesCount;
		reader.ReadInt(playablesCount);
		for (int i = 0; i < playablesCount; i++)
		{
			PS_PlayableContainer container = new PS_PlayableContainer();
			container.Load(reader);
			RPC_RegisterPlayable(container);
		}

		int playableVehiclesCount;
		reader.ReadInt(playableVehiclesCount);
		for (int i = 0; i < playableVehiclesCount; i++)
		{
			PS_PlayableVehicleContainer container = new PS_PlayableVehicleContainer();
			container.Load(reader);
			RPC_RegisterGroupVehicle(container);
		}

		m_bRplLoaded = true;

		return true;
	}
}
