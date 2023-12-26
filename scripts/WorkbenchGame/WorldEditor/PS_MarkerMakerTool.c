[WorkbenchToolAttribute("Marker maker Tool", "Place markers for entities", "", awesomeFontCode: 0xf5ee)]
class PS_MarkerMakerTool: WorldEditorTool
{
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
























