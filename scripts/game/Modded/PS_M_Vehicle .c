modded class Vehicle 
{
	static ref array<Vehicle> m_aVehicles_PS = {};
	
	void Vehicle(IEntitySource src, IEntity parent)
	{
		m_aVehicles_PS.Insert(this);
	}
	
	void ~Vehicle()
	{
		m_aVehicles_PS.RemoveItem(this);
	}
}