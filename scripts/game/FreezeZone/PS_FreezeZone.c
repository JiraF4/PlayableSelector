class PS_FreezeZoneClass: GenericEntityClass
{
	
};
class PS_FreezeZone: GenericEntity
{
	[Attribute("12", desc: "Generate walls in radius")]
	protected float m_fZoneRadius;
	
	[Attribute("2", desc: "Generate walls in radius")]
	protected float m_fWallWidth;
	
	[Attribute("4", desc: "Generate walls in radius")]
	protected float m_fWallHeight;
	
	[Attribute("2", desc: "Generate walls in radius")]
	protected float m_fZoneHeight;
	
	ref protected array<IEntity> m_aWalls = new array<IEntity>();
	
	float GetZoneRadius()
	{
		return m_fZoneRadius;
	}
	
	float GetHeight()
	{
		return m_fWallHeight * m_fZoneHeight/2;
	}
	
	int GetWallsCount()
	{
		int resolution = (m_fZoneRadius * Math.PI) / m_fWallWidth;
		if (resolution < 4) resolution = 4;
		return resolution;
	}
	
	override protected void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;
		int resolution = GetWallsCount();
		float dirStep = Math.PI2 / resolution;
		for (int v = 0; v < resolution; v++)
		{
			float dir = dirStep * v;
			vector pos = Vector(Math.Sin(dir) * m_fZoneRadius, 0, Math.Cos(dir) * m_fZoneRadius);
			vector nextPos = Vector(Math.Sin(dir + dirStep) * m_fZoneRadius, 0, Math.Cos(dir + dirStep) * m_fZoneRadius);
			pos = (pos + nextPos) / 2;
			Resource resource = Resource.Load("{0F93B8B1F1EE090A}Prefabs/FreezeTime/FreezeWall.et");
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = pos + GetOrigin();
			vector rotation = "0 0 0";
			rotation[0] = (dir + dirStep/2) * Math.RAD2DEG;
			for (int h = 0; h < m_fZoneHeight; h++)
			{
				params.Transform[3][1] = m_fWallHeight - (m_fWallHeight*m_fZoneHeight)/2 + h*m_fWallHeight;
        		IEntity wall = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
				wall.SetYawPitchRoll(rotation);
				m_aWalls.Insert(wall);
			}
		}
		
		PS_FreezeZoneManager.GetInstance().RegisterZone(this);
	}
	
	void ~PS_FreezeZone()
	{
		foreach (IEntity wall: m_aWalls)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(wall);
		}
		PS_FreezeZoneManager freezeZoneManager = PS_FreezeZoneManager.GetInstance();
		if (freezeZoneManager) freezeZoneManager.UnRegisterZone(this);
	}
}