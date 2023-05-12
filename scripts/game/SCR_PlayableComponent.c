//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableComponentClass: ScriptComponentClass
{
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_PlayableComponent : ScriptComponent
{
	[Attribute()]
	protected string m_name;
	
	// List of all Playables
	private static ref array<SCR_PlayableComponent> m_aPlayables = new ref array<SCR_PlayableComponent>();
	
	override void OnPostInit(IEntity owner)
	{
		if(owner.Type().ToString() == "SCR_ChimeraCharacter")
			addPlayable(this);
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rpl) rpl.EnableStreaming(false);
	}
	
	static array<SCR_PlayableComponent> GetPlayables() 
	{
		return m_aPlayables;
	}
	
	static void addPlayable(SCR_PlayableComponent ent)
	{
		m_aPlayables.Insert(ent);
	}
	
	static void removePlayable(SCR_PlayableComponent ent)
	{
		m_aPlayables.RemoveItem(ent);
	}
	
	static void ClearPlayables()
	{
		m_aPlayables.Clear();
	}
	
	void ~SCR_PlayableComponent() {
		removePlayable(this);
	}
	
	string GetName()
	{
		return m_name;
	}
	
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
		
		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId)).SetPossessedEntity(playable);
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