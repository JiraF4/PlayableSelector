modded class SCR_ToggleHandbrakeAction
{
	override bool CanBePerformedScript(IEntity user)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			SetCannotPerformReason("#PS-HardFreeze_Title");
			return false;
		}

		return super.CanBePerformedScript(user);
	}
}
