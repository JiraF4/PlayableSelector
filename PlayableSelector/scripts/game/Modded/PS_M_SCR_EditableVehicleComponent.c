modded class SCR_EditableVehicleComponent
{
	RplId m_iRemoveRpl;
	void ~SCR_EditableVehicleComponent()
	{
		if (!GetGame().InPlayMode())
			return;

		if (!GetGame().GetWorld())
			return;

		if (!Replication.IsServer())
			return;

		BaseGameMode gamemode = GetGame().GetGameMode();
		if (!gamemode)
			return;

		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter && m_iRemoveRpl)
			replayWriter.WriteEntityDelete(m_iRemoveRpl);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;
		GetGame().GetCallqueue().CallLater(RegisterToReplay, 0, false, owner);
	}

	void RegisterToReplay(IEntity owner)
	{
		SCR_UIInfo uIInfo = GetInfo();
		Vehicle vehicle = Vehicle.Cast(owner);
		RplComponent Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (!replayWriter || !Rpl)
			return;
		RplId rplId = Rpl.Id();
		SCR_VehicleFactionAffiliationComponent factionComponent = SCR_VehicleFactionAffiliationComponent.Cast(owner.FindComponent(SCR_VehicleFactionAffiliationComponent));
		Faction faction;
		FactionKey factionKey = "";
		if (factionComponent)
		{
			faction = factionComponent.GetDefaultAffiliatedFaction();
			if (faction) factionKey = faction.GetFactionKey();
		}
		string name = "";
		if (uIInfo) name = uIInfo.GetName();
		if (vehicle)
			replayWriter.WriteVehicleRegistration(rplId, name, vehicle.m_eVehicleType, factionKey);

		m_iRemoveRpl = Rpl.Id();

		GetGame().GetCallqueue().CallLater(PositionLogger, 100, false, rplId, owner);
	}

	protected void PositionLogger(RplId rplId, IEntity owner)
	{
		// Entity may have been destroyed between CallLater invocations.
		if (!owner)
			return; // stop rescheduling — entity is gone
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter)
			replayWriter.WriteEntityMove(rplId, owner);
		GetGame().GetCallqueue().CallLater(PositionLogger, 500, false, rplId, owner);
	}
}
