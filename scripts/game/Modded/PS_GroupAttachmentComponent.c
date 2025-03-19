class PS_GroupAttachmentComponentClass : ScriptComponentClass
{

}

class PS_GroupAttachmentComponent : ScriptComponent
{
	[Attribute()]
	string m_sAttachmentGroupName;
	
	void PS_GroupAttachmentComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (GetGame().GetCallqueue())
			GetGame().GetCallqueue().Call(RegisterToMissionDate);
	}
	
	void RegisterToMissionDate()
	{		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(m_sAttachmentGroupName));
			if (group)
			{
				RplId id = Replication.FindId(GetOwner());
				playableManager.RegisterGroupVehicle(id, group, GetOwner());
			}
		}
	}
	
	void ~PS_GroupAttachmentComponent()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			RplId id = Replication.FindId(GetOwner());
			playableManager.UnRegisterGroupVehicle(id);
		}
	}
}