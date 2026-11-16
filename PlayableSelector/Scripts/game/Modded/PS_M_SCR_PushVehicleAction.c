modded class SCR_PushVehicleAction
{
	protected bool PS_IsPushBlocked(IEntity user)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager)
			return false;

		if (freezeManager.IsHardFreezeActive())
		{
			SetCannotPerformReason("#PS-HardFreeze_Title");
			return true;
		}

		if (!freezeManager.IsFreezeTimeEnd())
		{
			Vehicle vehicle = Vehicle.Cast(GetOwner().GetRootParent());
			if (vehicle && (!vehicle.IsEnableMoveOnFreeze() || SCR_GetOutAction.PS_IsUserOutFreezeZone(user)))
			{
				SetCannotPerformReason("#PS-Freeze_Time");
				return true;
			}
		}

		return false;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (PS_IsPushBlocked(user))
			return false;

		return super.CanBePerformedScript(user);
	}

	override void OnActionStart(IEntity pUserEntity)
	{
		if (PS_IsPushBlocked(pUserEntity))
			return;

		super.OnActionStart(pUserEntity);
	}

	// Unflip overrides PerformContinuousAction without super, so the guard lives in the shared force path.
	override protected void ApplyForce(IEntity rootEntity, IEntity pUserEntity, float timeSlice, vector forceOffset = vector.Zero)
	{
		if (PS_IsPushBlocked(pUserEntity))
			return;

		super.ApplyForce(rootEntity, pUserEntity, timeSlice, forceOffset);
	}
}