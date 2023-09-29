//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableControllerComponentClass: ScriptComponentClass
{
}

enum PlayableControllerState
{
	NotReady,
	Ready,
	Playing
}

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableControllerComponent : ScriptComponent
{
	PlayableControllerState m_bState;
	
	void SetState(PlayableControllerState state)
	{
		Rpc_SetStateServer(SCR_PlayerController.Cast(GetOwner()).GetPlayerId(), state);
		Rpc(Rpc_SetStateServer, SCR_PlayerController.Cast(GetOwner()).GetPlayerId(), state);
	}
	
	void SetState(PlayableControllerState state, int playerId)
	{
		Rpc_SetStateServer(playerId, state);
		Rpc(Rpc_SetStateServer, playerId, state);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void Rpc_SetStateServer(int playerId, PlayableControllerState state)
	{
		SCR_GameModeCoop.Cast(GetGame().GetGameMode()).SetPlayerState(playerId, state);
	}
	
	static PlayableControllerState GetState(int playerId)
	{
		return SCR_GameModeCoop.GetPlayerState(playerId);
	}
	
	void TakePossession(int playerId, int playableId) 
	{
		Rpc(RPC_TakePossession, playerId, playableId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_TakePossession(int playerId, int playableId) 
	{
		Print("RPC_TakePossession 1: " + playerId.ToString() + " - " + playableId);
		map<int, SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		SCR_ChimeraCharacter playable = SCR_ChimeraCharacter.Cast(playables[playableId].GetOwner());
		int curretPlayerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(playable);
		if (playable.GetDamageManager().IsDestroyed() 
				|| (curretPlayerId != 0 && curretPlayerId != playerId)) {
			
			RPC_PossesionResult(playerId, false);
			Rpc(RPC_PossesionResult, playerId, false);
			
			return;
		}
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		IEntity currentEntity = playerController.GetControlledEntity();
		if (playerId == curretPlayerId) {
			playerController.SetPossessedEntity(null);
		}
		if (currentEntity)
			playerController.SetInitialMainEntity(currentEntity); // Fix controlls and don't break camera
		playerController.SetPossessedEntity(playable); // reset ai? but still broken...
		playerController.SetInitialMainEntity(playable);
		
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		SCR_Faction faction = SCR_Faction.Cast(playable.GetFaction());
		playerFactionAffiliation.SetAffiliatedFaction(faction);
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
		
		Print("RPC_TakePossession 2: " + playerId.ToString() + " - " + playableId);
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
		
		RPC_PossesionResult(playerId, true);
		Rpc(RPC_PossesionResult, playerId, true);
		
		SCR_GameModeCoop.Cast(GetGame().GetGameMode()).UpdateMenu();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_PossesionResult(int playerId, bool isPossesed)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerId != playerController.GetPlayerId()) return;
		MenuManager menuManager = GetGame().GetMenuManager();
		
		SCR_PlayableSelectorMenu menu = SCR_PlayableSelectorMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.PlayableSelector));
		if (menu != null) {
			if (isPossesed) {
				SCR_GameModeCoop.Cast(GetGame().GetGameMode()).RemoveCamera();
				menu.Close();
			} else {
				menu.Unlock();
			}
		}
	}
	
	
}