modded class SCR_GetOutAction
{
	static bool PS_IsUserOutFreezeZone(IEntity user)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager || freezeManager.GetFreezePhase() != PS_EFreezePhase.ORDINARY)
			return false;

		PlayerController pc = GetGame().GetPlayerController();
		if (pc && (!user || pc.GetControlledEntity() == user))
		{
			PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(
				pc.FindComponent(PS_PlayableControllerComponent)
			);
			if (playableController && playableController.IsOutFreezeTime())
				return true;
		}

		if (Replication.IsServer() && user)
		{
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
			{
				int playerId = pm.GetPlayerIdFromControlledEntity(user);
				if (playerId > 0)
				{
					PlayerController serverPc = pm.GetPlayerController(playerId);
					if (serverPc)
					{
						PS_PlayableControllerComponent serverPlayable = PS_PlayableControllerComponent.Cast(
							serverPc.FindComponent(PS_PlayableControllerComponent)
						);
						if (serverPlayable && serverPlayable.IsOutFreezeTime())
							return true;
					}
				}
			}
		}

		return false;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			SetCannotPerformReason("#PS-HardFreeze_Title");
			return false;
		}

		if (PS_IsUserOutFreezeZone(user))
		{
			SetCannotPerformReason("#PS-Freeze_Time");
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

		if (SCR_GetOutAction.PS_IsUserOutFreezeZone(user))
		{
			SetCannotPerformReason("#PS-Freeze_Time");
			return false;
		}

		return super.CanBePerformedScript(user);
	}
}
