modded class VehicleControllerComponent
{
	override bool OnBeforeEngineStart()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive() && !PS_VehicleHold.IsStartingHeldEngine())
			return false;

		return super.OnBeforeEngineStart();
	}
}
