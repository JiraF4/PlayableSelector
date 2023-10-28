class PS_ManualMarkerComponent : SCR_ScriptedWidgetComponent
{
	protected SCR_MapEntity m_MapEntity;
	protected ImageWidget m_wMarkerIcon;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		m_wMarkerIcon = ImageWidget.Cast(w.FindAnyWidget("MarkerIcon"));
	}
	
	void SetImage(ResourceName m_sImageSet, string m_sQuadName)
	{
		m_wMarkerIcon.LoadImageFromSet(0, m_sImageSet, m_sQuadName);
	}
	
	void SetSlot(float posX, float posY, float sizeX, float sizeY)
	{
		FrameSlot.SetPos(m_wRoot, posX, posY);
		FrameSlot.SetSize(m_wRoot, sizeX, sizeY);
		FrameSlot.SetSize(m_wMarkerIcon, sizeX, sizeY);
	}
};