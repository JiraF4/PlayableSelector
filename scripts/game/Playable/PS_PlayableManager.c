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
class PS_PlayableManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManager : ScriptComponent
{	
	// Map of our playables
	ref map<RplId, ref PS_PlayableContainer> m_aPlayables = new map<RplId, ref PS_PlayableContainer>(); // We NOW sync it!
	ref array<PS_PlayableContainer> m_PlayablesSorted = {};
	ref map<RplId, ref PS_PlayableVehicleContainer> m_mPlayableVehicles = new map<RplId, ref PS_PlayableVehicleContainer>;
	
	// Maps for saving players staff, player controllers local to client
	ref map<int, PS_EPlayableControllerState> m_playersStates = new map<int, PS_EPlayableControllerState>;
	ref map<int, RplId> m_playersPlayableRemembered = new map<int, RplId>;
	ref map<int, RplId> m_playersPlayable = new map<int, RplId>;
	ref map<RplId, int> m_playablePlayersRemembered = new map<RplId, int>;
	ref map<RplId, int> m_playablePlayers = new map<RplId, int>; // reversed m_playersPlayable for fast search
	ref map<int, bool> m_playersPin = new map<int, bool>; // is player pined
	ref map<int, FactionKey> m_playersFaction = new map<int, FactionKey>; // player factions
	ref map<int, FactionKey> m_playersFactionRemembered = new map<int, FactionKey>; // player factions persistant
	ref map<RplId, int> m_playablePlayerGroupId = new map<RplId, int>; // playable to player group
	ref map<int, string> m_playersLastName = new map<int, string>; // playerid to player name (persistant)
	ref map<FactionKey, int> m_mFactionReady = new map<FactionKey, int>; // faction ready state
	ref map<RplId, string> m_mPlayablePrefabs = new map<RplId, string>;
	ref map<RplId, string> m_mPlayableNames = new map<RplId, string>;
	
	
	// Invokers
	ref ScriptInvokerInt m_eOnPlayerConnected = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerConnected()
		return m_eOnPlayerConnected;
	ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
		return m_eOnPlayerDisconnected;
	ref PS_ScriptInvokerFactionChange m_eOnFactionChange = new PS_ScriptInvokerFactionChange(); // int playerId, FactionKey factionKey, FactionKey factionKeyOld
	PS_ScriptInvokerFactionChange GetOnFactionChange()
		return m_eOnFactionChange;
	ref PS_ScriptInvokerPlayable m_eOnPlayableRegistered = new PS_ScriptInvokerPlayable(); 
	PS_ScriptInvokerPlayable GetOnPlayableRegistered()
		return m_eOnPlayableRegistered;
	ref PS_ScriptInvokerPlayable m_eOnPlayableUnregistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableUnregistered()
		return m_eOnPlayableUnregistered;
	ref PS_ScriptInvokerPinChange m_eOnPlayerPinChange = new PS_ScriptInvokerPinChange();
	PS_ScriptInvokerPinChange GetOnPlayerPinChange()
		return m_eOnPlayerPinChange;
	ref PS_ScriptInvokerPlayerStateChange m_eOnPlayerStateChange = new PS_ScriptInvokerPlayerStateChange();
	PS_ScriptInvokerPlayerStateChange GetOnPlayerStateChange()
		return m_eOnPlayerStateChange;
	ref PS_ScriptInvokerPlayerPlayableChange m_eOnPlayerPlayableChange = new PS_ScriptInvokerPlayerPlayableChange();
	PS_ScriptInvokerPlayerPlayableChange GetOnPlayerPlayableChange()
		return m_eOnPlayerPlayableChange;
	ref PS_ScriptInvokerPlayableChangeGroup m_eOnPlayableChangeGroup = new PS_ScriptInvokerPlayableChangeGroup();
	PS_ScriptInvokerPlayableChangeGroup GetOnPlayableChangeGroup()
		return m_eOnPlayableChangeGroup;
	ref ScriptInvokerInt m_eOnStartTimerCounterChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnStartTimerCounterChanged()
		return m_eOnStartTimerCounterChanged;
	ref PS_ScriptInvokerFactionReadyChangeGroup m_eFactionReadyChanged = new PS_ScriptInvokerFactionReadyChangeGroup();
	PS_ScriptInvokerFactionReadyChangeGroup GetOnFactionReadyChanged()
		return m_eFactionReadyChanged;
	
	[RplProp()]
	int m_iMaxPlayersCount = 1;
	
	[RplProp(onRplName: "OnStartTimerCounterChanged")]
	int m_iStartTimerCounter = -1;
	
