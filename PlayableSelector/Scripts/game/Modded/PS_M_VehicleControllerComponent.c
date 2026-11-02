modded class VehicleControllerComponent
{
	protected bool PS_IsPilotOutFreezeZone()
	{
		PilotCompartmentSlot pilotSlot = GetPilotCompartmentSlot();
		IEntity occupant = null;
		if (pilotSlot)
			occupant = pilotSlot.GetOccupant();

		return SCR_GetOutAction.PS_IsUserOutFreezeZone(occupant);
	}

	override bool OnBeforeEngineStart()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
		{
			if (freezeManager.IsHardFreezeActive() && !PS_VehicleHold.IsStartingHeldEngine())
				return false;

			if (!freezeManager.IsFreezeTimeEnd())
			{
				Vehicle vehicle = Vehicle.Cast(GetOwner());
				if (!vehicle || !vehicle.IsEnableMoveOnFreeze())
					return false;

				if (PS_IsPilotOutFreezeZone())
					return false;
			}
		}

		return super.OnBeforeEngineStart();
	}
}

modded class SCR_TurretControllerComponent
{
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		bool freezeLocked = freezeManager && (freezeManager.IsFreezeSafetyActive() || freezeManager.IsHardFreezeActive());

		if (freezeLocked)
		{
			if (am)
			{
				am.SetActionValue("TurretFire", 0);
				am.SetActionValue("CharacterFireStatic", 0);
				am.SetActionValue("VehicleFire", 0);
			}
			SetFireWeaponWanted(false);
		}

		super.OnPrepareControls(owner, am, dt, player);

		if (freezeLocked)
			SetFireWeaponWanted(false);
	}
}
