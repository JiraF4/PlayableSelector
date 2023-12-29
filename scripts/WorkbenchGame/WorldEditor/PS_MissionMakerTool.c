[WorkbenchToolAttribute("Mission maker Tool", "", "", awesomeFontCode: 0xf5ee)]
class PS_MissionMakerTool: WorldEditorTool
{
	[Attribute()]
	ref PS_LoadoutInspector m_inventoryInspector;
	
	[ButtonAttribute("Inventory apply")]
	void InventoryApply()
	{
		if (!m_inventoryInspector)
			return;
		
		if (m_API.GetSelectedEntitiesCount() != 1)
			return;
		
		IEntity entity = m_API.GetSelectedEntity();
		IEntitySource entitySource = m_API.EntityToSource(entity);
		
		m_inventoryInspector.SetItems(m_API, entitySource);
	}
		
	[ButtonAttribute("Inventory errors")]
	void InventoryErrors()
	{
		if (m_inventoryInspector)
			m_inventoryInspector.CheckErrors();
		UpdatePropertyPanel();
	}
	
	[ButtonAttribute("Inventory inspector")]
	void InventoryInspector()
	{
		if (m_API.GetSelectedEntitiesCount() != 1)
			return;
		
		IEntity entity = m_API.GetSelectedEntity();
		IEntitySource entitySource = m_API.EntityToSource(entity);
		
		IEntityComponentSource baseLoadoutManagerComponent = null;
		array<IEntityComponentSource> weaponSlots = new array<IEntityComponentSource>();
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
				int SlotIndex;
				componentSource.Get("WeaponSlotIndex", SlotIndex);
				weaponSlots.InsertAt(componentSource, SlotIndex);
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
		
		m_inventoryInspector = new PS_LoadoutInspector();
		m_inventoryInspector.m_entitySource = entitySource;
		
		BaseContainerList slots = baseLoadoutManagerComponent.GetObjectArray("Slots");
		for (int s = 0, slotCount = slots.Count(); s < slotCount; s++)
		{
			BaseContainer slotContainer = slots.Get(s);
			ResourceName ItemPrefab = "";
			slotContainer.Get("Prefab", ItemPrefab);
			LoadoutAreaType areaType;
			slotContainer.Get("AreaType", areaType);
			m_inventoryInspector.SetLoadoutItemPrefab(areaType, ItemPrefab, slotContainer);
		}
		BaseContainerList slotsEquipment = equipmentStorageComponent.GetObjectArray("InitialStorageSlots");
		if (true) {
			ResourceName ItemPrefab = "";
			slotsEquipment.Get(0).Get("Prefab", ItemPrefab);
			m_inventoryInspector.SetLoadoutItemPrefab(new LoadoutBinocularsArea(), ItemPrefab, slotsEquipment.Get(0));
		}
		if (true) {
			ResourceName ItemPrefab = "";
			slotsEquipment.Get(1).Get("Prefab", ItemPrefab);
			m_inventoryInspector.SetLoadoutItemPrefab(new LoadoutWatchArea(), ItemPrefab, slotsEquipment.Get(1));
		}
		foreach(IEntityComponentSource componentSource : weaponSlots)
		{
			int slotIndex;
			componentSource.Get("WeaponSlotIndex", slotIndex);
			ResourceName itemPrefab = "";
			componentSource.Get("WeaponTemplate", itemPrefab);
			m_inventoryInspector.SetWeaponItemPrefab(slotIndex, itemPrefab, componentSource);
		}
		
		BaseContainerList items = inventoryStorageManagerComponent.GetObjectArray("InitialInventoryItems");
		for (int s = 0, itemsCount = items.Count(); s < itemsCount; s++)
		{
			BaseContainer itemsContainer = items.Get(s);
			DataVarType type = itemsContainer.GetDataVarType(itemsContainer.GetVarIndex("TargetStorage"));
			string prefabPath;
			itemsContainer.Get("TargetStorage", prefabPath);
			EStoragePurpose targetPurpose;
			itemsContainer.Get("TargetPurpose", targetPurpose);
			array<ResourceName> itemsList;
			itemsContainer.Get("PrefabsToSpawn", itemsList);
			m_inventoryInspector.SetItemsContainer(prefabPath, targetPurpose, itemsList);
		}
		
		
		
		UpdatePropertyPanel();
	}
	
