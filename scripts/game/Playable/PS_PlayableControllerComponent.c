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
	// Force change game state
	void ForceGameStart()
	{
		Rpc(RPC_ForceGameStart)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ForceGameStart()
	{
		// only admins can force start
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME)
			gameMode.StartGameMode();
	}
	
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
	
	void UnpinPlayer(int playerId)
	{
		Rpc(RPC_UnpinPlayer, playerId)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_UnpinPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		playableManager.SetPlayerPin(playerId, false);
	}
	
	void KickPlayer(int playerId)
	{
		Rpc(RPC_KickPlayer, playerId)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_KickPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		playerManager.KickPlayer(playerId, PlayerManagerKickReason.KICK, 0);
	}
	
	// -------------------- Set ---------------------
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		Rpc(RPC_SetPlayerState, playerId, state)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (thisPlayerController.GetPlayerId() != playerId && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		playableManager.SetPlayerState(playerId, state);
	}
	
	void SetPlayerPlayable(int playerId, RplId playableId) 
	{
		Rpc(RPC_SetPlayerPlayable, playerId, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerPlayable(int playerId, RplId playableId) 
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (thisPlayerController.GetPlayerId() != playerId && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		// You can't change playable if pinned and not admin
		if (playableManager.GetPlayerPin(playerId) && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		// don't check other staff if empty playable
		if (playableId == RplId.Invalid()) {
			playableManager.SetPlayerPlayable(playerId, playableId);
			return;
		}
		
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
		
		// Check is playable already selected or dead
		int curretPlayerId = playableManager.GetPlayerByPlayable(playableId);
		if (playableCharacter.GetDamageManager().IsDestroyed() || (curretPlayerId != -1 && curretPlayerId != playerId)) {
			return;
		}
		
		playableManager.SetPlayerPlayable(playerId, playableId);
		
		// Pin player if setted by admin
		if (playerId != thisPlayerController.GetPlayerId()) playableManager.SetPlayerPin(playerId, true);
				
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
	
}