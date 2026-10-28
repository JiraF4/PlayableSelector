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

		if (freezeManager && !freezeManager.IsFreezeTimeEnd())
		{
			Vehicle vehicle = Vehicle.Cast(GetOwner().GetRootParent());
			if (vehicle && !vehicle.IsEnableMoveOnFreeze())
			{
				SetCannotPerformReason("#PS-Freeze_Time");
				return false;
			}
		}

		return super.CanBePerformedScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!CanBePerformedScript(pUserEntity))
			return;

		super.PerformAction(pOwnerEntity, pUserEntity);
	}
}
