class PS_ImportantSlot : SCR_ScriptedWidgetComponent
{
	ItemPreviewWidget m_wItemPreview;
	ImageWidget m_wImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wItemPreview = ItemPreviewWidget.Cast(w.FindWidget("ItemPreview"));
		m_wImage = ImageWidget.Cast(w.FindWidget("Image"));
	}
	
	void SetItemEntity(IEntity itemEntity)
	{
		ItemPreviewManagerEntity m_PreviewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		m_PreviewManager.SetPreviewItem(m_wItemPreview, itemEntity);
		m_wItemPreview.SetRefresh(1, 1);
		if (itemEntity) m_wImage.SetOpacity(0.0);
		else m_wImage.SetOpacity(1.0);
	}
}