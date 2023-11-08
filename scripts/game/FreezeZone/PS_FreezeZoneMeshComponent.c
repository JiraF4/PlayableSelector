[ComponentEditorProps(category: "GameScripted/Area Mesh", description: "")]
class PS_FreezeZoneMeshComponentClass: SCR_BaseAreaMeshComponentClass
{
};
class PS_FreezeZoneMeshComponent: SCR_BaseAreaMeshComponent
{
	[Attribute("0", desc: "Wether or not the Mesh area will get the inner zone (warning) or outer zone (death) radius")]
	protected bool GetWarningRadius;
	
	override float GetRadius()
	{		
		PS_FreezeZone freezeZone = PS_FreezeZone.Cast(GetOwner());
		return freezeZone.GetZoneRadius();
	}
	
	override float GetHeight()
	{
		PS_FreezeZone freezeZone = PS_FreezeZone.Cast(GetOwner());
		return freezeZone.GetHeight();
	}
	
	override int GetResolution()
	{
		PS_FreezeZone freezeZone = PS_FreezeZone.Cast(GetOwner());
		int resolution = freezeZone.GetWallsCount();
		if (resolution > 30) resolution = 30;
		return resolution;
	}
	
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) GetOwner().SetFlags(EntityFlags.VISIBLE);
		else GetOwner().ClearFlags(EntityFlags.VISIBLE);
		
		GenerateAreaMesh();
	}
};