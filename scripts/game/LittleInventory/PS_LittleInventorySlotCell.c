class PS_LittleInventorySlotCell : PS_LittleInventoryItemCell
{
	ImageWidget m_iSlotImage;
	
	override void HandlerAttached(Widget w)
	{
		m_iSlotImage = ImageWidget.Cast(w.FindAnyWidget("SlotImage"));
		super.HandlerAttached(w);
	}
	
	void SetSlot(PS_SlotCell slotCell)
	{
		SetItem(slotCell.GetItem());
		if (slotCell.GetImage() == "" || slotCell.GetItem()) return;
		m_iSlotImage.LoadImageTexture(0, slotCell.GetImage());
		m_iSlotImage.SetVisible(true);
	}
}