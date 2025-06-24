class PS_LittleInventoryItemInfo : SCR_ScriptedWidgetComponent
{
	TextWidget m_wEntityName;
	TextWidget m_wEntityDescription;
	
	PS_LittleInventoryItemCell m_hLittleInventoryItemCell;
	Widget m_wParent;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wParent = w.GetParent();
		m_wEntityName = TextWidget.Cast(w.FindAnyWidget("EntityName"));
		m_wEntityDescription = TextWidget.Cast(w.FindAnyWidget("EntityDescription"));
	}
	
	void SetCell(PS_LittleInventoryItemCell itemCell)
	{
		m_hLittleInventoryItemCell = itemCell;
		if (!itemCell) {
			m_wRoot.SetVisible(false);
			return;
		}
			
		// Show only for cells with item
		IEntity entity = itemCell.GetItem();
		if (!itemCell.GetItem())
		{
			m_wRoot.SetVisible(false);
			return;
		}
		m_wRoot.SetVisible(true);
		
		// Set info
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
		if (inventoryItem)
		{
			UIInfo info = inventoryItem.GetUIInfo();
			if (info)
			{
				m_wEntityName.SetText(info.GetName());
				m_wEntityDescription.SetText(info.GetDescription());
			}
			else 
			{
				m_wRoot.SetVisible(false);
				return;
			}
		}
		
		// Set pos
		Widget itemCellWidget = itemCell.GetRootWidget();
		float xs, ys, ws, hs, pxs, pys;
		itemCellWidget.GetScreenPos(xs, ys);
		itemCellWidget.GetScreenSize(ws, hs);
		m_wParent.GetScreenPos(pxs, pys);
		xs = xs - pxs;
		ys = ys + hs - pys;
		float x, y;
		x = GetGame().GetWorkspace().DPIUnscale(xs);
		y = GetGame().GetWorkspace().DPIUnscale(ys);
		FrameSlot.SetPos(m_wRoot, x, y);
	}
}