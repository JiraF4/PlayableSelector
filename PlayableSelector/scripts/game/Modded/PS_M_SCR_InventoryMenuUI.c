/**
 * @brief Модификация инвентаря для блокировки открытия в фазе Hard Freeze.
 * @subsystem UI
 * @context UI / Client
 * @entity SCR_InventoryMenuUI
 * @depends PlayableSelector
 * @listens None
 * @details Блокирует открытие экрана инвентаря во время действия Hard Freeze.
 */
modded class SCR_InventoryMenuUI
{
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		if (PS_GameModeCoop.IsHardFreezeActiveStatic())
			Close();
	}
}
