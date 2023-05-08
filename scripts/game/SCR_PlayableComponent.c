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
		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId)).SetPossessedEntity(playables[playableId].GetOwner());
		
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		if (entity) Print(entity.Type().ToString());
		
		//Print("Possesed");
	}
	
	
}