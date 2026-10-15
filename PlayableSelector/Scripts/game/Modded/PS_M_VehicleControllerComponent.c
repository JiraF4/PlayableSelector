modded class VehicleControllerComponent
{
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
			}
		}

		return super.OnBeforeEngineStart();
	}
}
