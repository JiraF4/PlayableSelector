modded class SCR_GetInUserAction
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

modded class SCR_SwitchSeatAction
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

modded class SCR_ChangeComparmentAction
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
