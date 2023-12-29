class PS_LoadoutInspectorCustomTitle: BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		title = "Loadout";
		return true;
	}
}

[BaseContainerProps(), PS_LoadoutInspectorSlotsListCustomTitle()]
class PS_LoadoutInspector
{
	IEntitySource m_entitySource;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_primaryWeapon;
	BaseContainer m_primaryWeaponContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_secondWeapon;
	BaseContainer m_secondWeaponContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_handWeapon;
	BaseContainer m_handWeaponContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_grenade;
	BaseContainer m_grenadeContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_throwable;
	BaseContainer m_throwableContainer;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_helmet;
	BaseContainer m_helmetContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_jacket;
	[Attribute("")]
	ref array<ref PS_LoadoutInspectorItemsList> m_jacketItems;
	BaseContainer m_jacketContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_armorVest;
	BaseContainer m_armorVestContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_pants;
	[Attribute("")]
	ref array<ref PS_LoadoutInspectorItemsList> m_pantsItems;
	BaseContainer m_pantsContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_boots;
	BaseContainer m_bootsContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_backpack;
	[Attribute("")]
	ref array<ref PS_LoadoutInspectorItemsList> m_backpackItems;
	BaseContainer m_backpackContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_vest;
	[Attribute("")]
	ref array<ref PS_LoadoutInspectorSlotsList> m_vestSlots;
	BaseContainer m_vestContainer;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_binoculars;
	BaseContainer m_binocularsContainer;
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_watch;
	BaseContainer m_watchContainer;
	
	[Attribute("", UIWidgets.Auto, "et")]
	ref array<ref PS_LoadoutInspectorError> m_errors;
	
