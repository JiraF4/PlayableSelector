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
	[Attribute()]
	protected bool m_bIsPlayable;
	
	protected int m_id;
	SCR_ChimeraCharacter m_cOwner;
	
	override void OnPostInit(IEntity owner)
	{
		m_cOwner = SCR_ChimeraCharacter.Cast(owner);
		GetGame().GetCallqueue().CallLater(AddToList, 0, false, owner) // init delay
	}
	
	// Get/Set Broadcast
	bool GetPlayable()
	{
		return m_bIsPlayable;
	}
	void SetPlayable(bool isPlayable)
	{
		RPC_SetPlayable(isPlayable);
		Rpc(RPC_SetPlayable, isPlayable);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayable(bool isPlayable)
	{
		m_bIsPlayable = isPlayable;
		if (m_bIsPlayable) GetGame().GetCallqueue().CallLater(AddToList, 0, false, m_cOwner);
		else RemoveFromList();
	}
	
	void ResetRplStream()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		//rpl.EnableStreaming(true);
	}
	
	private void RemoveFromList()
	{
		BaseGameMode gamemode = GetGame().GetGameMode();
		if (!gamemode)
			return;
		
		if (m_cOwner)
		{
			AIControlComponent aiComponent = AIControlComponent.Cast(m_cOwner.FindComponent(AIControlComponent));
			if (aiComponent)
			{
				AIAgent agent = aiComponent.GetAIAgent();
				if (agent) agent.ActivateAI();
			}
			
			RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
			rpl.EnableStreaming(true);
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.RemovePlayable(m_id);
	}
	
	private void AddToList(IEntity owner)
	{
		if (!m_bIsPlayable) return;
		
		AIControlComponent aiComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (aiComponent)
		{
			AIAgent agent = aiComponent.GetAIAgent();
			if (agent) agent.DeactivateAI();
		}
		
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		rpl.EnableStreaming(false);
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		m_id = rpl.Id();
		playableManager.RegisterPlayable(this);
	}
		
	string GetName()
	{
		if (m_name != "") return m_name;
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(m_cOwner.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo info = editableCharacterComponent.GetInfo();
		return info.GetName();
	}
	
	RplId GetId()
	{
		return m_id;
	}
	
	void PS_PlayableComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		
	}
	
	void ~PS_PlayableComponent()
	{
		RemoveFromList();
	}
	
	// Send our precision data, we need it on clients
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bIsPlayable);
		return true;
	}
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bIsPlayable);
		return true;
	}
}