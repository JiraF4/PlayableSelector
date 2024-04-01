modded class Vehicle 
{
	static ref array<Vehicle> m_aVehicles_PS = {};
	
	void Vehicle(IEntitySource src, IEntity parent)
	{
		m_aVehicles_PS.Insert(this);
		if (GetGame().GetCallqueue())
			GetGame().GetCallqueue().Call(RegisterToMissionDate);
	}
	
	void RegisterToMissionDate()
	{
		PS_MissionDataManager missionDataManager = PS_MissionDataManager.GetInstance();
		missionDataManager.RegisterVehicle(this);
	}
	
	void ~Vehicle()
	{
		m_aVehicles_PS.RemoveItem(this);
	}
}