	bool m_bFactionsReadySended;
	
	void PS_PlayableManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.FRAME);
	}
	int m_bUpdated;
	bool isUpdated()
	{
		return m_bUpdated == 1;
	}
	void SetUpdated()
	{
		m_bUpdated = 2;
	}
	override void EOnFrame(IEntity owner, float timeSlice) //!EntityEvent.FRAME
	{
		m_bUpdated--;
	}
	
	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		if (Replication.IsServer()) m_bRplLoaded = true;
		
		if (RplSession.Mode() == RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(ForceGetSessionPlyersCount, 100, true);
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.GetOnPlayerConnected().Insert(OnPlayerConnected);
		gameModeCoop.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		gameModeCoop.GetOnPlayerRoleChange().Insert(OnPlayerRoleChange);
	}
	
	void ForceGetSessionPlyersCount()
	{
		DSSession dSSession = GetGame().GetBackendApi().GetDSSession();
		if (dSSession)
		{
			m_iMaxPlayersCount = dSSession.PlayerLimit();
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(ForceGetSessionPlyersCount);
		}
	}
	
	int GetMaxPlayers()
	{
		return m_iMaxPlayersCount;
	}
	
	// Events
	void OnPlayerConnected(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableComponent = GetPlayableById(playableId);
		if (playableComponent)
			playableComponent.GetOnPlayerConnected().Invoke(playerId);
	}
	
	void OnPlayerDisconnected(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		Rpc(RPC_OnPlayerDisconnected, playerId, cause, timeout);
		RPC_OnPlayerDisconnected(playerId, cause, timeout);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableComponent = GetPlayableById(playableId);
		if (playableComponent)
			playableComponent.GetOnPlayerDisconnected().Invoke(playerId);
		m_eOnPlayerDisconnected.Invoke(playerId, cause, timeout);
	}
	
	void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableComponent = GetPlayableById(playableId);
		if (playableComponent)
			playableComponent.GetOnPlayerRoleChange().Invoke(playerId, roleFlags);
	}
	
	void OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		Rpc(RPC_OnPlayableDamageStateChanged, playableId, damageState);
		RPC_OnPlayableDamageStateChanged(playableId, damageState);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		if (!m_aPlayables.Contains(playableId)) return;
		m_aPlayables[playableId].OnDamageStateChanged(damageState);
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_PlayableManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_PlayableManager.Cast(gameMode.FindComponent(PS_PlayableManager));
		else
			return null;
	}
	
	// Get control on selected playable entity
	// Executed only on server
	void ApplyPlayable(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		
		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);
		
		RplId playableId = GetPlayableByPlayer(playerId);	
		if (playableId != RplId.Invalid())
		{
			IEntity character = GetPlayableById(playableId).GetPlayableComponent().GetOwner();
			if (character)
			{
				SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(character);
				if (damageManager)
				{
					if (damageManager.GetState() == EDamageState.DESTROYED)
					{
						GetGame().GetCallqueue().CallLater(WrapSwitch, 1000, false, playerId);
					}
				}
			}
		}
		
		IEntity entity;
		if (playableId == RplId.Invalid() || playableId == -1) {
			// Remove group
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup) currentGroup.RemovePlayer(playerId);
			SetPlayerFactionKey(playerId, "");
			
			entity = playableController.GetInitialEntity();
			if (!entity)
			{
				Resource resource = Resource.Load("{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et");
				EntitySpawnParams params = new EntitySpawnParams();
				entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
				playableController.SetInitialEntity(entity);
			}
			
			playerController.SetInitialMainEntity(entity);
			
			return;
		} else entity = GetPlayableById(playableId).GetPlayableComponent().GetOwner();		 
		
		
		// Delete VoN
		IEntity vonEntity = playableController.GetInitialEntity();
		if (vonEntity) GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, vonEntity);
	
		playerController.SetInitialMainEntity(entity);
		
		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		SetPlayerFactionKey(playerId, faction.GetFactionKey());
		
		GetGame().GetCallqueue().CallLater(ChangeGroup, 0, false, playerId, playableId);
	}
	
	void WrapSwitch(int playerId)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.SwitchToInitialEntity(playerId);
	}
	
	// Holster weapon on all playables
	void HolsterWeapons()
	{
		if (!Replication.IsServer())
			return;
		
		foreach (RplId id, PS_PlayableContainer playable: m_aPlayables)
		{
			playable.GetPlayableComponent().HolsterWeapon();
		}
	}
	
	void RegisterGroupVehicle(RplId rplId, SCR_AIGroup group, Vehicle vehicle)
	{
		if (!Replication.IsServer())
			return;
		if (!group.m_PlayersGroup)
		{
			GetGame().GetCallqueue().Call(RegisterGroupVehicle, rplId, group, vehicle);
			return;
		}
		int groupCallsign = group.GetCallsignNum();
		PS_PlayableVehicleContainer playableVehicleContainer = new PS_PlayableVehicleContainer();
		playableVehicleContainer.Init(rplId, vehicle.GetPrefabData().GetPrefabName(), groupCallsign, group.m_PlayersGroup.GetGroupID(), vehicle.GetFactionAffiliation().GetDefaultFactionKey());
		Rpc(RPC_RegisterGroupVehicle, playableVehicleContainer);
		RPC_RegisterGroupVehicle(playableVehicleContainer);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_RegisterGroupVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		m_mPlayableVehicles[playableVehicleContainer.m_iRplId] = playableVehicleContainer;
	}
	void UnRegisterGroupVehicle(RplId rplId)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RPC_UnRegisterGroupVehicle, rplId);
		RPC_UnRegisterGroupVehicle(rplId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_UnRegisterGroupVehicle(RplId rplId)
	{
		m_mPlayableVehicles.Remove(rplId);
	}
	
	void NotifyKick(int playerId)
	{
		RPC_NotifyKick(playerId);
		Rpc(RPC_NotifyKick, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_NotifyKick(int playerId)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController && playerId == playerController.GetPlayerId())
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("lmsg");
			invoker.Invoke(null, "#PS-Lobby_RoleKick");
		}
	}
	
	// Force ApplyPlayable through menu switch
	void ForceSwitch(int playerId)
	{
		RPC_ForceSwitch(playerId);
		Rpc(RPC_ForceSwitch, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_ForceSwitch(int playerId)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController || playerController.GetPlayerId() != playerId) return;
		
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.GAME);
	}
	
	void ChangeGroup(int playerId, RplId playableId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		SCR_AIGroup playerGroup = GetPlayerGroupByPlayable(playableId);
		IEntity leaderEntity = null;
		if (playerGroup)
			leaderEntity = playerGroup.GetLeaderEntity();
		PS_PlayableContainer playableComponentLeader; 
		if (leaderEntity) playableComponentLeader = PS_PlayableContainer.Cast(leaderEntity.FindComponent(PS_PlayableContainer));
		
		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		if (playerGroup)
			playerControllerGroupComponent.PS_AskJoinGroup(playerGroup.GetGroupID());
		
		// Another workaround
		// Thanks bohem, no one zoomer will be harmed if you remove all text from game.
		// Maybe also multiplayer from arma? It can harm people, very dangerous.
		if (playerGroup && playerGroup.GetNameAuthorID() == -1)
		{
			playerGroup.SetCustomName(playerGroup.GetCustomName(), playerId);
		}
		if (playableComponentLeader)
			if (playableComponentLeader.GetRplId() > playableId) 
				groupsManagerComponent.SetGroupLeader(playerGroup.GetGroupID(), playerId);
	}
	
	void KillRedundantUnits()
	{
		map<RplId, ref PS_PlayableContainer> playables = GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableContainer playable = playables.GetElement(i);
			if (GetPlayerByPlayable(playable.GetRplId()) <= 0)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetPlayableComponent().GetOwner());
				SCR_EntityHelper.DeleteEntityAndChildren(character);
			}
		}
	}
	
	// -------------------------- Get ----------------------------
	PS_PlayableContainer GetPlayableById(RplId PlayableId)
	{
		PS_PlayableContainer playableComponent;
		m_aPlayables.Find(PlayableId, playableComponent);
		return playableComponent;
	}
	ref map<RplId, ref PS_PlayableContainer> GetPlayables()
	{
		return m_aPlayables;
	}
	void UpdatePlayablesSortedWrap() // sort playables by RplId AND CallSign
	{
		GetGame().GetCallqueue().Call(UpdatePlayablesSorted);
	}
	void UpdatePlayablesSorted() // sort playables by RplId AND CallSign
	{
		array<PS_PlayableContainer> playablesSorted = new array<PS_PlayableContainer>();
		
		map<RplId, ref PS_PlayableContainer> playables = GetPlayables();
		
		foreach (RplId playableId, PS_PlayableContainer playable : playables) {
			if (!playable) continue;
			int callSign = GetGroupCallsignByPlayable(playable.GetRplId());
			bool isInserted = false;
			for (int s = 0; s < playablesSorted.Count(); s++) {
				PS_PlayableContainer playableS = playablesSorted[s];
				int callSignS = GetGroupCallsignByPlayable(playableS.GetRplId());
				
				bool rplIdGreater = playableS.GetRplId() > playable.GetRplId();
				bool rankEquival = playable.GetCharacterRank() == playableS.GetCharacterRank();
				bool rankGreater = playable.GetCharacterRank() >  playableS.GetCharacterRank();
				bool callSignEquival = callSignS == callSign;
				bool callSignGreater = callSignS > callSign;
				
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
		
		m_PlayablesSorted = playablesSorted;
		Replication.BumpMe();
	}
	ref array<PS_PlayableContainer> GetPlayablesSorted() // sort playables by RplId AND CallSign
	{
		return m_PlayablesSorted;
	}
	ref map<RplId, ref PS_PlayableVehicleContainer> GetPlayableVehicles()
	{
		return m_mPlayableVehicles;
	}
	FactionKey GetPlayerFactionKeyRemembered(int playerId)
	{
		if (!m_playersFactionRemembered.Contains(playerId)) return "";
		return m_playersFactionRemembered[playerId];
	}
	FactionKey GetPlayerFactionKey(int playerId)
	{
		if (!m_playersFaction.Contains(playerId)) return "";
		return m_playersFaction[playerId];
	}
	PS_EPlayableControllerState GetPlayerState(int playerId)
	{
		PS_EPlayableControllerState state = PS_EPlayableControllerState.NotReady;
		m_playersStates.Find(playerId, state);
		return state;
	}
	string GetPlayerName(int playerId)
	{
		if (!m_playersLastName.Contains(playerId)) return "";
		return m_playersLastName[playerId];
	}
	
	int GetFactionReady(FactionKey factionKey)
	{
		return m_mFactionReady[factionKey];
	}
	
	string GetPlayablePrefab(RplId playableId)
	{
		if (!m_mPlayablePrefabs.Contains(playableId)) return "";
		return m_mPlayablePrefabs[playableId];
	}
	
	string GetPlayableName(RplId playableId)
	{
		if (!m_mPlayableNames.Contains(playableId)) return "";
		return m_mPlayableNames[playableId];
	}
	
	RplId GetPlayableByPlayer(int playerId)
	{
		if (!m_playersPlayable.Contains(playerId)) return RplId.Invalid();
		return m_playersPlayable[playerId];
	}
	
	RplId GetPlayableByPlayerRemembered(int playerId)
	{
		if (!m_playablePlayersRemembered.Contains(playerId)) return RplId.Invalid();
		return m_playablePlayersRemembered[playerId];
	}
	
	int GetPlayerByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayers.Contains(PlayableId)) return -1;
		return m_playablePlayers[PlayableId];
	}
	
	int GetPlayerByPlayableRemembered(RplId PlayableId)
	{
		if (!m_playersPlayableRemembered.Contains(PlayableId)) return -1;
		return m_playersPlayableRemembered[PlayableId];
	}
	
	SCR_AIGroup GetPlayerGroupByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayerGroupId.Contains(PlayableId)) 
			return null;
		
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(m_playablePlayerGroupId[PlayableId]);
	}
	
	SCR_AIGroup GetPlayerGroupByVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(playableVehicleContainer.m_iGroupId);
	}
	
	int GetGroupCallsignByPlayable(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group) return -1;
		
		return group.GetCallsignNum();
	}
	
	bool GetPlayerPin(int playerId)
	{
		if (!m_playersPin.Contains(playerId)) return false;
		return m_playersPin[playerId];
	}
	
	// -------------------------- Set server ----------------------------
	// Execute on server
	void RegisterPlayable(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetRplId();
		if (m_aPlayables.Contains(playableId))
			return;
		PS_PlayableContainer container = playableComponent.GetPlayableContainer();
		RPC_RegisterPlayable(container);
		Rpc(RPC_RegisterPlayable, container);
		SetPlayablePrefab(playableId, playableComponent.GetOwner().GetPrefabData().GetPrefabName());
		SetPlayableName(playableId, playableComponent.GetName());
		
		if (Replication.IsServer())
		{
			SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			AIControlComponent aiControl = AIControlComponent.Cast(playableCharacter.FindComponent(AIControlComponent));
			SCR_AIGroup playableGroup =  SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
			SCR_AIGroup playerGroup;
			
			if (!playableGroup)
				return;
			
			if (!playableGroup.m_PlayersGroup)
			{
				SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
				playerGroup = groupsManagerComponent.CreateNewPlayableGroup(playableGroup.GetFaction());
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
			SetPlayablePlayerGroupId(playableId, playerGroup.GetGroupID());
			GetGame().GetCallqueue().CallLater(UpdateGroupCallsigne, 0, false, playableId, playerGroup, playableGroup)
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_RegisterPlayable(PS_PlayableContainer container)
	{
		m_aPlayables[container.GetRplId()] = container;
		GetGame().GetCallqueue().Call(OnPlayableRegisteredLateInvoke, container.GetRplId(), container);
		GetGame().GetCallqueue().Remove(UpdatePlayablesSortedWrap);
		GetGame().GetCallqueue().Remove(UpdatePlayablesSorted);
		GetGame().GetCallqueue().Call(UpdatePlayablesSortedWrap);
	}
	void OnPlayableRegisteredLateInvoke(RplId playableId, PS_PlayableContainer playableComponent)
	{
		GetGame().GetCallqueue().Call(OnPlayableRegisteredLateInvoke2, playableId, playableComponent);
	}
	void OnPlayableRegisteredLateInvoke2(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_eOnPlayableRegistered.Invoke(playableId, playableComponent);
	}
	void RemovePlayable(RplId playableId)
	{
		RPC_RemovePlayable(playableId);
		Rpc(RPC_RemovePlayable, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_RemovePlayable(RplId playableId)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		PS_PlayableContainer playableContainer = m_aPlayables[playableId];
		m_aPlayables.Remove(playableId);
		
		UpdatePlayablesSorted();
		m_eOnPlayableUnregistered.Invoke(playableId, playableContainer);
		playableContainer.m_eOnUnregister.Invoke();
	}
	void UpdateGroupCallsigne(RplId playableId, SCR_AIGroup playerGroup, SCR_AIGroup playableGroup)
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
		
		GetGame().GetCallqueue().CallLater(RegisterGroupName, 0, false, playableId, playerGroup)
	}
	
	void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		// save callsign name for clients
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		int company, platoon, squad;
		callsignComponent.GetCallsignIndexes(company, platoon, squad);
		int groupCallsign = 1000000 * company + 1000 * platoon + 1 * squad;
		
		// Create VoN group channel
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), groupCallsign.ToString());
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Faction");
		
		SetUpdated();
	}
	
	
	// -------------------------- Set broadcast ----------------------------
	// For modify from client side use PS_PlayableControllerComponent insted
	
	// ------ PlayerFactionKey ------
	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		RPC_SetPlayerFactionKey(playerId, factionKey);
		Rpc(RPC_SetPlayerFactionKey, playerId, factionKey);
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		if (!playerController)
			return;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		FactionKey factionKeyOld = GetPlayerFactionKey(playerId);
		m_playersFaction[playerId] = factionKey;
		if (factionKey != "")
			m_playersFactionRemembered[playerId] = factionKey;
		
		m_eOnFactionChange.Invoke(playerId, factionKey, factionKeyOld);
		
		SetUpdated();
	}
	
	// ------ FactionReady ------
	void SetFactionReady(FactionKey factionKey, int readyValue)
	{
		RPC_SetFactionReady(factionKey, readyValue);
		Rpc(RPC_SetFactionReady, factionKey, readyValue);
		
		if (m_bFactionsReadySended)
			return;
		
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		bool allFactionsReady = true;
		foreach (int playerId : players)
		{
			factionKey = GetPlayerFactionKey(playerId);
			if (factionKey == "")
				continue;
			if (m_mFactionReady[factionKey])
				continue;
			allFactionsReady = false;
			break;
		}
		if (allFactionsReady)
		{
			m_bFactionsReadySended = true;
			
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("tmsg");
			invoker.Invoke(null, "Factions ready");
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFactionReady(FactionKey factionKey, int readyValue)
	{
		m_mFactionReady[factionKey] = readyValue;
		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
	}
	
	// ------ PlayablePlayer ------
	void SetPlayablePlayer(RplId playableId, int playerId)
	{
		RPC_SetPlayablePlayer(playableId, playerId);
		Rpc(RPC_SetPlayablePlayer, playableId, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayablePlayer(RplId playableId, int playerId)
	{
		if (playerId > 0) {
			RplId oldPlayable = GetPlayableByPlayer(playerId);
			if (oldPlayable != RplId.Invalid()) 
			{
				m_playablePlayers[oldPlayable] = -1;
				//if (playableId != RplId.Invalid())
				//	m_playersPlayableRemembered[playableId] = -1;
			}
			
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(-1);
		}
			
		m_playersPlayable[playerId] = playableId;
		m_playablePlayers[playableId] = playerId;
		
		if (playableId != RplId.Invalid()) {
			m_playablePlayersRemembered[playerId] = playableId;
		}
		
		if (playerId > 0)
		{
			m_playersPlayableRemembered[playableId] = playerId;
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
		}
		
		PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
		if (playableComponent)
			playableComponent.InvokeOnPlayerChanged(playerId);
		
		SetUpdated();
	}
	
	// ------ PlayerPlayable ------
	void SetPlayerPlayable(int playerId, RplId playableId)
	{
		RPC_SetPlayerPlayable(playerId, playableId);
		Rpc(RPC_SetPlayerPlayable, playerId, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerPlayable(int playerId, RplId playableId)
	{		
		RplId oldPlayable = GetPlayableByPlayer(playerId);
		if (oldPlayable != RplId.Invalid()) {
			m_playablePlayers[oldPlayable] = -1;
			//if (playableId != RplId.Invalid())
			//	m_playersPlayableRemembered[playableId] = -1;
			
			PS_PlayableContainer playableComponent = m_aPlayables.Get(oldPlayable);
			if (playableComponent)
				playableComponent.InvokeOnPlayerChanged(-1);
		}
		
		m_playersPlayable[playerId] = playableId;
		m_playablePlayers[playableId] = playerId;
		
		if (playableId != RplId.Invalid()) {
			m_playablePlayersRemembered[playerId] = playableId;
		}
		
		if (playerId > 0)
		{
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
			m_playersPlayableRemembered[playableId] = playerId;
		}
		
		PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
		if (playableComponent)
			playableComponent.InvokeOnPlayerChanged(playerId);
		
		SetUpdated();
	}
	
	void StartTime()
	{
		m_iStartTimerCounter -= 1;
		Replication.BumpMe();
		OnStartTimerCounterChanged();
		if (m_iStartTimerCounter == 0)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gameModeCoop.AdvanceGameState(SCR_EGameModeState.SLOTSELECTION);
			GetGame().GetCallqueue().Remove(StartTime);
		}
	}
	
	void OnStartTimerCounterChanged()
	{
		m_eOnStartTimerCounterChanged.Invoke(m_iStartTimerCounter);
	}
	
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		RPC_SetPlayerState(playerId, state);
		Rpc(RPC_SetPlayerState, playerId, state);
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameModeCoop.GetState();
		if (gameModeState == SCR_EGameModeState.SLOTSELECTION)
		{
			GetGame().GetCallqueue().Remove(StartTime);
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
				GetGame().GetCallqueue().CallLater(StartTime, 1000, true);
			}
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		m_playersStates[playerId] = state;
		
		m_eOnPlayerStateChange.Invoke(playerId, state);
		
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerStateChange().Invoke(state);
		}
		
		SetUpdated();
	}
	
	void SetPlayerPin(int playerId, bool pined)
	{
		RPC_SetPlayerPin(playerId, pined);
		Rpc(RPC_SetPlayerPin, playerId, pined);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerPin(int playerId, bool pined)
	{
		//Print("RPC_SetPlayerPin: " + playerId.ToString() + " - " + pined.ToString());
		m_playersPin[playerId] = pined;
		
		m_eOnPlayerPinChange.Invoke(playerId, pined);
		
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerPinChange().Invoke(pined);
		}
			
		
		SetUpdated();
	}
	
	void SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		RPC_SetPlayablePlayerGroupId(PlayableId, groupId);
		Rpc(RPC_SetPlayablePlayerGroupId, PlayableId, groupId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		m_playablePlayerGroupId[PlayableId] = groupId;
		
		UpdatePlayablesSorted();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_eOnPlayableChangeGroup.Invoke(PlayableId, GetPlayableById(PlayableId), groupsManagerComponent.FindGroup(groupId));
		SetUpdated();
	}
	
	void SetPlayerName(int playerId, string playerName)
	{
		RPC_SetPlayerName(playerId, playerName);
		Rpc(RPC_SetPlayerName, playerId, playerName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerName(int playerId, string playerName)
	{
		m_playersLastName[playerId] = playerName;
		m_eOnPlayerConnected.Invoke(playerId);
		
		SetUpdated();
	}
	
	void SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		Rpc(RPC_SetPlayablePrefab, playableId, prefab);
		RPC_SetPlayablePrefab(playableId, prefab);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		m_mPlayablePrefabs[playableId] = prefab;
	}
	
	void SetPlayableName(RplId playableId, string name)
	{
		Rpc(RPC_SetPlayableName, playableId, name);
		RPC_SetPlayableName(playableId, name);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayableName(RplId playableId, string name)
	{
		m_mPlayableNames[playableId] = name;
	}
	
	// -------------------------- Util ----------------------------
	bool IsPlayerGroupLeader(int thisPlayerId)
	{
		if (thisPlayerId == -1) return false;
		
		RplId thisPlayableId = GetPlayableByPlayer(thisPlayerId);
		if (thisPlayableId == RplId.Invalid()) return false;
		
		int thisGroupCallsign = GetGroupCallsignByPlayable(thisPlayableId);
		
		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable: playables)
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
	
	// Use only in UI
	string GroupCallsignToGroupName(SCR_Faction faction, int groupCallSign)
	{
		int companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex;
		companyCallsignIndex = groupCallSign / 1000000;
		platoonCallsignIndex = (groupCallSign % 1000000) / 1000;
		squadCallsignIndex = (groupCallSign % 1000) / 1;
		
		if (!faction)
			return "";
		
		SCR_FactionCallsignInfo callsignInfo = faction.GetCallsignInfo();
		string company = callsignInfo.GetCompanyCallsignName(companyCallsignIndex);
		string platoon = callsignInfo.GetPlatoonCallsignName(platoonCallsignIndex);
		string squad   = callsignInfo.GetSquadCallsignName(squadCallsignIndex);
		string format  = callsignInfo.GetCallsignFormat(false);	
		
		return WidgetManager.Translate(format, company, platoon, squad, "");
	}
	
	// Send our precision data, we need it on clients
	override bool RplSave(ScriptBitWriter writer)
	{
		// I want template functions, this seems ugly :(
		
		int playersStatesCount = m_playersStates.Count();
		writer.WriteInt(playersStatesCount);
		for (int i = 0; i < playersStatesCount; i++)
		{
			writer.WriteInt(m_playersStates.GetKey(i));
			writer.WriteInt(m_playersStates.GetElement(i));
		}
		
		int playersPlayableCount = m_playersPlayable.Count();
		writer.WriteInt(playersPlayableCount);
		for (int i = 0; i < playersPlayableCount; i++)
		{
			writer.WriteInt(m_playersPlayable.GetKey(i));
			writer.WriteInt(m_playersPlayable.GetElement(i));
		}
		
		int playablePlayersCount = m_playablePlayers.Count();
		writer.WriteInt(playablePlayersCount);
		for (int i = 0; i < playablePlayersCount; i++)
		{
			writer.WriteInt(m_playablePlayers.GetKey(i));
			writer.WriteInt(m_playablePlayers.GetElement(i));
		}
		
		int playersPinCount = m_playersPin.Count();
		writer.WriteInt(playersPinCount);
		for (int i = 0; i < playersPinCount; i++)
		{
			writer.WriteInt(m_playersPin.GetKey(i));
			writer.WriteBool(m_playersPin.GetElement(i));
		}
		
		int playersFactionCount = m_playersFaction.Count();
		writer.WriteInt(playersFactionCount);
		for (int i = 0; i < playersFactionCount; i++)
		{
			writer.WriteInt(m_playersFaction.GetKey(i));
			writer.WriteString(m_playersFaction.GetElement(i));
		}
		
		int playersFactionRememberedCount = m_playersFactionRemembered.Count();
		writer.WriteInt(playersFactionRememberedCount);
		for (int i = 0; i < playersFactionRememberedCount; i++)
		{
			writer.WriteInt(m_playersFactionRemembered.GetKey(i));
			writer.WriteString(m_playersFactionRemembered.GetElement(i));
		}
		
		int playablePlayerGroupIdCount = m_playablePlayerGroupId.Count();
		writer.WriteInt(playablePlayerGroupIdCount);
		for (int i = 0; i < playablePlayerGroupIdCount; i++)
		{
			writer.WriteInt(m_playablePlayerGroupId.GetKey(i));
			writer.WriteInt(m_playablePlayerGroupId.GetElement(i));
		}
		
		int plyableLastPlayerNameCount = m_playersLastName.Count();
		writer.WriteInt(plyableLastPlayerNameCount);
		for (int i = 0; i < plyableLastPlayerNameCount; i++)
		{
			writer.WriteInt(m_playersLastName.GetKey(i));
			writer.WriteString(m_playersLastName.GetElement(i));
		}
		
		int playersPlayableRememberedCount = m_playersPlayableRemembered.Count();
		writer.WriteInt(playersPlayableRememberedCount);
		for (int i = 0; i < playersPlayableRememberedCount; i++)
		{
			writer.WriteInt(m_playersPlayableRemembered.GetKey(i));
			writer.WriteInt(m_playersPlayableRemembered.GetElement(i));
		}
		
		int playablePlayersRememberedCount = m_playablePlayersRemembered.Count();
		writer.WriteInt(playablePlayersRememberedCount);
		for (int i = 0; i < playablePlayersRememberedCount; i++)
		{
			writer.WriteInt(m_playablePlayersRemembered.GetKey(i));
			writer.WriteInt(m_playablePlayersRemembered.GetElement(i));
		}
		
		int factionReadyCount = m_mFactionReady.Count();
		writer.WriteInt(factionReadyCount);
		for (int i = 0; i < factionReadyCount; i++)
		{
			writer.WriteString(m_mFactionReady.GetKey(i));
			writer.WriteInt(m_mFactionReady.GetElement(i));
		}
		
		int playablePrefabsCount = m_mPlayablePrefabs.Count();
		writer.WriteInt(playablePrefabsCount);
		for (int i = 0; i < playablePrefabsCount; i++)
		{
			writer.WriteInt(m_mPlayablePrefabs.GetKey(i));
			writer.WriteString(m_mPlayablePrefabs.GetElement(i));
		}
		
		int playableNamesCount = m_mPlayableNames.Count();
		writer.WriteInt(playablePrefabsCount);
		for (int i = 0; i < playablePrefabsCount; i++)
		{
			writer.WriteInt(m_mPlayableNames.GetKey(i));
			writer.WriteString(m_mPlayableNames.GetElement(i));
		}
		
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
	
	override bool RplLoad(ScriptBitReader reader)
	{
		int playersStatesCount;
		reader.ReadInt(playersStatesCount);
		for (int i = 0; i < playersStatesCount; i++)
		{
			int key;
			PS_EPlayableControllerState value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playersStates.Insert(key, value);
		}
		
		int playersPlayableCount;
		reader.ReadInt(playersPlayableCount);
		for (int i = 0; i < playersPlayableCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playersPlayable.Insert(key, value);
		}
		
		int playablePlayersCount;
		reader.ReadInt(playablePlayersCount);
		for (int i = 0; i < playablePlayersCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playablePlayers.Insert(key, value);
		}
		
		int playersPinCount;
		reader.ReadInt(playersPinCount);
		for (int i = 0; i < playersPinCount; i++)
		{
			int key;
			bool value;
			reader.ReadInt(key);
			reader.ReadBool(value);
			
			m_playersPin.Insert(key, value);
		}
		
		int playersFactionCount;
		reader.ReadInt(playersFactionCount);
		for (int i = 0; i < playersFactionCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_playersFaction.Insert(key, value);
		}
		
		int playersFactionRememberedCount;
		reader.ReadInt(playersFactionRememberedCount);
		for (int i = 0; i < playersFactionRememberedCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_playersFactionRemembered.Insert(key, value);
		}
		
		int playablePlayerGroupIdCount;
		reader.ReadInt(playablePlayerGroupIdCount);
		for (int i = 0; i < playablePlayerGroupIdCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playablePlayerGroupId.Insert(key, value);
		}
		
		int plyableLastPlayerNameCount;
		reader.ReadInt(plyableLastPlayerNameCount);
		for (int i = 0; i < plyableLastPlayerNameCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_playersLastName.Insert(key, value);
		}
		
		int playersPlayableRememberedCount;
		reader.ReadInt(playersPlayableRememberedCount);
		for (int i = 0; i < playersPlayableRememberedCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playersPlayableRemembered.Insert(key, value);
		}
		
		int playablePlayersRememberedCount;
		reader.ReadInt(playablePlayersRememberedCount);
		for (int i = 0; i < playablePlayersRememberedCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playablePlayersRemembered.Insert(key, value);
		}
		
		int factionReadyCount;
		reader.ReadInt(factionReadyCount);
		for (int i = 0; i < factionReadyCount; i++)
		{
			FactionKey key;
			int value;
			reader.ReadString(key);
			reader.ReadInt(value);
			
			m_mFactionReady.Insert(key, value);
		}
		
		int playablePrefabsCount;
		reader.ReadInt(playablePrefabsCount);
		for (int i = 0; i < playablePrefabsCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_mPlayablePrefabs.Insert(key, value);
		}
		
		int playableNamesCount;
		reader.ReadInt(playableNamesCount);
		for (int i = 0; i < playableNamesCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_mPlayableNames.Insert(key, value);
		}
		
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
};