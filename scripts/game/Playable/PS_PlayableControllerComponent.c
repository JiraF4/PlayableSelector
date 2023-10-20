//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableControllerComponentClass: ScriptComponentClass
{
}


// We can send rpc only from authority
// And here we are, modifying player controller since it's only what we have on client.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableControllerComponent : ScriptComponent
{
	// Get controll on selected playable entity
	void ApplyPlayable()
	{
		Rpc(RPC_ApplyPlayable)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ApplyPlayable()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = PlayerController.Cast(GetOwner());
		playableManager.ApplyPlayable(playerController.GetPlayerId());
	}
	
	// -------------------- Set ---------------------
	void SetPlayerState(PS_EPlayableControllerState state)
	{
		Rpc(RPC_SetPlayerState, state)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerState(PS_EPlayableControllerState state)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = PlayerController.Cast(GetOwner());
		playableManager.SetPlayerState(playerController.GetPlayerId(), state);
	}
	
	void SetPlayerPlayable(int playableId) 
	{
		Rpc(RPC_SetPlayerPlayable, playableId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerPlayable(RplId playableId) 
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = PlayerController.Cast(GetOwner());
		int playerId = playerController.GetPlayerId();
		
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
		
		// Check is playable already selected or dead
		int curretPlayerId = playableManager.GetPlayerByPlayable(playableId);
		if (playableCharacter.GetDamageManager().IsDestroyed() || (curretPlayerId != -1 && curretPlayerId != playerId)) {
			return;
		}
		
		playableManager.SetPlayerPlayable(playerId, playableId);
		
		/*
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
		playerFactionAffiliation.SetAffiliatedFaction(faction);
		
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
		
		*/
		
		//Print("RPC_SetPlayerPlayable 2: " + playerId.ToString() + " - " + playableId);
		/*
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		AIControlComponent aiControl = AIControlComponent.Cast(playable.FindComponent(AIControlComponent));
		SCR_AIGroup groupPlayable =  SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
		SCR_AIGroup group = groupsManagerComponent.CreateNewPlayableGroup(faction);
		group.SetSlave(groupPlayable);
		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		playerControllerGroupComponent.RPC_AskJoinGroup(group.GetGroupID());
		
		group.SetGroupID(-2);
		group.SetMaxMembers(10);
		groupsManagerComponent.RegisterGroup(group);
		playerControllerGroupComponent.RPC_AskJoinGroup(group.GetGroupID());
		*/
	}
	
	
	// olds here
	void SetPlayerPlayableReconnect(int playerId, int playableId) 
	{
		Rpc(RPC_SetPlayerPlayableReconnect, playerId, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerPlayableReconnect(int playerId, int playableId) 
	{
		/*
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		SCR_ChimeraCharacter playable = SCR_ChimeraCharacter.Cast(playables[playableId].GetOwner());
		playerController.SetPossessedEntity(playable); // reset ai? but still broken...
		playerController.SetInitialMainEntity(playable);
		*/
	}
	
	
}