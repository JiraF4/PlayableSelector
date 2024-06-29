class PS_SpawnClass : GenericEntityClass
{

}

class PS_Spawn : GenericEntity
{
	[Attribute("")]
	ResourceName m_sPrefabName;
	
	IEntity m_PreviewEntity;
	
	override void EOnInit(IEntity owner)
	{
		if (GetGame().InPlayMode())
		{
			GetGame().GetCallqueue().Call(Spawn,);
		}
		else
			Spawn()
	}
	
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		vector mat1[4];
		vector mat2[4];
		GetTransform(mat1);
		m_PreviewEntity.GetTransform(mat2);
		if (SCR_Math3D.MatrixEqual(mat1, mat2))
			return;
		UpdatePreview();
	}
	
	override void _WB_OnContextMenu(int id)
	{
		UpdatePreview();
	}
	
	void UpdatePreview()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
		Spawn();
	}
	
	void Spawn()
	{
		Resource resource = Resource.Load(m_sPrefabName);
		EntitySpawnParams params = new EntitySpawnParams();
		GetTransform(params.Transform);
		m_PreviewEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (GetGame().InPlayMode())
			SCR_EntityHelper.DeleteEntityAndChildren(this);
	}
	
	void PS_Spawn(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
	void ~PS_Spawn()
	{
		if (!GetGame().InPlayMode())
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
	}
}