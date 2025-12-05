// Remove squad markers
modded class SCR_MapMarkerManagerComponent
{
	override void Update(float timeSlice)
	{
		if (!m_MapEntity)
		{
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			m_MapEntity.GetOnMapPanEnd().Insert(OnMapPanEnd);
			super.Update(timeSlice);
		}
		else
			super.Update(timeSlice);
	}
	
	override void OnMapPanEnd(float  x, float y)
	{
		if (m_MapEntity)
			super.OnMapPanEnd(x, y);
	}
}