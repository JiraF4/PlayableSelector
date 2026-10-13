modded class SCR_GameGameModeStateComponent
{
	override bool CanAdvanceState(SCR_EGameModeState nextState)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
			return false;

		return super.CanAdvanceState(nextState);
	}
}
