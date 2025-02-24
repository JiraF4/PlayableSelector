[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_DisableCharacterCollisionComponentClass : ScriptComponentClass
{
}

class PS_DisableCharacterCollisionComponent : ScriptComponent
{
	RplComponent m_RplComponent;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		GetGame().GetCallqueue().CallLater(Recolor, 1000, false, owner);
	}
	
	void Recolor(IEntity owner)
	{
		if (!m_RplComponent.IsOwner())
			return;
		
		VObject obj = owner.GetVObject();		
		string materials[256];
		int numMats = obj.GetMaterials(materials);
		string remap = "";
		for (int i = 0; i < numMats; i++)
		{
			remap += "$remap '" + materials[i] + "' '{788C42E96DB8587C}Assets/Editor/VirtualArea/VirtualArea_01_Danger.emat';";
		}
		owner.SetObject(obj, remap);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_RplComponent.IsOwner())
			return;
		
		if (SCR_PlayerController.GetLocalControlledEntity() != owner)
			Rpc(RPC_DeleteMe)
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DeleteMe()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
