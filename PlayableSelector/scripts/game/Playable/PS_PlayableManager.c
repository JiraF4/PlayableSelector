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

void PS_ScriptInvokerGroupReadyChangeMethod(int groupId, int readyValue);
typedef func PS_ScriptInvokerGroupReadyChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerGroupReadyChangeMethod> PS_ScriptInvokerGroupReadyChange;

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
	ref map<int, int> m_mGroupReady = new map<int, int>(); // groupId → squad ready state (0/1)
	ref map<RplId, string> m_mPlayablePrefabs = new map<RplId, string>();
	// Role display info keyed by PREFAB (deduplicated - many playables share a role prefab), instead of
	// shipping the same long icon path/name on all 128 containers.
	ref map<string, string> m_mPrefabRoleIcon = new map<string, string>();
	ref map<string, string> m_mPrefabRoleQuad = new map<string, string>();
	ref map<string, string> m_mPrefabRoleName = new map<string, string>();

	// Server-only reconnect cache, keyed by player GUID (a reconnecting player gets a NEW playerId,
	// so all the playerId-keyed state above is lost - this lets us restore faction/slot/pin/name).
	protected ref map<string, RplId> m_mReconnectPlayable = new map<string, RplId>();
	protected ref map<string, FactionKey> m_mReconnectFaction = new map<string, FactionKey>();
	protected ref map<string, bool> m_mReconnectPin = new map<string, bool>();
	protected ref map<string, string> m_mReconnectName = new map<string, string>();
	// Diagnostic only: connect-time world-time (ms) per RECONNECTING playerId, so the reconnect -> Global ->
	// group/Command VoN transition can be logged with elapsed timing (TraceReconnectConnect + RestorePlayerReconnectData).
	protected ref map<int, float> m_mReconnectTraceTime = new map<int, float>();
	// Server-only: player IDs whose RestorePlayerReconnectData CallLater should be skipped.
	// Enfusion's Callqueue.Remove() only accepts a function reference (no extra args), so we
	// cannot cancel a specific CallLater(func, arg). Instead, OnPlayerDisconnected flags the
	// playerId here and RestorePlayerReconnectData checks on entry.
	protected ref map<int, bool> m_mReconnectCancelled = new map<int, bool>();

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
	ref PS_ScriptInvokerGroupReadyChange m_eGroupReadyChanged = new PS_ScriptInvokerGroupReadyChange();
	PS_ScriptInvokerGroupReadyChange GetOnGroupReadyChanged()
	{
		return m_eGroupReadyChanged;
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
	
	// TODO: Remove?
	[RplProp(onRplName: "OnStartTimerCounterChanged")]
	int m_iStartTimerCounter = -1;
	void StartTime()
	{
		m_iStartTimerCounter -= 1;
		Replication.BumpMe();
		OnStartTimerCounterChanged();
		if (m_iStartTimerCounter == 0)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			// Use the CURRENT state, not a hardcoded one — this timer is reused by both
			// slot-selection "all ready" and briefing "faction ready" countdowns.
			gameModeCoop.AdvanceGameState(gameModeCoop.GetState());
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
			if (playableContainer && playableContainer.GetDamageState() == EDamageState.DESTROYED)
			{
				m_CallQueue.CallLater(DelayedSwitchToInitialEntity, 1000, false, playerId);
			}
		}

		IEntity entity;
		if (playableId == RplId.Invalid()) { // no slot: lobby / spectator
			// Body-less: control NOTHING. Lobby players never had a body; a just-died player keeps
			// their corpse as the controlled entity. The client spectator camera (PS_SpectatorManager)
			// and the VoN proxy (PS_MenuVoN) handle view + voice. The engine has no working
			// SetControlledEntity(null), so we simply do not assign a controlled entity here.
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);
			SetPlayerFactionKey(playerId, "");
			return;
		} else {
			PS_PlayableContainer applyContainer = GetPlayableById(playableId);
			PS_PlayableComponent applyPlayable;
			if (applyContainer)
				applyPlayable = applyContainer.GetPlayableComponent();
			if (!applyPlayable || !applyPlayable.GetOwner())
				return; // stale link, playable entity is already gone
			entity = applyPlayable.GetOwner();
		}

		// Delete initial entity if exists
		IEntity initialEntity = playableController.GetInitialEntity();
		if (initialEntity)
			m_CallQueue.Call(SCR_EntityHelper.DeleteEntityAndChildren, initialEntity);

		// Apply entity immediately. At game start / respawn the playables have been force-streamed all
		// through the briefing, so the control switch is not a streaming burst and the player should take
		// control with no delay. The spectator transitions stage their streaming separately via the
		// deferred spectator observer, so no preload handshake is needed on this slot-apply path.
		playerController.SetInitialMainEntity(entity);

		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		if (playableCharacter)
		{
			SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
			if (faction)
				SetPlayerFactionKey(playerId, faction.GetFactionKey());
		}

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
		string prefab = playableComponent.GetOwner().GetPrefabData().GetPrefabName();
		SetPlayablePrefab(playableId, prefab);
		// Role icon/name are static per prefab - stored once per prefab instead of on every container
		SetPrefabRoleInfo(prefab, playableComponent.GetRoleIconPath(), playableComponent.GetRoleIconQuad(), playableComponent.GetRoleName());
		// Name is carried by the container itself, no separate replicated map needed

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
		if (!playableGroup || !playerGroup)
		{
			m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup);
			return;
		}

		// Assign manualy set callsigns
		PS_GroupCallsignAssigner groupCallsignAssigner = PS_GroupCallsignAssigner.Cast(playableGroup.FindComponent(PS_GroupCallsignAssigner));
		int company, platoon, squad;
		if (groupCallsignAssigner) {
			groupCallsignAssigner.GetCallsign(company, platoon, squad);
		} else {
			SCR_CallsignGroupComponent sourceCallsign = SCR_CallsignGroupComponent.Cast(playableGroup.FindComponent(SCR_CallsignGroupComponent));
			if (sourceCallsign)
				sourceCallsign.GetCallsignIndexes(company, platoon, squad);
		}
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		if (callsignComponent)
			callsignComponent.ReAssignGroupCallsign(company, platoon, squad);

		m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup) // Delay for callsign init
	}
	protected void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		if (!playerGroup)
			return;
		// Create VoN group channels. The GROUP channel is keyed by the UNIQUE GROUP ID (GroupVonRoomName),
		// NOT the callsign number - callsign numbers can collide between groups (or be unassigned), which
		// would collapse different groups onto one channel = cross-group voice leak. (Mirrors LiteLobby.)
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		Faction faction = playerGroup.GetFaction();
		if (!faction)
			return;
		FactionKey factionKey = faction.GetFactionKey();
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, GroupVonRoomName(playerGroup.GetGroupID()));
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, "#PS-VoNRoom_Faction");
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
		// This can be re-queued onto OnUpdate (below); the vehicle may have been deleted by the time it
		// runs, leaving a null reference - bail rather than dereferencing it.
		if (!vehicle)
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
		// A vehicle missing its editable component / faction affiliation can't be turned into a
		// playable slot; bail out instead of throwing a VME from the OnUpdate retry path. Log it so a
		// genuinely-misconfigured vehicle (expected to be playable but missing a component) is findable
		// rather than silently dropped.
		if (!editableVehicleComponent || !vehicleFactionAffiliationComponent)
		{
			Print(string.Format("[PS] RegisterGroupVehicle: skipping vehicle '%1' - not a valid playable vehicle (missing SCR_EditableVehicleComponent or SCR_VehicleFactionAffiliationComponent)", vehicle), LogLevel.WARNING);
			return;
		}
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		if (!uIInfo)
			return;
		ResourceName prefab = vehicle.GetPrefabData().GetPrefabName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetResourceName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetAncestor().GetResourceName();
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

	// ------------------------------------- Reconnect restore -----------------------------------------
	// A reconnecting player is assigned a NEW playerId, so every playerId-keyed map above is empty for
	// them. Vanilla SCR_ReconnectComponent restores the controlled character but sets the faction
	// affiliation from that entity - which is the factionless lobby/VoN body during preview/lobby/
	// briefing - leaving the player with an empty faction and therefore the WRONG side's map markers.
	// We cache the relevant state by GUID on disconnect and re-apply it on reconnect.
	// - Execute ONLY on server
	void StorePlayerReconnectData(int playerId)
	{
		if (!Replication.IsServer())
			return;
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid == "")
			return;
		FactionKey faction = GetPlayerFactionKey(playerId);
		RplId playable = GetPlayableByPlayer(playerId);
		if (faction == "" && playable == RplId.Invalid())
			return; // nothing worth restoring (player never slotted/picked a faction)
		m_mReconnectPlayable[guid] = playable;
		m_mReconnectFaction[guid] = faction;
		m_mReconnectPin[guid] = GetPlayerPin(playerId);
		// Cache the player name (incl. PodvalPatches clan tag) so the reconnecting player's
		// display name survives the playerId change without waiting for PodvalPatches to re-apply it.
		string name = GetPlayerName(playerId);
		if (name != "")
			m_mReconnectName[guid] = name;
	}
	// - Execute ONLY on server
	void ClearPlayerReconnectData(int playerId)
	{
		if (!Replication.IsServer())
			return;
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid == "")
			return;
		m_mReconnectPlayable.Remove(guid);
		m_mReconnectFaction.Remove(guid);
		m_mReconnectPin.Remove(guid);
		m_mReconnectName.Remove(guid);
	}
	// - Execute ONLY on server
	// Diagnostic (server, once per reconnect): called from SpawnInitialEntity right after the connect-time
	// AssignPhaseVoiceChannel. If this connection is a pending RECONNECT (cached GUID data not yet consumed by
	// RestorePlayerReconnectData), record the connect time and log the IMMEDIATE VoN channel - this is before the
	// slot is restored, so it is expected to be Global. RestorePlayerReconnectData logs the matching "slot restored
	// after Xms" line, making the reconnect -> Global -> group/Command transition + timing explicit in the log.
	void TraceReconnectConnect(int playerId)
	{
		if (!Replication.IsServer())
			return;
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid == "" || !m_mReconnectFaction.Contains(guid))
			return; // genuinely fresh join, not a reconnect
		m_mReconnectTraceTime.Set(playerId, GetGame().GetWorld().GetWorldTime());
		string channel = "";
		PS_VoNRoomsManager von = PS_VoNRoomsManager.GetInstance();
		if (von)
			channel = von.GetPlayerChannel(playerId);
		Print(string.Format("[PS_ReconnectTrace] player %1 RECONNECT connected: immediate VoN channel='%2' (slot not restored yet; expect group/Command in ~2.5s)", playerId, channel), LogLevel.NORMAL);
	}

	void CancelPendingReconnectRestore(int playerId)
	{
		m_mReconnectCancelled[playerId] = true;
	}

	void RestorePlayerReconnectData(int playerId)
	{
		if (!Replication.IsServer())
			return;
		// Guard: if this playerId was flagged as cancelled (player disconnected before
		// the 2500ms delay), skip execution so we do not consume the GUID cache for a
		// ghost playerId that would lock the real player out on their next reconnect.
		if (m_mReconnectCancelled.Contains(playerId))
		{
			m_mReconnectCancelled.Remove(playerId);
			return;
		}
		string guid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (guid == "" || !m_mReconnectFaction.Contains(guid))
			return;

		RplId playable = m_mReconnectPlayable[guid];
		FactionKey faction = m_mReconnectFaction[guid];
		bool pin = m_mReconnectPin[guid];
		string reconnectName;
		m_mReconnectName.Find(guid, reconnectName);

		// Consume so a later fresh join by the same account does not pick up stale state
		m_mReconnectPlayable.Remove(guid);
		m_mReconnectFaction.Remove(guid);
		m_mReconnectPin.Remove(guid);
		m_mReconnectName.Remove(guid);

		// Re-link the slot only if it is still theirs / free / held by a now-gone ghost id,
		// never steal a slot a live player has taken in the meantime
		if (playable != RplId.Invalid() && GetPlayableById(playable))
		{
			int holder = GetPlayerByPlayable(playable);
			bool free = holder <= 0 || holder == playerId || !m_PlayerManager.IsPlayerConnected(holder);
			if (free)
				SetPlayerPlayable(playerId, playable);
		}

		// Re-apply faction so markers match the player's side again
		if (faction != "")
			SetPlayerFactionKey(playerId, faction);

		if (pin)
			SetPlayerPin(playerId, true);

		// Restore the cached player name (incl. PodvalPatches clan tag) under the new playerId
		// so UI shows the correct name immediately, without waiting for PodvalPatches to re-apply it.
		if (reconnectName != "")
			SetPlayerName(playerId, reconnectName);

		// Diagnostic - once per reconnect restore (non-spammy). Pairs with the [PS_VoNDBG] channel line that
		// follows from AssignPhaseVoiceChannel below, so reconnect slot/faction/channel can be traced together.
		Print(string.Format("[PS_ReconnectDBG] player %1 reconnect-restored: faction='%2' hasSlot=%3 pin=%4",
			playerId, faction, GetPlayableByPlayer(playerId) != RplId.Invalid(), pin), LogLevel.NORMAL);

		// If reconnecting mid-GAME into a re-linked slot, put the player back INTO their playable
		// (ApplyPlayable controls it; a dead slot routes to spectator). In lobby/briefing they have no
		// playable yet - they keep the reserved slot and get it at game start. Body-less: a reconnecting
		// player controls nothing until this runs, so without it they would be stuck spectating with
		// their slot restored but never entered.
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode && gameMode.GetState() == SCR_EGameModeState.GAME && GetPlayableByPlayer(playerId) != RplId.Invalid())
			ApplyPlayable(playerId);
		else if (gameMode)
			// Reconnect gets a NEW playerId with an empty VoN channel (RestoreRoom set it to ""); re-assign the
			// proper briefing/lobby channel for the restored slot, or the player is stuck in "" - out of their
			// group, and two such reconnects would share "" (cross-faction leak). GAME+slotted is handled by
			// ApplyPlayable above (alive proxy stays parked, dead routes to spectator -> global).
			gameMode.AssignPhaseVoiceChannel(playerId);

		// Diagnostic: pair with TraceReconnectConnect - log the POST-restore VoN channel + elapsed since connect,
		// so the reconnect -> Global -> group/Command transition + timing is unmistakable in one place.
		float traceT0;
		if (m_mReconnectTraceTime.Find(playerId, traceT0))
		{
			string channelNow = "";
			PS_VoNRoomsManager vonNow = PS_VoNRoomsManager.GetInstance();
			if (vonNow)
				channelNow = vonNow.GetPlayerChannel(playerId);
			Print(string.Format("[PS_ReconnectTrace] player %1 RECONNECT slot restored after %2ms: VoN channel now='%3' hasSlot=%4 faction='%5'",
				playerId, GetGame().GetWorld().GetWorldTime() - traceT0, channelNow, GetPlayableByPlayer(playerId) != RplId.Invalid(), faction), LogLevel.NORMAL);
			m_mReconnectTraceTime.Remove(playerId);
		}
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
		// Cache hit with a real name: return it. PodvalLobby/PodvalPatches store the
		// squad-prefixed display name (<b><color..>[PREFIX]</color></b>BaseName) via SetPlayerName,
		// so we must prefer the cache over the raw engine name to preserve clan tags.
		// An empty-string entry means the engine hadn't resolved the Steam name yet when
		// OnPlayerConnected fired (cache poisoned) — treat it as a miss so we re-query.
		string cachedName;
		if (m_playersLastName.Find(playerId, cachedName) && cachedName != "")
			return cachedName;
		// Cache miss or poisoned with empty: fall back to the live engine name so name-driven
		// UI (alive list, 3D label, kill feed, voice list, lobby selectors...) shows the nickname
		// instead of the role name ("Rifleman"). OnPlayerConnected re-seeds the cache with the
		// engine name on every (re)connect before PodvalLobby re-applies the prefix, so
		// cache-first never double-wraps the tag.
		if (playerId > 0)
			return GetGame().GetPlayerManager().GetPlayerName(playerId);
		return "";
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

		// All factions ready — notify admins and start the 3-2-1 countdown
		if (m_bFactionsReadySended)
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("tmsg");
			invoker.Invoke(null, "Factions ready");

			// Reuse the same countdown as the slot-selection "all ready" timer:
			// 3 → 2 → 1 → 0 → AdvanceGameState(BRIEFING) → StartGame → GAME
			m_iStartTimerCounter = 3;
			Replication.BumpMe();
			OnStartTimerCounterChanged();
			m_CallQueue.Remove(StartTime);
			m_CallQueue.CallLater(StartTime, 1000, true);
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetFactionReady(FactionKey factionKey, int readyValue)
	{
		m_mFactionReady[factionKey] = readyValue;
		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
	}

	// -------------------------------- Squad (group) ready state ----------------------------------
	// Get squad ready state by group ID
	// - Synced on clients
	int GetGroupReady(int groupId)
	{
		if (!m_mGroupReady.Contains(groupId))
			return 0;
		return m_mGroupReady[groupId];
	}
	// Set squad ready state
	// - Execute ONLY on server
	void SetGroupReady(int groupId, int readyValue)
	{
		// Skip redundant updates: a leader re-pressing (or holding) F9/F10 on an already-ready/not-ready squad
		// would otherwise re-broadcast to EVERY client each press - needless reliable-RPC traffic during the
		// freeze-time vote when many squads spam ready at once. Only broadcast on an actual state change.
		if (GetGroupReady(groupId) == readyValue)
			return;
		RPC_SetGroupReady(groupId, readyValue);
		Rpc(RPC_SetGroupReady, groupId, readyValue);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetGroupReady(int groupId, int readyValue)
	{
		m_mGroupReady[groupId] = readyValue;
		m_eGroupReadyChanged.Invoke(groupId, readyValue);
	}
	// Reset all group ready states (called at freeze time start)
	void ResetGroupReady()
	{
		m_mGroupReady.Clear();
		Replication.BumpMe();
	}
	// Build a map of groupId → leaderPlayerId by iterating sorted playables.
	// The first occupied playable in each group (sorted by callsign→rank→rplId) is the leader.
	void GetGroupLeaders(out map<int, int> groupLeaders)
	{
		groupLeaders.Clear();
		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (m_PlayerManager && !m_PlayerManager.IsPlayerConnected(playerId))
				continue;
			SCR_AIGroup group = GetPlayerGroupByPlayable(playable.GetRplId());
			if (!group)
				continue;
			int groupId = group.GetGroupID();
			if (!groupLeaders.Contains(groupId))
				groupLeaders.Insert(groupId, playerId);
		}
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

	// ------------------------------- Playable role info (per prefab) ------------------------------
	// Store role icon/name for a prefab once (dedup). Execute ONLY on server.
	void SetPrefabRoleInfo(string prefab, string iconPath, string iconQuad, string roleName)
	{
		if (prefab == "" || m_mPrefabRoleName.Contains(prefab))
			return; // already known for this prefab
		Rpc(RPC_SetPrefabRoleInfo, prefab, iconPath, iconQuad, roleName);
		RPC_SetPrefabRoleInfo(prefab, iconPath, iconQuad, roleName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPrefabRoleInfo(string prefab, string iconPath, string iconQuad, string roleName)
	{
		m_mPrefabRoleIcon[prefab] = iconPath;
		m_mPrefabRoleQuad[prefab] = iconQuad;
		m_mPrefabRoleName[prefab] = roleName;
	}
	// Get role info for a playable (resolved playable -> prefab -> role info). Synced on clients.
	string GetPlayableRoleIconPath(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleIcon.Contains(prefab))
			return "";
		return m_mPrefabRoleIcon[prefab];
	}
	string GetPlayableRoleIconQuad(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleQuad.Contains(prefab))
			return "";
		return m_mPrefabRoleQuad[prefab];
	}
	string GetPlayableRoleName(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleName.Contains(prefab))
			return "";
		return m_mPrefabRoleName[prefab];
	}

	// ------------------------------------ Playable name -----------------------------------------
	// Get playable cached name by playable id
	// - Synced on clients (read from the playable container, which already carries the name -
	//   a separate replicated name map would duplicate it in every JIP snapshot)
	string GetPlayableName(RplId playableId)
	{
		PS_PlayableContainer container = GetPlayableById(playableId);
		if (!container)
			return "";
		return container.GetName();
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

		// INVARIANT: a seated player's faction ALWAYS matches their slot. The faction key is otherwise set by
		// a SEPARATE path (RPC_ChangeFactionKey) whose faction-balance check can reject while the slot is still
		// seated, and a reconnect restores the slot before the faction - either leaves a SEATED player with an
		// empty/stale faction key, which collapses their VoN channel across factions (see PS_VoNRoomsManager)
		// AND gives them the wrong side's map markers (faction affiliation drives both). Seating is already
		// balance/authorization-checked before this server method is reached (PS_PlayableControllerComponent.
		// RPC_SetPlayerPlayable, or an authoritative reconnect restore), so deriving the faction from the slot
		// here cannot bypass balance - it only stops faction and slot from ever disagreeing.
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer slot = GetPlayableById(playableId);
			if (slot && slot.GetFactionKey() != "" && GetPlayerFactionKey(playerId) != slot.GetFactionKey())
				SetPlayerFactionKey(playerId, slot.GetFactionKey());
		}
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

	// Unique per-group VoN room name, keyed by GROUP ID (not the callsign number). Callsign numbers are NOT
	// unique per group - two groups can share a callsign (or it is 0/unassigned during init), so keying group
	// voice channels by callsign collapses different groups onto ONE channel = the cross-group voice leak.
	// Group ids are unique. (Mirrors LiteLobby's group-id channel keying.) The "#PS-VoNRoom_Group" prefix
	// namespaces it and keeps it out of the digit=callsign display path; PS_VoiceRoomHeader maps it back to
	// the group's callsign name for display.
	static string GroupVonRoomName(int groupId)
	{
		if (groupId < 0)
			return "";
		return "#PS-VoNRoom_Group" + groupId.ToString();
	}
	// VoN group room name for a playable (resolves its player group). "" if the playable has no group.
	string GetGroupVonRoomName(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group)
			return "";
		return GroupVonRoomName(group.GetGroupID());
	}
	// Authoritative faction for a player derived from their SLOT only. Group faction is deliberately
	// excluded because TvT setups can have groups with overlapping IDs across factions, making
	// FindGroup(groupId) return a group from the wrong side. The slot's faction comes from the prefab
	// and is always reliable. "" if un-slotted (the player is legitimately factionless and belongs in
	// the Global pool). Used by the VoN reconcile safety-net and AssignPhaseVoiceChannel.
	FactionKey GetSlotFactionForPlayer(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer slot = GetPlayableById(playableId);
		if (slot && slot.GetFactionKey() != "")
			return slot.GetFactionKey();
		return "";
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

		// Voice follows the group: re-key this player's menu VoN to the new group/command channel so the AUDIO
		// matches the (always-recomputed) widget. A group change used to leave voice stranded on the OLD group's
		// channel because nothing re-routed it - the "widget fine, audio wrong" leak. Server + menu phases only
		// (in GAME alive players are parked and the dead/spectator path owns Global).
		if (Replication.IsServer())
		{
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode && gameMode.GetState() != SCR_EGameModeState.GAME)
			{
				int groupChangePlayerId = GetPlayerByPlayable(PlayableId);
				if (groupChangePlayerId > 0)
					gameMode.AssignPhaseVoiceChannel(groupChangePlayerId);
			}
		}
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

		// Leader status drives the Command (HQ) channel: re-key voice on a role change so a new leader is pulled
		// into HQ and a demoted one drops back to their group channel, keeping AUDIO in sync with the widget.
		// Nothing re-routed voice on role change before. Server + menu phases only.
		if (Replication.IsServer())
		{
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode && gameMode.GetState() != SCR_EGameModeState.GAME)
				gameMode.AssignPhaseVoiceChannel(playerId);
		}
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
	// Remove playables that should be cleaned up.
	// adminClosedOnly = false (default): delete admin-closed (-2) AND unoccupied units when
	//   m_bRemoveRedundantUnits is enabled (old behavior at freeze time end — now unused).
	// adminClosedOnly = true: delete ONLY admin-closed (-2) characters and locked vehicles,
	//   leaving unassigned-but-not-closed slots alive (called at BRIEFING → GAME transition).
	void RemoveRedundantUnits(bool adminClosedOnly = false)
	{
		for (int i = 0; i < m_aPlayables.Count(); i++)
		{
			PS_PlayableContainer playable = m_aPlayables.GetElement(i);
			int playerForPlayable = GetPlayerByPlayable(playable.GetRplId());
			bool isAdminClosed = playerForPlayable == -2;
			bool isRedundant = !adminClosedOnly && playerForPlayable <= 0 && m_GameModeCoop.GetRemoveRedundantUnits();
			if (isAdminClosed || isRedundant)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetPlayableComponent().GetOwner());
				if (character)
				{
					UnparentVehicleChildren(character);
					// Unregister BEFORE deleting: DeleteEntityAndChildren triggers child component
					// callbacks (OnParentSlotChanged, etc.) that may access m_aPlayables. If the
					// playable is still registered when children are destroyed, gadgets like
					// RHS_GarminForetrexComponent crash with "Entity is already deleted".
					UnRegisterPlayable(playable.GetRplId());
					SCR_EntityHelper.DeleteEntityAndChildren(character);
					m_CallQueue.Call(RemoveRedundantUnits, adminClosedOnly);
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
					// Unregister the vehicle from the replicated map before deletion so child
					// component callbacks do not reference a dying entity.
					UnRegisterGroupVehicle(playableVehicleContainer.GetRplId());
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
					m_CallQueue.Call(RemoveRedundantUnits, adminClosedOnly);
					return;
				}
			}
		}
	}
	
	protected void UnparentVehicleChildren(IEntity entity)
	{
		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity nextSibling = child.GetSibling();
			RplId childRplId = Replication.FindItemId(child);
			if (childRplId.IsValid() && m_mPlayableVehicles.Contains(childRplId))
			{
				SCR_AIGroup group = GetPlayerGroupByVehicle(m_mPlayableVehicles[childRplId]);
				entity.RemoveChild(child, true);
				if (group)
					group.AddChild(child, -1, EAddChildFlags.AUTO_TRANSFORM);
			}
			child = nextSibling;
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

	bool IsPlayerFactionCommander(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
			return false;

		FactionKey factionKey = GetPlayerFactionKey(playerId);
		if (factionKey == "")
			return false;

		array<int> commanderIds = {};
		GetFactionCommanders(factionKey, commanderIds);
		return commanderIds.Contains(playerId);
	}

	// Returns the player in the top (first) slot for the given faction.
	// Only one faction commander per faction — the first playable in the sorted list.
	// Skips disconnected players so the ready button doesn't get stuck on a ghost slot.
	void GetFactionCommanders(FactionKey factionKey, out array<int> commanderIds)
	{
		commanderIds.Clear();

		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			if (playable.GetFactionKey() != factionKey)
				continue;

			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (m_PlayerManager && !m_PlayerManager.IsPlayerConnected(playerId))
				continue;

			commanderIds.Insert(playerId);
			return;
		}
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------ Replication -------------------------------------------
	// --------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save maps
		// Note: m_playablePlayers (reverse of m_playersPlayable) and the playable name map
		// (carried by each container) are intentionally NOT sent - they are reconstructed on load
		// to keep the JIP snapshot smaller on full servers.
		PS_ReplicationHelper.WriteMapIntInt(writer, m_playersStates);
		PS_ReplicationHelper.WriteMapIntRplId(writer, m_playersPlayable);
		PS_ReplicationHelper.WriteMapIntBool(writer, m_playersPin);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFaction);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFactionRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayerGroupId);
		PS_ReplicationHelper.WriteMapIntString(writer, m_playersLastName);
		PS_ReplicationHelper.WriteMapIntRplId(writer, m_playersPlayableRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayersRemembered);
		PS_ReplicationHelper.WriteMapFactionKeyInt(writer, m_mFactionReady);
		PS_ReplicationHelper.WriteMapIntInt(writer, m_mGroupReady);
		PS_ReplicationHelper.WriteMapRplIdString(writer, m_mPlayablePrefabs);

		// Save per-prefab role info (deduplicated icon/quad/name)
		int roleCount = m_mPrefabRoleName.Count();
		writer.WriteInt(roleCount);
		for (int i = 0; i < roleCount; i++)
		{
			string prefab = m_mPrefabRoleName.GetKey(i);
			writer.WriteString(prefab);
			writer.WriteString(m_mPrefabRoleIcon[prefab]);
			writer.WriteString(m_mPrefabRoleQuad[prefab]);
			writer.WriteString(m_mPrefabRoleName[prefab]);
		}

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

		// [PS_NetStat] JIP snapshot composition - logged on the SERVER each time a client joins, so it shows
		// the real per-join lobby payload at live player counts (the prime lobby-kick suspect). The whole
		// blob is sent to the joining client in one burst; watch this when lobby-stage kicks happen.
		if (PS_NetStat.s_bEnabled)
			Print(string.Format("[PS_NetStat][SERVER] JIP snapshot: playables=%1 vehicles=%2 states=%3 factions=%4 groups=%5 names=%6 roleDefs=%7",
				playablesCount, playableVehiclessCount, m_playersStates.Count(), m_playersFaction.Count(), m_playablePlayerGroupId.Count(), m_playersLastName.Count(), roleCount), LogLevel.NORMAL);

		return true;
	}

	// --------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load maps (must match RplSave order)
		PS_ReplicationHelper.ReadMapIntInt(reader, m_playersStates);
		PS_ReplicationHelper.ReadMapIntRplId(reader, m_playersPlayable);
		PS_ReplicationHelper.ReadMapIntBool(reader, m_playersPin);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFaction);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFactionRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayerGroupId);
		PS_ReplicationHelper.ReadMapIntString(reader, m_playersLastName);
		PS_ReplicationHelper.ReadMapIntRplId(reader, m_playersPlayableRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayersRemembered);
		PS_ReplicationHelper.ReadMapFactionKeyInt(reader, m_mFactionReady);
		PS_ReplicationHelper.ReadMapIntInt(reader, m_mGroupReady);
		PS_ReplicationHelper.ReadMapRplIdString(reader, m_mPlayablePrefabs);

		// Load per-prefab role info (deduplicated icon/quad/name)
		int roleCount;
		reader.ReadInt(roleCount);
		for (int i = 0; i < roleCount; i++)
		{
			string prefab, icon, quad, name;
			reader.ReadString(prefab);
			reader.ReadString(icon);
			reader.ReadString(quad);
			reader.ReadString(name);
			m_mPrefabRoleIcon[prefab] = icon;
			m_mPrefabRoleQuad[prefab] = quad;
			m_mPrefabRoleName[prefab] = name;
		}

		// Reconstruct the reverse player<->playable map instead of replicating it
		m_playablePlayers.Clear();
		foreach (int playerId, RplId playableId : m_playersPlayable)
			m_playablePlayers[playableId] = playerId;

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
