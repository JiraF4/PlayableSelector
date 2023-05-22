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
	protected int m_id;
	
	// List of all Playables
	private static ref map<int, SCR_PlayableComponent> m_aPlayables = new ref map<int, SCR_PlayableComponent>();
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddToList, 0, false, owner) // Rpl init delay
	}
	
	private void AddToList(IEntity owner)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if(rpl && owner.Type().ToString() == "SCR_ChimeraCharacter")
		{
			m_id = rpl.Id();
			m_aPlayables.Set(m_id, this);
			rpl.EnableStreaming(false); // They need to be loaded for preview
		}
	}
	
	static map<int, SCR_PlayableComponent> GetPlayables() 
	{
		return m_aPlayables;
	}
	
	static void removePlayable(int playableId)
	{
		m_aPlayables.Remove(playableId);
	}
	
	static void ClearPlayables()
	{
		m_aPlayables.Clear();
	}
	
	void ~SCR_PlayableComponent() {
		removePlayable(m_id);
	}
	
	string GetName()
	{
		return m_name;
	}	
	
	int GetId()
	{
		return m_id;
	}	
}