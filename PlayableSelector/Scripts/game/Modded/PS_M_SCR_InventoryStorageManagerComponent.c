modded class SCR_InventoryStorageManagerComponent
{
	override void Action_OpenInventory()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}

		super.Action_OpenInventory();
	}
};