	void SetItems(WorldEditorAPI wApi, IEntitySource entitySource)
	{
		IEntityComponentSource baseLoadoutManagerComponent = null;
		array<ref Tuple2<int, IEntityComponentSource>> weaponSlots = new array<ref Tuple2<int, IEntityComponentSource>>();
		BaseContainer equipmentStorageComponent = null;
		IEntityComponentSource inventoryStorageManagerComponent = null;
		int componentsCount = entitySource.GetComponentCount();
		for (int r = 0; r < componentsCount; r++)
		{
			IEntityComponentSource componentSource = entitySource.GetComponent(r);
			if (componentSource.GetClassName() == "BaseLoadoutManagerComponent")
			{
				baseLoadoutManagerComponent = componentSource;
			}
			if (componentSource.GetClassName() == "CharacterGrenadeSlotComponent"
			 || componentSource.GetClassName() == "CharacterWeaponSlotComponent")
			{
				weaponSlots.Insert(new Tuple2<int, IEntityComponentSource>(r, componentSource));
			}
			if (componentSource.GetClassName() == "SCR_CharacterInventoryStorageComponent")
			{
				BaseContainerList subComponents = componentSource.GetObjectArray("components");
				for (int j = 0; j < subComponents.Count(); j++)
				{
					BaseContainer subComponent = subComponents.Get(j);
					if (subComponent.GetClassName() == "SCR_EquipmentStorageComponent")
					{
						equipmentStorageComponent = subComponent;
					}
				}
			}
			if (componentSource.GetClassName() == "SCR_InventoryStorageManagerComponent")
			{
				inventoryStorageManagerComponent = componentSource;
			}
		}
		
		int varCount = entitySource.GetComponentCount();
		for (int i = 0; i < varCount; i++)
		{
			Print(entitySource.GetVarName(i));
		}
		
		BaseContainerList slots = baseLoadoutManagerComponent.SetObjectArray("Slots");
		for (int s = 0; s < slots.Count(); s++)
		{
			BaseContainer slotContainer = slots.Get(s);
			LoadoutAreaType areaType;
			slotContainer.Get("AreaType", areaType);
			ResourceName itemPrefabOld;
			slotContainer.Get("Prefab", itemPrefabOld);
			
			ResourceName itemPrefab = GetItemByLoadoutArea(areaType);
			if (itemPrefabOld == itemPrefab) continue;
			
			wApi.SetVariableValue(entitySource, {ContainerIdPathEntry("BaseLoadoutManagerComponent"), ContainerIdPathEntry("Slots", s)}, "Prefab", itemPrefab);
		}
		
		BaseContainerList slotsEquipment = equipmentStorageComponent.GetObjectArray("InitialStorageSlots");
		if (true)
		{
			ResourceName itemPrefabOld;
			slotsEquipment.Get(0).Get("Prefab", itemPrefabOld);
			ResourceName itemPrefab = GetItemByLoadoutArea(new LoadoutBinocularsArea());
			if (itemPrefabOld != itemPrefab)
				wApi.SetVariableValue(entitySource, {ContainerIdPathEntry("SCR_CharacterInventoryStorageComponent"), ContainerIdPathEntry("components", 0), ContainerIdPathEntry("InitialStorageSlots", 0)}, "Prefab", itemPrefab);
		}
		if (true)
		{
			ResourceName itemPrefabOld;
			slotsEquipment.Get(1).Get("Prefab", itemPrefabOld);
			ResourceName itemPrefab = GetItemByLoadoutArea(new LoadoutWatchArea());
			if (itemPrefabOld != itemPrefab)
				wApi.SetVariableValue(entitySource, {ContainerIdPathEntry("SCR_CharacterInventoryStorageComponent"), ContainerIdPathEntry("components", 0), ContainerIdPathEntry("InitialStorageSlots", 1)}, "Prefab", itemPrefab);
		}
		
		foreach(Tuple2<int, IEntityComponentSource> t : weaponSlots)
		{
			int componetnId = t.param1; 
			IEntityComponentSource componentSource = t.param2;
			ResourceName itemPrefabOld;
			componentSource.Get("WeaponTemplate", itemPrefabOld);
			int slotIndex;
			componentSource.Get("WeaponSlotIndex", slotIndex);
			ResourceName itemPrefab = GetWeaponItemPrefab(slotIndex);
			if (itemPrefabOld != itemPrefab)
				wApi.SetVariableValue(entitySource, {ContainerIdPathEntry("components", componetnId)}, "WeaponTemplate", itemPrefab);
		}
		
		BaseContainerList slotsList = inventoryStorageManagerComponent.GetObjectArray("InitialInventoryItems");
		if (slotsList.Count() == 0)
			wApi.CreateObjectArrayVariableMember(entitySource, {ContainerIdPathEntry("SCR_InventoryStorageManagerComponent")}, "InitialInventoryItems", "ItemsInitConfigurationItem", 0);
		while (slotsList.Count() > 1)
			wApi.RemoveObjectArrayVariableMember(entitySource, {ContainerIdPathEntry("SCR_InventoryStorageManagerComponent")}, "InitialInventoryItems", slotsList.Count() - 1);
		
		array<ref PS_LoadoutInspectorSlotsList> allSlots = getAllSlots();
		for (int i = 0; i < allSlots.Count(); i++)
		{
			PS_LoadoutInspectorSlotsList itemSlot = allSlots.Get(i);
			if (itemSlot.m_slot == "")
				continue;
			
			wApi.CreateObjectArrayVariableMember(entitySource, {ContainerIdPathEntry("SCR_InventoryStorageManagerComponent")}, "InitialInventoryItems", "ItemsInitConfigurationItem", i+1);
			
			array<ref ContainerIdPathEntry> containerPath = new array<ref ContainerIdPathEntry>();
			containerPath = {ContainerIdPathEntry("SCR_InventoryStorageManagerComponent"), ContainerIdPathEntry("InitialInventoryItems", i+1)};
			
			string prefabPath = itemSlot.m_slot.Substring(18, itemSlot.m_slot.Length() - 18);
			
			wApi.SetVariableValue(entitySource, containerPath, "TargetStorage", prefabPath);
			wApi.SetVariableValue(entitySource, containerPath, "TargetPurpose", itemSlot.storagePurpose.ToString());
			string itemsStr = "";
			for (int r = 0; r < allSlots.Get(i).m_Items.Count(); r++)
			{
				PS_LoadoutInspectorItemsList itemList = allSlots.Get(i).m_Items.Get(r);
				if (itemList.m_item == "")
					continue;
				
				for (int j = 0; j < itemList.m_count; j++)
				{
					if (itemsStr != "") itemsStr += ",";
					itemsStr += itemList.m_item;
				}
			}
			wApi.SetVariableValue(entitySource, containerPath, "PrefabsToSpawn", itemsStr);
		}
	}
	
