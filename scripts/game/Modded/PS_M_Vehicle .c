modded class Vehicle 
{
	static ref array<Vehicle> m_aVehicles_PS = {};
	
	[Attribute()]
	string m_sAttachmentGroupName;
	
	void Vehicle(IEntitySource src, IEntity parent)
	{
		m_aVehicles_PS.Insert(this);
		if (GetGame().GetCallqueue())
			GetGame().GetCallqueue().Call(RegisterToMissionDate);
	}
	
	void RegisterToMissionDate()
	{
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