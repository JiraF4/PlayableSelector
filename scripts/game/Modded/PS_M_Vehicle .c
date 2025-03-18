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
	
	void Freeze()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if ((gameMode.GetState() == SCR_EGameModeState.GAME && gameMode.IsFreezeTimeEnd()) || !gameMode.IsFreezeTimeShootingForbiden())
		{
			GetGame().GetCallqueue().Remove(Freeze);
			return;
		}
		
		if (!GetPhysics())
			return;
		
		//GetPhysics().SetVelocity("0 0 0");
		//GetPhysics().SetAngularVelocity("0 0 0");
	}
	
	void RegisterToMissionDate()
	{
		if (!m_bEnableMoveOnFreeze)
			GetGame().GetCallqueue().CallLater(Freeze, 0, true);
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