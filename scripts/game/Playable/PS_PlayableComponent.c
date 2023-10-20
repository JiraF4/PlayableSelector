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
	protected string s_sGroupName; // dead has no group, soo cache name
	
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
			m_id = rpl.Id();
			playableManager.RegisterPlayable(this);
			rpl.EnableStreaming(false); // They need to be loaded for preview
			
			if (Replication.IsServer() && !Replication.IsClient()) {
				AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
				AIAgent agent = ctrl.GetAIAgent();
				SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
				string company, platoon, squad, sCharacter, format;
				group.GetCallsigns(company, platoon, squad, sCharacter, format);
				s_sGroupName = WidgetManager.Translate(format, company, platoon, squad, sCharacter);
				
				playableManager.SetPlayableGroupName(m_id, s_sGroupName);
			}
		}
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