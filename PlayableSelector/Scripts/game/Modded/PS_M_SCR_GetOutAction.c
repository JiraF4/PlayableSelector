modded class SCR_GetOutAction
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

modded class SCR_JumpOutAction
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
