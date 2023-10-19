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
	
	// List of all Playables
	private static ref map<int, PS_PlayableComponent> m_aPlayables = new ref map<int, PS_PlayableComponent>();
	
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
			
			if (Replication.IsServer()) {
				AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
				AIAgent agent = ctrl.GetAIAgent();
				SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
				string company, platoon, squad, sCharacter, format;
				group.GetCallsigns(company, platoon, squad, sCharacter, format);
				s_sGroupName = WidgetManager.Translate(format, company, platoon, squad, sCharacter);
				
				PS_GameModeCoop.Cast(GetGame().GetGameMode()).SetPlayableGroupName(m_id, s_sGroupName);
			}
		}
	}
	
	static map<int, PS_PlayableComponent> GetPlayables() 
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
	
	void ~PS_PlayableComponent() {
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
	
	string GetGroupName()
	{
		return PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetPlayableGroupName(m_id);
	}
}