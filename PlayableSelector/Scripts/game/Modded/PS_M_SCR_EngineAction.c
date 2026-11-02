modded class SCR_EngineAction
{
	protected bool PS_IsEngineStartBlocked(IEntity user = null)
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
			if (!vehicle || !vehicle.IsEnableMoveOnFreeze() || SCR_GetOutAction.PS_IsUserOutFreezeZone(user))
			{
				SetCannotPerformReason("#PS-Freeze_Time");
				return true;
			}
		}

		return false;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		bool starting = m_bTargetState;
		if (m_bIsToggle)
			starting = !GetState();

		if (starting && PS_IsEngineStartBlocked(user))
			return false;

		return super.CanBePerformedScript(user);
	}

	override void OnActionStart(IEntity pUserEntity)
	{
		if (PS_IsEngineStartBlocked(pUserEntity))
			return;

		super.OnActionStart(pUserEntity);
	}

	override void PerformContinuousAction(IEntity pOwnerEntity, IEntity pUserEntity, float timeSlice)
	{
		if (PS_IsEngineStartBlocked(pUserEntity))
			return;

		super.PerformContinuousAction(pOwnerEntity, pUserEntity, timeSlice);
	}

	override void SetState(bool enable)
	{
		// Helicopters use their own continuous action and reach this check through super.SetState.
		if (enable && PS_IsEngineStartBlocked())
			return;

		super.SetState(enable);
	}
}
