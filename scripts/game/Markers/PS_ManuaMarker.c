class PS_ManualMarkerClass : GenericEntityClass
{

}

class PS_ManualMarker : GenericEntity
{
	[Attribute("{52CA8FF5F56C6F31}UI/layouts/Map/ManualMapMarkerBase.layout")]
	protected ResourceName m_sMarkerPrefab;
	[Attribute("{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset")]
	protected ResourceName m_sImageSet;
	[Attribute("empty")]
	protected string m_sQuadName;
	
	[Attribute("5.0")]
	protected float m_fWorldSize;
	
	FrameWidget m_wRoot;
	SCR_MapEntity m_MapEntity;
	PS_ManualMarkerComponent m_hManualMarkerComponent;
	
	//
	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		if (!m_wRoot)
			return;
		
		// Get screen position
		float wX, wY, screenX, screenY, screenXEnd, screenYEnd;
		vector worldPosition = GetOrigin();
		wX = worldPosition[0];
		wY = worldPosition[2];
		m_MapEntity.WorldToScreen(wX, wY, screenX, screenY, true);
		m_MapEntity.WorldToScreen(wX + m_fWorldSize, wY + m_fWorldSize, screenXEnd, screenYEnd, true);
		
		float screenXD = GetGame().GetWorkspace().DPIUnscale(screenX);
		float screenYD = GetGame().GetWorkspace().DPIUnscale(screenY);
		float sizeXD = GetGame().GetWorkspace().DPIUnscale(screenXEnd - screenX);
		float sizeYD = GetGame().GetWorkspace().DPIUnscale(screenY - screenYEnd); // Y flip
		
		m_hManualMarkerComponent.SetSlot(screenXD, screenYD, sizeXD, sizeYD);
	}
	
	//
	override protected void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(CreateMap, 1000);
	}
	
	void CreateMap()
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		m_wRoot = FrameWidget.Cast(GetGame().GetWorkspace().CreateWidgets(m_sMarkerPrefab, mapFrame));
		m_hManualMarkerComponent = PS_ManualMarkerComponent.Cast(m_wRoot.FindHandler(PS_ManualMarkerComponent));
		m_hManualMarkerComponent.SetImage(m_sImageSet, m_sQuadName);
	}
	
	//
	void PS_ManualMarker(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
		SetEventMask(EntityEvent.POSTFRAME);
	}
}