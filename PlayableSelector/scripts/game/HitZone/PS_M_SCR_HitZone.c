/**
 * @brief Модификация хитзоны для полного предотвращения урона в фазах Hard Freeze и Soft Freeze.
 * @subsystem Combat / Damage
 * @context Hybrid
 * @entity SCR_HitZone
 * @depends PlayableSelector
 * @listens None
 * @details Блокирует весь входящий урон во время Hard Freeze (абсолютный годмод)
 *          и опционально во время Soft Freeze, если включен запрет стрельбы на миссии.
 *          Использует быстрый статический флаг без накладных расходов динамических кастов.
 */
modded class SCR_HitZone
{
	/**
	 * @brief Расчет эффективного урона с проверкой активных режимов заморозки.
	 * @param damageContext Контекст наносимого повреждения
	 * @param isDOT Флаг урона со временем (Damage Over Time)
	 * @return float 0 при активной заморозке, иначе результат базового расчета
	 */
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT)
	{
		if (PS_GameModeCoop.IsDamageBlockedStatic())
			return 0;

		return super.ComputeEffectiveDamage(damageContext, isDOT);
	}
}