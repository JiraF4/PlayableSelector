//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableComponentClass: ScriptComponentClass
{
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableComponent : ScriptComponent
{
	[Attribute()]
	protected string m_name;
	protected int m_id;
	SCR_ChimeraCharacter m_cOwner;
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddToList, 0, false, owner) // Rpl init delay
	}
	
	private void AddToList(IEntity owner)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if(rpl && owner.Type().ToString() == "SCR_ChimeraCharacter")
		{
			m_cOwner = SCR_ChimeraCharacter.Cast(owner);
			m_id = rpl.Id();
			playableManager.RegisterPlayable(this);
			rpl.EnableStreaming(false); // They need to be loaded for preview
		}
	}
	
	void ~PS_PlayableComponent()
	{
		RemoveFromList()
	}
	
	private void RemoveFromList()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.RemovePlayable(m_id);
	}
		
	string GetName()
	{
		return m_name;
	}
	
	RplId GetId()
	{
		return m_id;
	}
}