	array<ref PS_LoadoutInspectorSlotsList> getAllSlots()
	{
		array<ref PS_LoadoutInspectorSlotsList> slots = new array<ref PS_LoadoutInspectorSlotsList>();
		if (m_vestSlots)
			for (int i = 0; i < m_vestSlots.Count(); i++)
			{
				slots.Insert(m_vestSlots.Get(i));
			}
		
		if (m_jacketItems && m_jacket != "")
		{
			PS_LoadoutInspectorSlotsList slot = new PS_LoadoutInspectorSlotsList();
			slot.m_slot = m_jacket;
			slot.m_Items = m_jacketItems;
			slot.storagePurpose = EStoragePurpose.PURPOSE_DEPOSIT;
			slots.Insert(slot);
		}
		
		if (m_pantsItems && m_pants != "")
		{
			PS_LoadoutInspectorSlotsList slot = new PS_LoadoutInspectorSlotsList();
			slot.m_slot = m_pants;
			slot.m_Items = m_pantsItems;
			slot.storagePurpose = EStoragePurpose.PURPOSE_DEPOSIT;
			slots.Insert(slot);
		}
		
		if (m_backpackItems && m_backpack != "")
		{
			PS_LoadoutInspectorSlotsList slot = new PS_LoadoutInspectorSlotsList();
			slot.m_slot = m_backpack;
			slot.m_Items = m_backpackItems;
			slot.storagePurpose = EStoragePurpose.PURPOSE_DEPOSIT;
			slots.Insert(slot);
		}
		
		return slots;
	}
	
	
	void CheckErrors()
	{
		m_errors = new array<ref PS_LoadoutInspectorError>();
		CheckInventoryCapacity(m_errors, m_jacket, m_jacketItems);
		CheckInventoryCapacity(m_errors, m_pants, m_pantsItems);
		CheckInventoryCapacity(m_errors, m_backpack, m_backpackItems);
		if (m_vestSlots)
		{
			foreach (PS_LoadoutInspectorSlotsList slotList : m_vestSlots)
			{
				CheckInventoryCapacity(m_errors, slotList.m_slot, slotList.m_Items);
			}
		}
	}
	
	void CheckInventoryCapacity(array<ref PS_LoadoutInspectorError> errors, ResourceName inventoryResource, array<ref PS_LoadoutInspectorItemsList> items)
	{
		if (items == null || items.Count() == 0)
			return;
		
		Resource slotItemHolder = BaseContainerTools.LoadContainer(inventoryResource);
		IEntitySource slotItemContainer = slotItemHolder.GetResource().ToBaseContainer().ToEntitySource();
		
		BaseContainer inventoryStorageComponent;
		int slotItemComponentsCount = slotItemContainer.GetComponentCount();
		for (int r = 0; r < slotItemComponentsCount; r++)
		{
			IEntityComponentSource componentSource = slotItemContainer.GetComponent(r);
			if (componentSource.GetClassName() == "SCR_UniversalInventoryStorageComponent")
			{
				inventoryStorageComponent = componentSource;
				break;
			}
			if (componentSource.GetClassName() == "SCR_EquipmentStorageComponent")
			{
				inventoryStorageComponent = componentSource;
				break;
			}
		}
		
		if (!inventoryStorageComponent)
		{
			AddError(errors, inventoryResource);
		}
		
		float maxVolume = 0;
		vector maxDimesions = "0 0 0";
		inventoryStorageComponent.Get("MaxCumulativeVolume", maxVolume);
		inventoryStorageComponent.Get("MaxItemSize", maxDimesions);
		
		string title = inventoryResource;
		int index = title.LastIndexOf("/");
		if (index > 0) title = title.Substring(index + 1, title.Length() - index - 4);
		
		float cumulativeVolume = 0;
		foreach (PS_LoadoutInspectorItemsList itemList : items)
		{
			if (itemList.m_item == "")
				continue;
			
			string titleItem = itemList.m_item;
			int indexItem = titleItem.LastIndexOf("/");
			if (indexItem > 0) titleItem = titleItem.Substring(indexItem + 1, titleItem.Length() - indexItem - 4);
			
			float volume = 0;
			vector dimesions = "0 0 0";
			GetItemSize(itemList.m_item, volume, dimesions);
			cumulativeVolume += volume * itemList.m_count;
			
			if (!DimensionsCheck(maxDimesions, dimesions))
			{
				AddError(errors, "D_" + title + ": " + titleItem);
			}
		}
		if (cumulativeVolume > maxVolume)
		{
			AddError(errors, "V_" + title);
		}
	}
	
