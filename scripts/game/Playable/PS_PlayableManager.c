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
	ref map<RplId, string> m_playableGroups = new map<RplId, string>;
	
	// Server only!
	ref map<SCR_AIGroup, SCR_AIGroup> m_playableToPlayerGroups = new map<SCR_AIGroup, SCR_AIGroup>; // Groups for players, not the same with ai
	ref map<RplId, SCR_AIGroup> m_playerGroups = new map<RplId, SCR_AIGroup>; // Groups for players, not the same with ai
	
	
		
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
		
		RplId playableId = GetPlayableByPlayer(playerId);
		IEntity entity;
		if (playableId == RplId.Invalid() || playableId == -1) {
			// Remove group
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup) currentGroup.RemovePlayer(playerId);
			
			entity = playableController.GetInitialEntity();
			playerController.SetInitialMainEntity(entity);
			return;
		} else entity = GetPlayableById(playableId).GetOwner();		 
		
		playerController.SetInitialMainEntity(entity);
		
		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		
		// Someone leave lobby start game
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME)
			gameMode.StartGameMode();
				
		GetGame().GetCallqueue().CallLater(ChangeGroup, 0, false, playerId, playableId);
		
		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);
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
	
	string GetGroupNameByPlayable(RplId PlayableId)
	{
		string groupName;
		m_playableGroups.Find(PlayableId, groupName);
		return groupName;
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
				groupsManagerComponent.RegisterGroup(playerGroup);
				m_playableToPlayerGroups[playableGroup] = playerGroup;
				
				playableGroup.SetCanDeleteIfNoPlayer(false);
				playerGroup.SetCanDeleteIfNoPlayer(false);
			} else {
				playerGroup = m_playableToPlayerGroups[playableGroup];
			}
			m_playerGroups[playableId] = playerGroup;
			GetGame().GetCallqueue().CallLater(RegisterGroupName, 0, false, playableId, playerGroup)
		}
	}
	
	void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		string company, platoon, squad, sCharacter, format;
		playerGroup.GetCallsigns(company, platoon, squad, sCharacter, format);
		string groupName = WidgetManager.Translate(format, company, platoon, squad, sCharacter);
		SetPlayableGroupName(playableId, groupName);
	}
	
	// -------------------------- Set broadcast ----------------------------
	// For modify from client side use PS_PlayableControllerComponent insted
	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		RPC_SetPlayerFactionKey(playerId, factionKey);
		Rpc(RPC_SetPlayerFactionKey, playerId, factionKey);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		m_playersFaction[playerId] = factionKey;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
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
	
	void SetPlayableGroupName(RplId PlayableId, string groupName)
	{
		RPC_SetPlayableGroupName(PlayableId, groupName);
		Rpc(RPC_SetPlayableGroupName, PlayableId, groupName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayableGroupName(RplId PlayableId, string groupName)
	{
		m_playableGroups[PlayableId] = groupName;
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
		
		int playableGroupsCount = m_playableGroups.Count();
		writer.WriteInt(playableGroupsCount);
		for (int i = 0; i < playableGroupsCount; i++)
		{
			writer.WriteInt(m_playableGroups.GetKey(i));
			writer.WriteString(m_playableGroups.GetElement(i));
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
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_playableGroups.Insert(key, value);
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
		
		return true;
	}
};