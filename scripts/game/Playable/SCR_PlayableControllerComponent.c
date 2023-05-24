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
		map<int, SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		SCR_ChimeraCharacter playable = SCR_ChimeraCharacter.Cast(playables[playableId].GetOwner());
		if (playable.GetDamageManager().IsDestroyed() 
				|| SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(playable) != 0) {
			
			RPC_PossesionResult(playerId, false);
			Rpc(RPC_PossesionResult, playerId, false);
			
			return;
		}
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		IEntity currentEntity = playerController.GetControlledEntity();
		if (currentEntity)
			playerController.SetInitialMainEntity(currentEntity); // Fix controlls and don't break camera
		playerController.SetPossessedEntity(playable); // reset ai? but still broken...
		playerController.SetInitialMainEntity(playable);
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