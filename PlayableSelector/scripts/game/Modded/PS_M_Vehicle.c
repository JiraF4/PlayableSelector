/**
 * @brief Модификация Vehicle для регистрации транспорта в PlayableSelector и поддержки заморозки.
 * @subsystem Core / Vehicles
 * @context Hybrid
 * @entity Vehicle
 * @depends PlayableSelector
 * @listens None
 * @details Ведет глобальный реестр m_aVehicles_PS, регистрирует транспорт в PlayableManager и MissionDataManager.
 *          Физическая заморозка и стояночный тормоз управляются централизованно через PS_GameModeCoop.
 */
modded class Vehicle 
{
	static ref array<Vehicle> m_aVehicles_PS = {};
	
	[Attribute()]
	string m_sAttachmentGroupName;
	
	[RplProp(), Attribute()]
	protected bool m_bEnableMoveOnFreeze;
	
	bool IsEnableMoveOnFreeze()
	{
		return m_bEnableMoveOnFreeze;
	}
	
	void Vehicle(IEntitySource src, IEntity parent)
	{
		m_aVehicles_PS.Insert(this);
		if (GetGame().GetCallqueue())
			GetGame().GetCallqueue().Call(RegisterToMissionDate);
	}
	
	void RegisterToMissionDate()
	{
		VehicleWheeledSimulation vehicleWheeledSimulation = VehicleWheeledSimulation.Cast(FindComponent(VehicleWheeledSimulation));
		if (vehicleWheeledSimulation && !m_bEnableMoveOnFreeze)
		{
			vehicleWheeledSimulation.SetBreak(1, true);
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(m_sAttachmentGroupName));
			if (group)
			{
				RplId id = Replication.FindItemId(this);
				playableManager.RegisterGroupVehicle(id, group, this);
			}
		}
		PS_MissionDataManager missionDataManager = PS_MissionDataManager.GetInstance();
		if (!missionDataManager)
			return;
		missionDataManager.RegisterVehicle(this);
	}
	
	void ~Vehicle()
	{
		m_aVehicles_PS.RemoveItem(this);
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			RplId id = Replication.FindItemId(this);
			playableManager.UnRegisterGroupVehicle(id);
		}
	}
}
