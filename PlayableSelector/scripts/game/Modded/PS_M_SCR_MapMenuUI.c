/**
 * @brief Модификация полноэкранной карты для блокировки открытия в фазе Hard Freeze.
 * @subsystem UI
 * @context UI / Client
 * @entity SCR_MapMenuUI
 * @depends PlayableSelector
 * @listens None
 * @details Блокирует открытие карты во время действия Hard Freeze.
 */
modded class SCR_MapMenuUI
{
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		if (PS_GameModeCoop.IsHardFreezeActiveStatic())
			Close();
	}
}
