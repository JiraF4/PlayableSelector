class PS_LittleInventoryItemCell : SCR_ScriptedWidgetComponent
{
	BaseInventoryStorageComponent m_iCellInventory;
	
	ItemPreviewWidget m_iItemPreview;
	IEntity m_eItemEntity;
	
	PS_LittleInventoryEntity m_iLittleInventoryEntity;
	int m_iSlotId;
	
	SCR_ButtonTextComponent m_bButton;
	
	TextWidget m_tCountText;
	
	override void HandlerAttached(Widget w)
	{
		m_iItemPreview = ItemPreviewWidget.Cast(w.FindAnyWidget("ItemPreview"));
		m_tCountText = TextWidget.Cast(w.FindAnyWidget("CountText"));
		m_bButton = SCR_ButtonTextComponent.Cast(w.FindAnyWidget("Button").FindHandler(SCR_ButtonTextComponent));
		m_bButton.m_OnClicked.Insert(OnCellClick);
		m_bButton.m_OnFocus.Insert(OnCellFocus);
		m_bButton.m_OnFocusLost.Insert(OnCellFocusLost);
		super.HandlerAttached(w);
	}
	
	void SetStorage(BaseInventoryStorageComponent cellInventory)
	{
		m_iCellInventory = cellInventory;
	}
	
	BaseInventoryStorageComponent GetStorage()
	{
		return m_iCellInventory;
	}
	
	void SetCell(PS_SlotCell slotCell)
	{
		SetItem(slotCell.GetItem());
		int count = slotCell.GetCount();
		if (count > 1)
			m_tCountText.SetText(count.ToString());
		else
			m_tCountText.SetText("");
	}
	
	void SetItem(IEntity itemEntity)
	{
		m_eItemEntity = itemEntity;
		
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		ItemPreviewManagerEntity previewManager = world.GetItemPreviewManager();
		if (!previewManager)
		{
			Resource rsc = Resource.Load("{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et");
			if (rsc.IsValid())
				GetGame().SpawnEntityPrefabLocal(rsc, world);
			previewManager = world.GetItemPreviewManager();
		}
		previewManager.SetPreviewItem(m_iItemPreview, itemEntity);
	}
	
	IEntity GetItem()
	{
		return m_eItemEntity;
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0) return true;
		OnCellSelect();
		return true;
	};
	
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0) return true;
		OnCellSelectLost();
		return true;
	};
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		GetGame().GetWorkspace().SetFocusedWidget(w);
		return super.OnMouseEnter(w, x, y);
	};
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		GetGame().GetWorkspace().SetFocusedWidget(null);
		return super.OnMouseLeave(w, enterW, x, y);
	};
	
	void SetInventoryEntity(PS_LittleInventoryEntity littleInventoryEntity, int slotId = -1)
	{
		m_iLittleInventoryEntity = littleInventoryEntity;
		m_iSlotId = slotId;
	}
	
	int GetSlotId()
	{
		return m_iSlotId;
	}
	
	PS_LittleInventoryEntity GetInventoryEntity()
	{
		return m_iLittleInventoryEntity;
	}
	
	// events
	void OnCellSelect()
	{
		if (!m_iLittleInventoryEntity) return;
		m_iLittleInventoryEntity.OnCellSelect(this);
	}
	void OnCellSelectLost()
	{
		if (!m_iLittleInventoryEntity) return;
		m_iLittleInventoryEntity.OnCellSelectLost(this);
	}
	
	void OnCellFocus()
	{
		if (!m_iLittleInventoryEntity) return;
		m_iLittleInventoryEntity.OnCellFocus(this);
	}
	
	void OnCellFocusLost()
	{
		if (!m_iLittleInventoryEntity) return;
		m_iLittleInventoryEntity.OnCellFocusLost(this);
	}
	
	void OnCellClick(SCR_ButtonTextComponent button)
	{
		OnCellSelectLost();
		m_iLittleInventoryEntity.OnCellClick(this);
	}
}