	bool DimensionsCheck(vector outside, vector inside)
	{
		StaticArray.Sort(outside);
		StaticArray.Sort(inside);
		return outside[0] >= inside[0] && outside[1] >= inside[1] && outside[2] >= inside[2];
	}
	
	void AddError(array<ref PS_LoadoutInspectorError> errors, string errorStr)
	{
			PS_LoadoutInspectorError error = new PS_LoadoutInspectorError();
			error.m_error = errorStr;
			errors.Insert(error);
	}
	
	void GetItemSize(ResourceName ItemPrefab, out float volume, out vector dimesions)
	{
		BaseContainer itemComponent;
		Resource itemHolder = BaseContainerTools.LoadContainer(ItemPrefab);
		IEntitySource itemContainer = itemHolder.GetResource().ToBaseContainer().ToEntitySource();
		
		int itemComponentsCount = itemContainer.GetComponentCount();
		for (int r = 0; r < itemComponentsCount; r++)
		{
			IEntityComponentSource componentSource = itemContainer.GetComponent(r);
			if (componentSource.GetClassName() == "SCR_UniversalInventoryStorageComponent")
			{
				itemComponent = componentSource;
				break;
			}
			if (componentSource.GetClassName() == "SCR_EquipmentStorageComponent")
			{
				itemComponent = componentSource;
				break;
			}
			if (componentSource.GetClassName() == "InventoryItemComponent")
			{
				itemComponent = componentSource;
				break;
			}
			if (componentSource.GetClassName() == "InventoryMagazineComponent")
			{
				itemComponent = componentSource;
				break;
			}
		}
		
		if (!itemComponent)
		{
			volume = 9999999;
			dimesions = "999 999 999";
			return;
		}
		
		BaseContainer itemAttributes = itemComponent.GetObject("Attributes");
		BaseContainer itemPhysAttributes = itemAttributes.GetObject("ItemPhysAttributes");
		itemPhysAttributes.Get("ItemVolume", volume);
		itemPhysAttributes.Get("ItemDimensions", dimesions);
	}
	
	ResourceName GetWeaponItemPrefab(int slotIndex)
	{
		switch (slotIndex)
		{
			case 0:
				return m_primaryWeapon;
			case 1:
				return m_secondWeapon;
			case 2:
				return m_handWeapon;
			case 3:
				return m_grenade;
			case 4:
				return m_throwable;
		}
		return "";
	}
	
	
	void SetWeaponItemPrefab(int slotIndex, ResourceName ItemPrefab, BaseContainer container)
	{
		switch (slotIndex)
		{
			case 0:
				m_primaryWeapon = ItemPrefab;
				m_primaryWeaponContainer = container;
				break;
			case 1:
				m_secondWeapon = ItemPrefab;
				m_secondWeaponContainer = container;
				break;
			case 2:
				m_handWeapon = ItemPrefab;
				m_handWeaponContainer = container;
				break;
			case 3:
				m_grenade = ItemPrefab;
				m_grenadeContainer = container;
				break;
			case 4:
				m_throwable = ItemPrefab;
				m_throwableContainer = container;
				break;
		}
	}
	
