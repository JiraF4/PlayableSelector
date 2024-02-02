class PS_DebriefingLossesWidgetComponent : SCR_ScriptedWidgetComponent
{
	ImageWidget m_wUnitTypeImage;
	TextWidget m_wUnitName;
	
	TextWidget m_wDestroyed;
	TextWidget m_wVehicleDamaged;
	TextWidget m_wCharacterDamaged;
	TextWidget m_wUndamaged;
	
	ref array<IEntity> m_aEntities = {};
	
	int m_iVehiclesFilter;
	typename m_UnitType;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wUnitTypeImage = ImageWidget.Cast(w.FindAnyWidget("UnitTypeImage"));
		m_wUnitName = TextWidget.Cast(w.FindAnyWidget("UnitName"));
		
		m_wDestroyed = TextWidget.Cast(w.FindAnyWidget("Destroyed"));
		m_wVehicleDamaged = TextWidget.Cast(w.FindAnyWidget("VehicleDamaged"));
		m_wCharacterDamaged = TextWidget.Cast(w.FindAnyWidget("CharacterDamaged"));
		m_wUndamaged = TextWidget.Cast(w.FindAnyWidget("Undamaged"));
	}
	
	void SetUnitName(string unitName)
	{
		m_wUnitName.SetText(unitName);
	}
	
	void SetUnitType(typename unitType, Faction faction, int vehicleTypes)
	{
		m_iVehiclesFilter = vehicleTypes;
		m_UnitType = unitType;
		int ok, medC, medV, des;
		//GetGame().GetWorld().GetActiveEntities(entities);
		GetGame().GetWorld().QueryEntitiesBySphere("0 0 0", 10000000000, QueryEntities);
		foreach (IEntity entity : m_aEntities)
		{
			ScriptedDamageManagerComponent damageManagerComponent = ScriptedDamageManagerComponent.Cast(entity.FindComponent(ScriptedDamageManagerComponent));
			if (!damageManagerComponent)
				continue;
			
			FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (!factionAffiliationComponent)
				continue;
			
			if (factionAffiliationComponent.GetDefaultAffiliatedFaction() != faction)
				continue;
			
			EDamageState damageState = damageManagerComponent.GetState();
			switch (damageState)
			{
				case EDamageState.UNDAMAGED:
					ok++;
					break;
				case EDamageState.INTERMEDIARY:
				case EDamageState.STATE3:
				case EDamageState.STATE2:
				case EDamageState.STATE1:
					if (entity.IsInherited(SCR_ChimeraCharacter))
						medC++;
					else
						medV++;
					break;
				case EDamageState.DESTROYED:
					des++;
					break;
			}
		}
		
		m_wDestroyed.SetText(des.ToString());
		m_wVehicleDamaged.SetText(medV.ToString());
		m_wCharacterDamaged.SetText(medC.ToString());
		m_wUndamaged.SetText(ok.ToString());
	}
	
	private bool QueryEntities(IEntity e)
	{
		if (e.IsInherited(m_UnitType))
		{
			if (e.IsInherited(Vehicle))
			{
				Vehicle vehicle = Vehicle.Cast(e);
				if (m_iVehiclesFilter == 0 && vehicle.m_eVehicleType == 0)
					m_aEntities.Insert(e);
				else if (vehicle.m_eVehicleType & m_iVehiclesFilter)
					m_aEntities.Insert(e);
			}
			else
				m_aEntities.Insert(e);
		}
		return true;
	}
}
