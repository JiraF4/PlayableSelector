modded class SCR_GadgetManagerComponent
{
	override protected void OnGadgetInput(float value, EActionTrigger reason)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}

		if (GetGadgetInputAction() == EGadgetType.MAP && PS_TrackerComponent.BlocksMapWhileHeld(GetHeldGadget()))
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}

		super.OnGadgetInput(value, reason);
	}
};
