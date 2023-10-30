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
	
	protected void PositionLogger()
	{
		// Regulary write position to replay
		PS_ReplayWriter.GetInstance().WriteEntityMove(m_id, m_cOwner);
		GetGame().GetCallqueue().CallLater(PositionLogger, 100);
	}
	
	protected void DieLogger(EDamageState state)
	{
		PS_ReplayWriter.GetInstance().WriteCharacterDamageStateChanged(m_id, state);
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
			if (Replication.IsServer()) { // Register character to replay
				SCR_CharacterDamageManagerComponent damageComponent = SCR_CharacterDamageManagerComponent.Cast(m_cOwner.FindComponent(SCR_CharacterDamageManagerComponent));
				ScriptInvoker damageEvent = damageComponent.GetOnDamageStateChanged();
				damageEvent.Insert(DieLogger);
				PS_ReplayWriter.GetInstance().WriteCharacterRegistration(m_id, m_cOwner);
				GetGame().GetCallqueue().CallLater(PositionLogger, 0);
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