	// Groups
	[ButtonAttribute("Assign callsigns")]
	void AssignCallsigns()
	{
		int selectedCount = m_API.GetSelectedEntitiesCount();
		if (selectedCount == 0) return;
		
		m_API.BeginEntityAction("Assign callsigns");
		for (int i = 0; i < selectedCount; i++)
		{
			IEntity entity = m_API.GetSelectedEntity(i);
			AssignCallsign(entity);
		}
		m_API.EndEntityAction();
	}
	
	void GetNextFreeCallSign(IEntity excludeEntity, Faction factionCurrent, out bool currentAssigned, out int companyCallsignIndex, out int platoonCallsignIndex, out int squadCallsignIndex)
	{
		array<int> assignedCallSigns = new array<int>();
		int count = m_API.GetEditorEntityCount();
		for (int i = 0; i < count; i++)
		{
			IEntitySource entitySource = m_API.GetEditorEntity(i);
			if (entitySource.GetClassName() == "SCR_AIGroup")
			{
				IEntity entity = m_API.SourceToEntity(entitySource);
				if (!entity)
					continue;
				if (entity == excludeEntity)
					continue;
				
				SCR_AIGroup group = SCR_AIGroup.Cast(entity);
				if (!group)
					continue;
				
				SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
				if (faction != factionCurrent)
					continue;
				
				IEntityComponentSource componentSourceCallsignAssigner = null;
				int componentsCount = entitySource.GetComponentCount();
				for (int r = 0; r < componentsCount; r++)
				{
					IEntityComponentSource componentSource = entitySource.GetComponent(r);
					if (componentSource.GetClassName() == "PS_GroupCallsignAssigner")
					{
						componentSourceCallsignAssigner = componentSource;
					}
				}
				
				if (!componentSourceCallsignAssigner)
					continue;
				
				int companyCallsignIndexTemp, platoonCallsignIndexTemp, squadCallsignIndexTemp;
				componentSourceCallsignAssigner.Get("m_iCompanyCallsign", companyCallsignIndexTemp);
				componentSourceCallsignAssigner.Get("m_iPlatoonCallsign", platoonCallsignIndexTemp);
				componentSourceCallsignAssigner.Get("m_iSquadCallsign", squadCallsignIndexTemp);
				
				assignedCallSigns.Insert(
					companyCallsignIndexTemp * 100 +
					platoonCallsignIndexTemp * 10 +
					squadCallsignIndexTemp
				);
			}
		}
			
		int callsign = companyCallsignIndex * 100 +
				platoonCallsignIndex * 10 +
				squadCallsignIndex;
		while (assignedCallSigns.Contains(callsign))
		{
			currentAssigned = true;
			squadCallsignIndex++;
			if (squadCallsignIndex > 2)
			{
				platoonCallsignIndex++;
				if (platoonCallsignIndex > 2)
				{
					companyCallsignIndex++;
					platoonCallsignIndex = 0;
				}
				squadCallsignIndex = 0;
			}
			callsign = companyCallsignIndex * 100 +
				platoonCallsignIndex * 10 +
				squadCallsignIndex;
		}
	}
	
	void AssignCallsign(IEntity entity)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
			return;
		SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
		if (!faction)
			return;
		
		IEntitySource entitySource = m_API.EntityToSource(entity);
		IEntityComponentSource componentSourceCallsignAssigner = null;
		int componentsCount = entitySource.GetComponentCount();
		for (int i = 0; i < componentsCount; i++)
		{
			IEntityComponentSource componentSource = entitySource.GetComponent(i);
			if (componentSource.GetClassName() == "PS_GroupCallsignAssigner")
			{
				componentSourceCallsignAssigner = componentSource;
			}
		}
		if (!componentSourceCallsignAssigner)
			componentSourceCallsignAssigner = m_API.CreateComponent(entitySource, "PS_GroupCallsignAssigner");
		