	ResourceName GetItemByLoadoutArea(LoadoutAreaType areaType)
	{
		switch (areaType.Type())
		{
			case LoadoutHeadCoverArea:
				return m_helmet;
			case LoadoutJacketArea:
				return m_jacket;
			case LoadoutArmoredVestSlotArea:
				return m_armorVest;
			case LoadoutPantsArea:
				return m_pants;
			case LoadoutBootsArea:
				return m_boots;
			case LoadoutBackpackArea:
				return m_backpack;
			case LoadoutVestArea:
				return m_vest;
				break;
			case LoadoutBinocularsArea:
				return m_binoculars;
				break;
			case LoadoutWatchArea:
				return m_watch;
				break;
		}
		return "";
	}
	
	void SetLoadoutItemPrefab(LoadoutAreaType areaType, ResourceName ItemPrefab, BaseContainer container)
	{
		switch (areaType.Type())
		{
			case LoadoutHeadCoverArea:
				m_helmet = ItemPrefab;
				m_helmetContainer = container;
				break;
			case LoadoutJacketArea:
				m_jacket = ItemPrefab;
				m_jacketContainer = container;
				break;
			case LoadoutArmoredVestSlotArea:
				m_armorVest = ItemPrefab;
				m_armorVestContainer = container;
				break;
			case LoadoutPantsArea:
				m_pants = ItemPrefab;
				m_pantsContainer = container;
				break;
			case LoadoutBootsArea:
				m_boots = ItemPrefab;
				m_bootsContainer = container;
				break;
			case LoadoutBackpackArea:
				m_backpack = ItemPrefab;
				m_backpackContainer = container;
				break;
			case LoadoutVestArea:
				m_vest = ItemPrefab;
				m_vestContainer = container;
				m_vestSlots = ExtractSlots(m_vest);
				break;
			case LoadoutBinocularsArea:
				m_binoculars = ItemPrefab;
				m_binocularsContainer = container;
				break;
			case LoadoutWatchArea:
				m_watch = ItemPrefab;
				m_watchContainer = container;
				break;
		}
	}
	
	ref array<ref PS_LoadoutInspectorSlotsList> ExtractSlots(ResourceName slotsHolder)
	{
		Resource holder = BaseContainerTools.LoadContainer(slotsHolder);
		IEntitySource container = holder.GetResource().ToBaseContainer().ToEntitySource();
		
		// Get BaseLoadoutClothComponent
		int componentsCount = container.GetComponentCount();
		BaseContainer baseLoadoutClothComponent;
		for (int i = 0; i < componentsCount; i++)
		{
			IEntityComponentSource componentSource = container.GetComponent(i);
			if (componentSource.GetClassName() == "BaseLoadoutClothComponent")
			{
				baseLoadoutClothComponent = componentSource;
				break;
			}
		}
		
		if (!baseLoadoutClothComponent)
			return null;
		
		array<ref PS_LoadoutInspectorSlotsList> slotsList = new array<ref PS_LoadoutInspectorSlotsList>();
		
		// Foreach slot
		BaseContainerList slots = baseLoadoutClothComponent.GetObjectArray("Slots");
		for (int s = 0, slotCount = slots.Count(); s < slotCount; s++)
		{
			BaseContainer slotContainer = slots.Get(s);
			ResourceName ItemPrefab = "";
			slotContainer.Get("Prefab", ItemPrefab);
			
			Resource slotItemHolder = BaseContainerTools.LoadContainer(ItemPrefab);
			IEntitySource slotItemContainer = slotItemHolder.GetResource().ToBaseContainer().ToEntitySource();
			
			// Get SCR_UniversalInventoryStorageComponent
			BaseContainer inventoryStorageComponent;
			int slotItemComponentsCount = slotItemContainer.GetComponentCount();
			EStoragePurpose storagePurpose = EStoragePurpose.PURPOSE_ANY;
			for (int r = 0; r < slotItemComponentsCount; r++)
			{
				IEntityComponentSource componentSource = slotItemContainer.GetComponent(r);
				if (componentSource.GetClassName() == "SCR_UniversalInventoryStorageComponent")
				{
					storagePurpose = EStoragePurpose.PURPOSE_DEPOSIT;
					inventoryStorageComponent = componentSource;
					break;
				}
				if (componentSource.GetClassName() == "SCR_EquipmentStorageComponent")
				{
					storagePurpose = EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT;
					inventoryStorageComponent = componentSource;
					break;
				}
			}
			
			if (!inventoryStorageComponent)
				continue;
			
			
			PS_LoadoutInspectorSlotsList slotList = new PS_LoadoutInspectorSlotsList();
			slotList.m_slot = ItemPrefab;
			slotList.storagePurpose = storagePurpose;
			slotsList.Insert(slotList);
		}
		return slotsList;
	}
	
