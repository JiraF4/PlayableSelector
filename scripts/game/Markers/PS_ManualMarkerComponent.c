class PS_ManualMarkerComponent : SCR_ScriptedWidgetComponent
{
	protected SCR_MapEntity m_MapEntity;
	protected ImageWidget m_wMarkerIcon;
	protected ImageWidget m_wMarkerGlowIcon;
	protected FrameWidget m_wMarkerFrame;
	protected FrameWidget m_wDescriptionFrame;
	protected RichTextWidget m_wDescriptionText;
	protected ScrollLayoutWidget m_wMarkerScrollLayout;
	protected string m_sDescription;
	protected bool m_bHasGlow;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		m_wMarkerIcon = ImageWidget.Cast(w.FindAnyWidget("MarkerIcon"));
		m_wMarkerGlowIcon = ImageWidget.Cast(w.FindAnyWidget("MarkerGlowIcon"));
		m_wMarkerFrame = FrameWidget.Cast(w.FindAnyWidget("MarkerFrame"));
		m_wDescriptionFrame = FrameWidget.Cast(w.FindAnyWidget("DescriptionFrame"));
		m_wDescriptionText = RichTextWidget.Cast(w.FindAnyWidget("DescriptionText"));
		m_wMarkerScrollLayout = ScrollLayoutWidget.Cast(w.FindAnyWidget("MarkerScrollLayout"));
		
	}
	
	void SetImage(ResourceName m_sImageSet, string quadName)
	{
		m_wMarkerIcon.LoadImageFromSet(0, m_sImageSet, quadName);
	}
	void SetImageGlow(ResourceName m_sImageSet, string quadName)
	{
		if (m_sImageSet != "") m_bHasGlow = true;
		m_wMarkerGlowIcon.LoadImageFromSet(0, m_sImageSet, quadName);
	}
	void SetDescription(string description)
	{
		m_sDescription = description;
		
		// Workbench crush if i use it with out wrap... WTF?
		// m_wDescriptionText.SetText(description);
		
		SetDescriptionWrap(description);
	}
	void SetDescriptionWrap(string description)
	{
		m_wDescriptionText.SetText(description);	
	}
	void SetColor(Color color)
	{
		m_wMarkerIcon.SetColor(color);	
		m_wMarkerGlowIcon.SetColor(color);	
	}
	
	void SetSlot(float posX, float posY, float sizeX, float sizeY)
	{
		FrameSlot.SetPos(m_wRoot, posX - sizeX/2, posY - sizeY/2);
		FrameSlot.SetPos(m_wDescriptionFrame, sizeX/2 + sizeX/4, sizeY/2 + sizeY/4);
		FrameSlot.SetSize(m_wMarkerIcon, sizeX, sizeY);
		FrameSlot.SetSize(m_wMarkerGlowIcon, sizeX, sizeY);
		FrameSlot.SetSize(m_wMarkerScrollLayout, sizeX, sizeY);
		FrameSlot.SetSize(m_wMarkerFrame, sizeX, sizeY);
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_bHasGlow) m_wMarkerGlowIcon.SetVisible(true);
		if (m_sDescription != "") m_wDescriptionFrame.SetVisible(true);
		
		return true;
	}
	
	
	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_wMarkerGlowIcon.SetVisible(false);	
		m_wDescriptionFrame.SetVisible(false);
		
		return true;
	}
};