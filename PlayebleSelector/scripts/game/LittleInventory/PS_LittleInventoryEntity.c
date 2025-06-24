class PS_LittleInventoryEntity : SCR_ScriptedWidgetComponent 
{
	ResourceName m_rInventoryIUConfig = "{024B56A4DE577001}Configs/Inventory/InventoryUI.conf";
	
	ResourceName m_rItemCell = "{28561DA61E961875}UI/LittleInventory/LittleInventoryItemCell.layout";
	ResourceName m_rSlotCell = "{297ED2A11D523941}UI/LittleInventory/LittleInventorySlotCell.layout";
	
	PS_LittleInventory m_iLittleInventory
	
	GridLayoutWidget m_gLittleInventoryEntityItems;
	GridLayoutWidget m_gLittleInventoryEntitySlots;
	
	TextWidget m_wEntityName;
	TextWidget m_wEntityWeight;
	TextWidget m_wEntityVolume;
	
	ButtonWidget m_wCloseButton;
	
	IEntity m_eEntity;
	
	ButtonWidget m_wItemsPanelButton;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		if (!GetGame().InPlayMode())
			return;
		
		m_gLittleInventoryEntityItems = GridLayoutWidget.Cast(w.FindAnyWidget("LittleInventoryEntityItems"));
		m_gLittleInventoryEntitySlots = GridLayoutWidget.Cast(w.FindAnyWidget("LittleInventoryEntitySlots"));
		
		m_wEntityName = TextWidget.Cast(w.FindAnyWidget("EntityName"));
		m_wEntityWeight = TextWidget.Cast(w.FindAnyWidget("EntityWeight"));
		m_wEntityVolume = TextWidget.Cast(w.FindAnyWidget("EntityVolume"));
		
		m_wCloseButton = ButtonWidget.Cast(w.FindAnyWidget("CloseButton"));
		m_wItemsPanelButton = ButtonWidget.Cast(w.FindAnyWidget("ItemsPanelButton"));
		
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(m_wCloseButton.FindHandler(SCR_ButtonBaseComponent));
		button.m_OnClicked.Insert(OnCloseClick);
	}
	
	void SetInventory(PS_LittleInventory littleInventory)
	{
		m_iLittleInventory = littleInventory;
	}
	
	void SetEntity(IEntity entity)
	{
		m_eEntity = entity;
		Refresh();
	}
	
	IEntity GetEntity()
	{
		return m_eEntity;
	}
	
	void ToMoveMode()
	{
		m_wItemsPanelButton.SetVisible(true);
	}
	
	void ResetMoveMode()
	{
		m_wItemsPanelButton.SetVisible(false);
	}
	
	void Refresh()
	{
		// Clear grid
		SCR_WidgetHelper.RemoveAllChildren(m_gLittleInventoryEntityItems);
		SCR_WidgetHelper.RemoveAllChildren(m_gLittleInventoryEntitySlots);
		
		// Get all storage types
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(m_eEntity.FindComponent(InventoryItemComponent));
		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(inventoryItem);
		ClothNodeStorageComponent ClothNode = ClothNodeStorageComponent.Cast(inventoryItem);
		ChimeraCharacter character = ChimeraCharacter.Cast(m_eEntity);
		
		/*	Main types
			Character inventory
			Cloth node aka Vest
			Base inventory like backpack
		*/
		
		array<ref PS_SlotCell> items = {};
		array<ref PS_SlotCell> slots = {};
		
		// temp
		m_wEntityWeight.SetText("");
		m_wEntityVolume.SetText("");
		
		// Try get info from item
		if (inventoryItem) {
			UIInfo info = inventoryItem.GetUIInfo();
			if (info) m_wEntityName.SetText(info.GetName());
		}
		
		// Get items and slots
		if (character)  
		{
			// Character has two inventory for some reason...
			SCR_CharacterInventoryStorageComponent characterInventoryStorage = SCR_CharacterInventoryStorageComponent.Cast(storage);
			SCR_EquipmentStorageComponent equipmentStorageComponent = SCR_EquipmentStorageComponent.Cast(characterInventoryStorage.FindComponent(SCR_EquipmentStorageComponent));
			
			// For characters get editable UIInfo
			SCR_EditableCharacterComponent editableCharacter = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
			SCR_UIInfo info = editableCharacter.GetInfo();
			m_wEntityName.SetText(info.GetName());
			
			// Get slots from first inventory
			int slotsCount = characterInventoryStorage.GetSlotsCount();
			for (int i = 0; i < slotsCount; i++)
			{
				InventoryStorageSlot slot = characterInventoryStorage.GetSlot(i);
				slots.Insert(new PS_SlotCell(characterInventoryStorage, slot.GetAttachedEntity(), "", i));
			}
			
			// Get slots from second inventory
			slotsCount = equipmentStorageComponent.GetSlotsCount();
			for (int i = 0; i < slotsCount; i++)
			{
				InventoryStorageSlot slot = equipmentStorageComponent.GetSlot(i);
				slots.Insert(new PS_SlotCell(equipmentStorageComponent, slot.GetAttachedEntity(), "", i));
			}
		} else if (ClothNode) {
			array<BaseInventoryStorageComponent> subStorages = {};
			ClothNode.GetOwnedStorages(subStorages, 1, false);
			
			// Get all items from sub inventories
			foreach (BaseInventoryStorageComponent subStorage : subStorages)
			{
				array<IEntity> entityItems = {};
				subStorage.GetAll(entityItems);
				foreach (IEntity item : entityItems)
				{
					items.Insert(new PS_SlotCell(ClothNode, item, "", -1))
				}
			}
		} else if (storage)	{
			array<IEntity> entityItems = new array<IEntity>();
			if (storage) 
			{
				storage.GetAll(entityItems);
				foreach (IEntity item : entityItems)
				{
					items.Insert(new PS_SlotCell(storage, item, "", -1))
				}
			}
		}
		
		array<ref PS_SlotCell> groupesItems = {};
		foreach (PS_SlotCell slotCell: items)
		{
			IEntity itemEntity = slotCell.GetItem();
			ResourceName resource = itemEntity.GetPrefabData().GetPrefabName();
			bool grouped;
			
			InventoryItemComponent inventoryItemGrouped = InventoryItemComponent.Cast(itemEntity.FindComponent(InventoryItemComponent));
			SCR_ItemAttributeCollection itemAttributeCollection = SCR_ItemAttributeCollection.Cast(inventoryItemGrouped.GetAttributes());
			
			if (itemAttributeCollection.IsStackable())
				foreach (PS_SlotCell slotCellGrouped: groupesItems)
				{
					ResourceName resourceGrouped = slotCellGrouped.GetItem().GetPrefabData().GetPrefabName();
					if (resourceGrouped == resource)
					{
						slotCellGrouped.m_iItemsCount++;
						grouped = true;
					}
				}
			
			if (!grouped)
			{
				groupesItems.Insert(slotCell);
			}
		}
		
		if (storage)
		{
			float occupiedSpace = storage.GetOccupiedSpace();
			float maxVolume = storage.GetMaxVolumeCapacity();
			float weight = storage.GetTotalWeight();
			
			//m_wEntityWeight.SetText("");
			m_wEntityVolume.SetTextFormat("%1/%2cm3", (int)(occupiedSpace/100), (int)(maxVolume/100));
			m_wEntityWeight.SetTextFormat("%1kg", weight);
		}
		
		if (!character)
			if (groupesItems.Count() == 0)
				groupesItems.Insert(new PS_SlotCell(storage, null, "", -1));
		
		FillSlotsGrid(slots, m_rSlotCell, m_gLittleInventoryEntitySlots);
		FillItemsGrid(groupesItems, storage, m_rItemCell, m_gLittleInventoryEntityItems);
	}
	
	void FillSlotsGrid(array<ref PS_SlotCell> slots, ResourceName cellResourceName, Widget littleInventoryEntityItems)
	{
		int x = 0;
		int y = 0;
		
		foreach (PS_SlotCell slot : slots)
		{
			CreateSlot(x, y, 1, 1, slot, cellResourceName, littleInventoryEntityItems);
			
			x++;
			if (x >= 7)
			{
				x = 0;
				y++;
			}
		}
	}
	
	void FillItemsGrid(array<ref PS_SlotCell> slots, BaseInventoryStorageComponent storage, ResourceName cellResourceName, Widget littleInventoryEntityItems)
	{
		PS_LittleInventoryMatrix matrix = new PS_LittleInventoryMatrix(7, 0);
		
		foreach (PS_SlotCell slot : slots)
		{
			int w = 1, h = 1;
			IEntity itemEntity = slot.GetItem();
			if (itemEntity)
			{
				InventoryItemComponent inventoryItemGrouped = InventoryItemComponent.Cast(itemEntity.FindComponent(InventoryItemComponent));
				SCR_ItemAttributeCollection itemAttributeCollection = SCR_ItemAttributeCollection.Cast(inventoryItemGrouped.GetAttributes());
				ESlotSize itemSize = itemAttributeCollection.GetItemSize();
				switch(itemSize)
				{
					case ESlotSize.SLOT_2x1: { w = 2; h = 1;} break;
					case ESlotSize.SLOT_2x2: { w = 2; h = 2;} break;
					case ESlotSize.SLOT_3x3: { w = 3; h = 3;} break;
				}
			}
			int x, y;
			matrix.ReserveFirstFreePlace(x, y, w, h);
			
			CreateSlot(x, y, w, h, slot, cellResourceName, littleInventoryEntityItems);
		}
		
		for (int x = 0; x < matrix.GetColumnsCount(); x++)
		{
			for (int y = 0; y < matrix.GetRowsCount(); y++)
			{
				if (matrix.IsPlaceFree(x, y, 1, 1))
					CreateSlot(x, y, 1, 1, new PS_SlotCell(storage, null, "", -1), cellResourceName, littleInventoryEntityItems);
			}
		}
	}
	
	void CreateSlot(int x, int y, int w, int h, PS_SlotCell slot, ResourceName cellResourceName, Widget littleInventoryEntityItems)
	{
		Widget itemCell = GetGame().GetWorkspace().CreateWidgets(cellResourceName, littleInventoryEntityItems);
		SizeLayoutWidget itemCellSize = SizeLayoutWidget.Cast(itemCell);
		
		GridSlot.SetColumn(itemCell, x + 1);
		GridSlot.SetRow(itemCell, y + 1);
		GridSlot.SetColumnSpan(itemCell, w);
		GridSlot.SetRowSpan(itemCell, h);
		
		itemCellSize.SetWidthOverride(64.0 * w);
		itemCellSize.SetHeightOverride(64.0 * h);
		
		PS_LittleInventoryItemCell littleInventorySlotCell = PS_LittleInventoryItemCell.Cast(itemCell.FindHandler(PS_LittleInventoryItemCell));
		littleInventorySlotCell.SetInventoryEntity(this, slot.GetSlotId());
		littleInventorySlotCell.SetCell(slot);
		littleInventorySlotCell.SetStorage(slot.GetInventory());
	}
	
	// events
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		Widget widget = m_gLittleInventoryEntityItems.GetChildren();
		if (!widget) return false;
		PS_LittleInventoryItemCell cell = PS_LittleInventoryItemCell.Cast(widget.FindHandler(PS_LittleInventoryItemCell));
		OnCellSelectLost(cell);
		return true;
	};
	
	void OnCellSelect(PS_LittleInventoryItemCell cell)
	{
		m_iLittleInventory.OnCellSelect(cell);
	}
	void OnCellSelectLost(PS_LittleInventoryItemCell cell)
	{
		m_iLittleInventory.OnCellSelectLost(cell);
	}
	
	void OnCellFocus(PS_LittleInventoryItemCell cell)
	{
		m_iLittleInventory.OnCellFocus(cell);
	}
	void OnCellFocusLost(PS_LittleInventoryItemCell cell)
	{
		m_iLittleInventory.OnCellFocusLost(cell);
	}
	
	void OnCellClick(PS_LittleInventoryItemCell cell)
	{
		m_iLittleInventory.OnCellClick(cell)
	}
	void OnCloseClick(SCR_ButtonTextComponent button)
	{
		m_wRoot.RemoveFromHierarchy();
		if (m_iLittleInventory)
			m_iLittleInventory.EntityRemoved(this);
	}
}

class PS_SlotCell
{
	BaseInventoryStorageComponent m_iSlotInventory;
	IEntity m_eItem;
	ResourceName m_rSlotImage;
	int m_iSlotId;
	int m_iItemsCount = 1;
	
	void PS_SlotCell(BaseInventoryStorageComponent slotInventory, IEntity item, ResourceName slotImage, int slotId)
	{
		m_iSlotInventory = slotInventory;
		m_eItem = item;
		m_rSlotImage = slotImage;
		m_iSlotId = slotId;
	}
	
	BaseInventoryStorageComponent GetInventory()
	{
		return m_iSlotInventory;
	}
	
	IEntity GetItem()
	{
		return m_eItem;
	}
	
	ResourceName GetImage()
	{
		return m_rSlotImage;
	}
	
	int GetSlotId()
	{
		return m_iSlotId;
	}
	
	int GetCount()
	{
		return m_iItemsCount;
	}
}




