	void SetItemsContainer(string prefabPath, EStoragePurpose storagePurpose, array<ResourceName> items)
	{
		if (m_pants.Contains(prefabPath))
		{
			m_pantsItems = GetItemsList(items);
		}
		if (m_jacket.Contains(prefabPath))
		{
			m_jacketItems = GetItemsList(items);
		}
		if (m_backpack.Contains(prefabPath))
		{
			m_backpackItems = GetItemsList(items);
		}
		foreach (PS_LoadoutInspectorSlotsList slotList : m_vestSlots)
		{
			if (slotList.m_Items != null)
				continue;
			if (slotList.m_slot.Contains(prefabPath))
			{
				slotList.m_Items = GetItemsList(items);
				break;
			}
		}
	}
	
	array<ref PS_LoadoutInspectorItemsList> GetItemsList(array<ResourceName> items)
	{
		array<ref PS_LoadoutInspectorItemsList> itemLists = new array<ref PS_LoadoutInspectorItemsList>();
		map<ResourceName, PS_LoadoutInspectorItemsList> itemsMap = new map<ResourceName, PS_LoadoutInspectorItemsList>();
		foreach (ResourceName item : items)
		{
			if (itemsMap.Contains(item)) itemsMap.Get(item).m_count++;
			else {
				PS_LoadoutInspectorItemsList itemList = new PS_LoadoutInspectorItemsList();
				itemList.m_item = item;
				itemList.m_count = 1;
				itemsMap.Set(item, itemList);
				itemLists.Insert(itemList);
			}
		}
		return itemLists;
	}
}

class PS_LoadoutInspectorErrorCustomTitle: BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		string error;
		source.Get("m_error", error);
		title = error;
		return true;
	}
}

[BaseContainerProps(), PS_LoadoutInspectorErrorCustomTitle()]
class PS_LoadoutInspectorError
{
	[Attribute("", UIWidgets.Hidden, "et")]
	string m_error;
}

class PS_LoadoutInspectorItemsListCustomTitle: BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		ResourceName item;
		source.Get("m_item", item);
		int count;
		source.Get("m_count", count);
		title = item;
		int index = title.LastIndexOf("/");
		if (index > 0) title = title.Substring(index + 1, title.Length() - index - 4);
		title = count.ToString() + "x" + title;
		return true;
	}
}

[BaseContainerProps(), PS_LoadoutInspectorItemsListCustomTitle()]
class PS_LoadoutInspectorItemsList
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "et")]
	ResourceName m_item;
	
	[Attribute("1", UIWidgets.Auto)]
	int m_count;
}

class PS_LoadoutInspectorSlotsListCustomTitle: BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		ResourceName slot;
		source.Get("m_slot", slot);
		
		BaseContainerList itemLists = source.GetObjectArray("m_Items");
		int allItemsCount = 0;
		int count = itemLists.Count();
		for (int i = 0; i < count; i++)
		{
			BaseContainer itemListContainer = itemLists.Get(i);
			int itemsCount;
			itemListContainer.Get("m_count", itemsCount);
			allItemsCount += itemsCount;
		}
		
		title = slot;
		int index = title.LastIndexOf("/");
		if (index > 0) title = title.Substring(index + 1, title.Length() - index - 4);
		
		title = allItemsCount.ToString() + "x" + title;
		return true;
	}
}
[BaseContainerProps(), PS_LoadoutInspectorSlotsListCustomTitle()]
class PS_LoadoutInspectorSlotsList
{
	[Attribute("", UIWidgets.Hidden, "et")]
	ResourceName m_slot;
	
	[Attribute("0", UIWidgets.Hidden, "", "", ParamEnumArray.FromEnum(EStoragePurpose))]
	EStoragePurpose storagePurpose;
	
	[Attribute("1", UIWidgets.Auto)]
	ref array<ref PS_LoadoutInspectorItemsList> m_Items;
}