		int companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex;
		componentSourceCallsignAssigner.Get("m_iCompanyCallsign", companyCallsignIndex);
		componentSourceCallsignAssigner.Get("m_iPlatoonCallsign", platoonCallsignIndex);
		componentSourceCallsignAssigner.Get("m_iSquadCallsign", squadCallsignIndex);
		bool currentAssigned = false;
		GetNextFreeCallSign(entity, faction, currentAssigned, companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex);
		if (currentAssigned)
		{
			componentSourceCallsignAssigner.Set("m_iCompanyCallsign", companyCallsignIndex);
			componentSourceCallsignAssigner.Set("m_iPlatoonCallsign", platoonCallsignIndex);
			componentSourceCallsignAssigner.Set("m_iSquadCallsign", squadCallsignIndex);
		}
		
		array<ResourceName> outPrefabs = new array<ResourceName>();
		entitySource.Get("m_aUnitPrefabSlots", outPrefabs);
		string customName;
		entitySource.Get("m_sCustomNameSet", customName);
		if (customName != "") customName = "(" + customName + ") ";
		
		string entityName = string.Format("%5%1_G_%2%3%4_%6", faction.GetFactionKey(), companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex, customName, outPrefabs.Count());
		if (!m_API.FindEntityByName(entityName))
			m_API.RenameEntity(entity, entityName);
	}
	
	// Markers
	[Attribute("", UIWidgets.Auto)]
	ResourceName m_MarkerMakerConfigPath;
	PS_MarkerMakerConfig m_MarkerMakerConfig;
	
	[ButtonAttribute("Create marker")]
	void CreateMarker()
	{
		Managed m = SCR_BaseContainerTools.CreateInstanceFromPrefab(m_MarkerMakerConfigPath);
		m_MarkerMakerConfig = PS_MarkerMakerConfig.Cast(m);
		
		int selectedCount = m_API.GetSelectedEntitiesCount();
		if (selectedCount == 0) return;
		
		m_API.BeginEntityAction("Create marker");
		for (int i = 0; i < selectedCount; i++)
		{
			IEntity entity = m_API.GetSelectedEntity(i);
			IEntitySource entitySource = m_API.EntityToSource(entity);
			
			vector origin = entity.GetOrigin();
			int currentLayerId = m_API.GetCurrentEntityLayerId();
			
			PS_ManualMarkerConfig markerConfig = m_MarkerMakerConfig.m_mManualMarkerConfig;
			
			string entityName;
			vector rotation = "0 90 0";
			
			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
			{
				PS_GroupCallsignAssigner callSignAssigner = PS_GroupCallsignAssigner.Cast(group.FindComponent(PS_GroupCallsignAssigner));
				if (callSignAssigner)
				{
					SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
					foreach (PS_FactionManualMarkerConfig factionManualMarkerConfig : m_MarkerMakerConfig.m_mFactionsManualMarkerConfig)
					{
						if (factionManualMarkerConfig.m_sFactionKey == faction.GetFactionKey())
						{
							markerConfig = factionManualMarkerConfig.m_mManualMarkerConfig;
							break;
						}
					}
					
					SCR_FactionCallsignInfo callsignInfo = faction.GetCallsignInfo();
					
					int companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex;
					callSignAssigner.GetCallsign(companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex);
					string company = callsignInfo.GetCompanyCallsignName(companyCallsignIndex);
					string platoon = callsignInfo.GetPlatoonCallsignName(platoonCallsignIndex);
					string squad   = callsignInfo.GetSquadCallsignName(squadCallsignIndex);
					string format  = callsignInfo.GetCallsignFormat(false);	
					
					markerConfig.m_sDescription = WidgetManager.Translate(format, company, platoon, squad, "");
					entityName = string.Format("%1_G_%2%3%4_M", faction.GetFactionKey(), companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex);
				}
			}
			
			Vehicle vehicle = Vehicle.Cast(entity);
			if (vehicle)
			{
				EVehicleType vehicleType = vehicle.m_eVehicleType;
				
				foreach (PS_VehicleManualMarkerConfig vehicleManualMarkerConfig : m_MarkerMakerConfig.m_mVehiclesManualMarkerConfig)
				{
					if (vehicleManualMarkerConfig.m_iVehicleType == vehicleType)
					{
						markerConfig = vehicleManualMarkerConfig.m_mManualMarkerConfig;
						break;
					}
				}
				
				SCR_VehicleFactionAffiliationComponent vehicleFaction = SCR_VehicleFactionAffiliationComponent.Cast(vehicle.FindComponent(SCR_VehicleFactionAffiliationComponent));
				SCR_EditableVehicleComponent editableVehicle = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
				
				SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				SCR_Faction faction = SCR_Faction.Cast(vehicleFaction.GetDefaultAffiliatedFaction());
				if (!faction) faction = SCR_Faction.Cast(vehicleFaction.GetDefaultAffiliatedFaction());
				SCR_UIInfo uiInfo = editableVehicle.GetInfo();
				
				rotation = vehicle.GetAngles() + Vector(0, 90, 0);
				if (faction) markerConfig.m_MarkerColor = faction.GetFactionColor();
				else markerConfig.m_MarkerColor = new Color(1, 1, 1, 1);
				
				vector boundMin;
				vector boundMax;
				vehicle.GetBounds(boundMin, boundMax);
				float size = boundMax[0] - boundMin[0];
				float sizeZ = boundMax[2] - boundMin[2];
				if (size < sizeZ) size = sizeZ;
				markerConfig.m_fWorldSize = size/2;
				markerConfig.m_sDescription = uiInfo.GetName();
			}
			
			IEntity markerEntity = m_API.CreateEntity(m_MarkerMakerConfig.m_sManualMarkerPrefab, entityName, currentLayerId, null, origin, rotation);
			IEntitySource markerEntitySource = m_API.EntityToSource(markerEntity);
			
			SetMarkerData(markerEntitySource, markerConfig);
		}
		m_API.EndEntityAction();
	}
	
	void SetMarkerData(IEntitySource markerEntitySource, PS_ManualMarkerConfig markerConfig)
	{
		m_API.SetVariableValue(markerEntitySource, null, "m_sImageSet", markerConfig.m_sImageSet);
		m_API.SetVariableValue(markerEntitySource, null, "m_sImageSetGlow", markerConfig.m_sImageSetGlow);
		string argb = markerConfig.m_MarkerColor.R().ToString() + " " + markerConfig.m_MarkerColor.G().ToString() + " " + markerConfig.m_MarkerColor.B().ToString() + " " + markerConfig.m_MarkerColor.A().ToString();
		m_API.SetVariableValue(markerEntitySource, null, "m_MarkerColor", argb);
		m_API.SetVariableValue(markerEntitySource, null, "m_sQuadName", markerConfig.m_sQuadName);
		m_API.SetVariableValue(markerEntitySource, null, "m_fWorldSize", markerConfig.m_fWorldSize.ToString());
		m_API.SetVariableValue(markerEntitySource, null, "m_sDescription", markerConfig.m_sDescription);
		if (markerConfig.m_bUseWorldScale)
			m_API.SetVariableValue(markerEntitySource, null, "m_bUseWorldScale", "1");
		else
			m_API.SetVariableValue(markerEntitySource, null, "m_bUseWorldScale", "0");
		string factionsStr = "";
		foreach (FactionKey factionKey : markerConfig.m_aVisibleForFactions)
		{
			if (factionsStr != "") factionsStr += ", ";
			factionsStr += factionKey;
		}
		m_API.SetVariableValue(markerEntitySource, null, "m_aVisibleForFactions", factionsStr);
		if (markerConfig.m_bVisibleForEmptyFaction)
			m_API.SetVariableValue(markerEntitySource, null, "m_bVisibleForEmptyFaction", "1");
		else
			m_API.SetVariableValue(markerEntitySource, null, "m_bVisibleForEmptyFaction", "0");
	}
	
	
};
























