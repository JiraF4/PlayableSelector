modded class Vehicle 
{
	static ref array<Vehicle> m_aVehicles_PS = {};
	
	[Attribute()]
	string m_sAttachmentGroupName;
	
	[Attribute()]
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
		if (vehicleWheeledSimulation)
		{
			vehicleWheeledSimulation.SetBreak(1, 1);
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(m_sAttachmentGroupName));
			if (group)
			{
				RplId id = Replication.FindId(this);
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
			RplId id = Replication.FindId(this);
			playableManager.UnRegisterGroupVehicle(id);
		}
	}
}