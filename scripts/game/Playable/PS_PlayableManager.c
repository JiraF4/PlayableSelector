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
		
	// Clients also don't give a shit about groups, soo we save staff here
	// TODO: move to playable 
	ref map<RplId, string> m_playableGroups = new map<RplId, string>;
		
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
		
		RplId playableId = GetPlayableByPlayer(playerId);
		IEntity entity;
		if (playableId == RplId.Invalid() || playableId == -1) {
			entity = playableController.GetInitialEntity();
			playerController.SetInitialMainEntity(entity);
			return;
		} else entity = GetPlayableById(playableId).GetOwner();		 
		
		playerController.SetInitialMainEntity(entity);
		
		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		playerFactionAffiliation.SetAffiliatedFaction(faction);
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
		
		// Someone leave lobby start game
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME)
			gameMode.StartGameMode();
		
		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);
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
		m_aPlayables[playableComponent.GetId()] = playableComponent;
	}
	
	// -------------------------- Set broadcast ----------------------------
	// For modify from client side use PS_PlayableControllerComponent insted
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
		
		return true;
	}
};