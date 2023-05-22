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
}