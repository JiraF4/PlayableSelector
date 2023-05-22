//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableControllerComponentClass: ScriptComponentClass
{
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableControllerComponent : ScriptComponent
{
	void TakePossession(int playerId, int playableId) 
	{
		Rpc(RPC_TakePossession, playerId, playableId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_TakePossession(int playerId, int playableId) 
	{
		array<SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
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
		if (isPossesed) {
			SCR_GameModeCoop.Cast(GetGame().GetGameMode()).RemoveCamera();
			menu.Close();
		} else {
			menu.Unlock();
		}
	}
	
	
}