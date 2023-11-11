//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManager : ScriptComponent
{	
	// Map of our playables
	ref map<RplId, PS_PlayableComponent> m_aPlayables = new ref map<RplId, PS_PlayableComponent>(); // We don't sync it!
	
	// Maps for saving players staff, player controllers local to client
	ref map<int, PS_EPlayableControllerState> m_playersStates = new map<int, PS_EPlayableControllerState>;
	ref map<int, RplId> m_playersPlayable = new map<int, RplId>;
	ref map<RplId, int> m_playablePlayers = new map<RplId, int>; // reversed m_playersPlayable for fast search
	ref map<int, bool> m_playersPin = new map<int, bool>; // is player pined
	ref map<int, FactionKey> m_playersFaction = new map<int, FactionKey>; // player factions
		
	// Clients also don't give a shit about groups, soo we save staff here
	ref map<RplId, int> m_playableGroupCallSigns = new map<RplId, int>;
	
	// Server only!
	ref map<SCR_AIGroup, SCR_AIGroup> m_playableToPlayerGroups = new map<SCR_AIGroup, SCR_AIGroup>; // Groups for players, not the same with ai
	ref map<RplId, SCR_AIGroup> m_playerGroups = new map<RplId, SCR_AIGroup>; // Groups for players, not the same with ai
	
	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		if (Replication.IsServer()) m_bRplLoaded = true;
	}
	
	// TODO: fix it please
	void ResetRplStream()
	{
		GetGame().GetCallqueue().CallLater(ResetRplStreamWrap, 1000);
	}
	void ResetRplStreamWrap()
	{
		map<RplId, PS_PlayableComponent> playables = GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableComponent playable = playables.GetElement(i);
			playable.ResetRplStream();
		}
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
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		
		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);
		
		RplId playableId = GetPlayableByPlayer(playerId);
		IEntity entity;
		if (playableId == RplId.Invalid() || playableId == -1) {
			// Remove group
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup) currentGroup.RemovePlayer(playerId);
			SetPlayerFactionKey(playerId, "");
			
			entity = playableController.GetInitialEntity();
			playerController.SetInitialMainEntity(entity);
			
			return;
		} else entity = GetPlayableById(playableId).GetOwner();		 
		
		playerController.SetInitialMainEntity(entity);
		
		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		SetPlayerFactionKey(playerId, faction.GetFactionKey());
		
		GetGame().GetCallqueue().CallLater(ChangeGroup, 0, false, playerId, playableId);
	}
	
	void ChangeGroup(int playerId, RplId playableId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		SCR_AIGroup playerGroup = m_playerGroups[playableId];
		IEntity leaderEntity = playerGroup.GetLeaderEntity();
		PS_PlayableComponent playableComponentLeader; 
		if (leaderEntity) playableComponentLeader = PS_PlayableComponent.Cast(leaderEntity.FindComponent(PS_PlayableComponent));
		
		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		playerControllerGroupComponent.PS_AskJoinGroup(playerGroup.GetGroupID());
		if (playableComponentLeader)
			if (playableComponentLeader.GetId() > playableId) groupsManagerComponent.SetGroupLeader(playerGroup.GetGroupID(), playerId);
	}
	
	void KillRedundantUnits()
	{
		map<RplId, PS_PlayableComponent> playables = GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableComponent playable = playables.GetElement(i);
			if (GetPlayerByPlayable(playable.GetId()) <= 0)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
				SCR_EntityHelper.DeleteEntityAndChildren(character);
				/*
				character.ClearFlags(EntityFlags.VISIBLE, true);
				SCR_CharacterDamageManagerComponent damageComponent = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
				damageComponent.Kill();
				*/
			}
		}
	}
	
	// -------------------------- Get ----------------------------
	PS_PlayableComponent GetPlayableById(RplId PlayableId)
	{
		PS_PlayableComponent playableComponent;
		m_aPlayables.Find(PlayableId, playableComponent);
		return playableComponent;
	}
	ref map<RplId, PS_PlayableComponent> GetPlayables()
	{
		return m_aPlayables;
	}
	ref array<PS_PlayableComponent> GetPlayablesSorted() // sort playables by RplId AND CallSign
	{
		array<PS_PlayableComponent> playablesSorted = new array<PS_PlayableComponent>();
		map<RplId, PS_PlayableComponent> playables = GetPlayables();
		
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableComponent playable = playables.GetElement(i);
			if (!playable) continue;
			int callSign = GetGroupCallsignByPlayable(playable.GetId());
			bool isInserted = false;
			for (int s = 0; s < playablesSorted.Count(); s++) {
				PS_PlayableComponent playableS = playablesSorted[s];
				int callSignS = GetGroupCallsignByPlayable(playableS.GetId());
				if ((playableS.GetId() > playable.GetId() && callSign == callSignS) || callSign < callSignS) {
					playablesSorted.InsertAt(playable, s);
					isInserted = true;
					break;
				}
			}
			if (!isInserted) {
				playablesSorted.Insert(playable);
			}
		}
		return playablesSorted;
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
	
	RplId GetPlayableByPlayer(int playerId)
	{
		if (!m_playersPlayable.Contains(playerId)) return RplId.Invalid();
		return m_playersPlayable[playerId];
	}
	
	int GetPlayerByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayers.Contains(PlayableId)) return -1;
		return m_playablePlayers[PlayableId];
	}
	
	int GetGroupCallsignByPlayable(RplId PlayableId)
	{
		int groupCallsign;
		m_playableGroupCallSigns.Find(PlayableId, groupCallsign);
		return groupCallsign;
	}
	
	bool GetPlayerPin(int playerId)
	{
		if (!m_playersPin.Contains(playerId)) return false;
		return m_playersPin[playerId];
	}
	
	// -------------------------- Set local ----------------------------
	// Execute on every client and server
	void RegisterPlayable(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetId();
		m_aPlayables[playableId] = playableComponent;
		
		// Create player groups for join
		if (Replication.IsServer())
		{
			SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			AIControlComponent aiControl = AIControlComponent.Cast(playableCharacter.FindComponent(AIControlComponent));
			SCR_AIGroup playableGroup =  SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
			SCR_AIGroup playerGroup;
			if (!m_playableToPlayerGroups.Contains(playableGroup))
			{
				SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
				playerGroup = groupsManagerComponent.CreateNewPlayableGroup(playableGroup.GetFaction());
				playerGroup.SetSlave(playableGroup);
				playerGroup.SetMaxMembers(12);
				m_playableToPlayerGroups[playableGroup] = playerGroup;
								
				playableGroup.SetCanDeleteIfNoPlayer(false);
				playerGroup.SetCanDeleteIfNoPlayer(false);
			} else {
				playerGroup = m_playableToPlayerGroups[playableGroup];
			}
			m_playerGroups[playableId] = playerGroup;
			GetGame().GetCallqueue().CallLater(UpdateGroupCallsigne, 0, false, playableId, playerGroup, playableGroup)
		}
	}
	void RemovePlayable(RplId playableId)
	{
		m_aPlayables.Remove(playableId);
	}
	
	void UpdateGroupCallsigne(RplId playableId, SCR_AIGroup playerGroup, SCR_AIGroup playableGroup)
	{
		// Assign manualy set callsigns
		PS_GroupCallsignAssigner groupCallsignAssigner = PS_GroupCallsignAssigner.Cast(playableGroup.FindComponent(PS_GroupCallsignAssigner));
		if (groupCallsignAssigner) {
			int company, platoon, squad;
			groupCallsignAssigner.GetCallsign(company, platoon, squad);
			SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
			callsignComponent.ReAssignGroupCallsign(company, platoon, squad);
		}
		
		GetGame().GetCallqueue().CallLater(RegisterGroupName, 0, false, playableId, playerGroup)
	}
	
	void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		// save callsign name for clients
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		int company, platoon, squad;
		callsignComponent.GetCallsignIndexes(company, platoon, squad);
		//string company, platoon, squad, sCharacter, format;
		//playerGroup.GetCallsigns(company, platoon, squad, sCharacter, format);
		//string groupName = WidgetManager.Translate(format, company, platoon, squad, sCharacter);
		int groupCallsign = 1000000 * company + 1000 * platoon + 1 * squad;
		SetPlayableGroupCallSign(playableId, groupCallsign);
		
		// Create VoN group chanel
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), groupCallsign.ToString());
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(playerGroup.GetFaction().GetFactionKey(), "#PS-VoNRoom_Faction");
	}
	
	
	// -------------------------- Set broadcast ----------------------------
	// For modify from client side use PS_PlayableControllerComponent insted
	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		RPC_SetPlayerFactionKey(playerId, factionKey);
		Rpc(RPC_SetPlayerFactionKey, playerId, factionKey);
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		m_playersFaction[playerId] = factionKey;
	}
	
	
	void SetPlayablePlayer(RplId PlayableId, int playerId)
	{
		RPC_SetPlayablePlayer(PlayableId, playerId);
		Rpc(RPC_SetPlayablePlayer, PlayableId, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayablePlayer(RplId PlayableId, int playerId)
	{
		if (playerId > 0) {
			RplId oldPlayable = GetPlayableByPlayer(playerId);
			if (oldPlayable != RplId.Invalid()) m_playablePlayers[oldPlayable] = -1;
		}
			
		m_playersPlayable[playerId] = PlayableId;
		m_playablePlayers[PlayableId] = playerId;
	}
	
	
	void SetPlayerPlayable(int playerId, RplId PlayableId)
	{
		RPC_SetPlayerPlayable(playerId, PlayableId);
		Rpc(RPC_SetPlayerPlayable, playerId, PlayableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerPlayable(int playerId, RplId PlayableId)
	{
		RplId oldPlayable = GetPlayableByPlayer(playerId);
		if (oldPlayable != RplId.Invalid()) m_playablePlayers[oldPlayable] = -1;
		
		m_playersPlayable[playerId] = PlayableId;
		m_playablePlayers[PlayableId] = playerId;
	}
	
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		RPC_SetPlayerState(playerId, state);
		Rpc(RPC_SetPlayerState, playerId, state);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		m_playersStates[playerId] = state;
	}
	
	void SetPlayableGroupCallSign(RplId PlayableId, int groupCallsign)
	{
		RPC_SetPlayableGroupCallSign(PlayableId, groupCallsign);
		Rpc(RPC_SetPlayableGroupCallSign, PlayableId, groupCallsign);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayableGroupCallSign(RplId PlayableId, int groupCallsign)
	{
		m_playableGroupCallSigns[PlayableId] = groupCallsign;
	}
	
	void SetPlayerPin(int playerId, bool pined)
	{
		RPC_SetPlayerPin(playerId, pined);
		Rpc(RPC_SetPlayerPin, playerId, pined);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerPin(int playerId, bool pined)
	{
		m_playersPin[playerId] = pined;
	}
	
	// -------------------------- Util ----------------------------
	bool IsPlayerGroupLeader(int thisPlayerId)
	{
		if (thisPlayerId == -1) return false;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		RplId thisPlayableId = GetPlayableByPlayer(thisPlayerId);
		if (thisPlayableId == RplId.Invalid()) return false;
		
		int thisGroupCallsign = GetGroupCallsignByPlayable(thisPlayableId);
		
		array<int> playerIds = new array<int>();
		playerManager.GetPlayers(playerIds); 
		foreach (int playerId: playerIds)
		{
			if (playerId == thisPlayerId) continue;
			if (GetPlayerFactionKey(playerId) != GetPlayerFactionKey(thisPlayerId)) continue;
			RplId playableId = GetPlayableByPlayer(playerId);
			if (playableId == RplId.Invalid()) continue;
			if (playableId > thisPlayableId) continue;
			int groupCallsign = GetGroupCallsignByPlayable(playableId);
			if (thisGroupCallsign != groupCallsign) continue;
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
		
		int playableGroupsCount = m_playableGroupCallSigns.Count();
		writer.WriteInt(playableGroupsCount);
		for (int i = 0; i < playableGroupsCount; i++)
		{
			writer.WriteInt(m_playableGroupCallSigns.GetKey(i));
			writer.WriteInt(m_playableGroupCallSigns.GetElement(i));
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
		
		int playableGroupsCount;
		reader.ReadInt(playableGroupsCount);
		for (int i = 0; i < playableGroupsCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_playableGroupCallSigns.Insert(key, value);
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
		
		m_bRplLoaded = true;
		
		return true;
	}
};