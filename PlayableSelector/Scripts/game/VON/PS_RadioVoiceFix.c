/**
 * @brief Сторож регистрации радиостанций в C++ RadioManagerEntity движка Enfusion.
 * @subsystem Audio | Network
 * @context Client
 * @depends PSCore
 * @details Выполняет автоматический безопасный цикл питания (PowerOff -> 150ms -> PowerOn)
 *          с дебаунсом при манипуляциях со снаряжением, смене персонажа и пересадке в транспорт.
 * @workaround Устраняет баг движка Enfusion, при котором C++ трансивер BaseTransceiver
 *             выпадает из RadioManagerEntity при перемещении радиостанции в инвентаре.
 */
class PS_RadioVoiceFix
{
	//! Флаг отладочного логирования радиоподсистемы PlayableSelector (false для отключения)
	static bool s_bDebug = true;

	protected static const int FIX_DELAY_MS = 350;
	protected static const int POWER_OFF_MS = 150;

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Запрос на ресинхронизацию радиостанций локального игрока с дебаунсом.
	 */
	static void Request()
	{
		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		if (s_bDebug)
			Print("[PS_Radio] PS_RadioVoiceFix.Request: scheduling power cycle...", LogLevel.NORMAL);

		GetGame().GetCallqueue().Remove(PowerCycleAll);
		GetGame().GetCallqueue().CallLater(PowerCycleAll, FIX_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	private static void PowerCycleAll()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity entity = pc.GetControlledEntity();
		if (!entity)
			return;

		SCR_GadgetManagerComponent gadgetMgr = SCR_GadgetManagerComponent.Cast(entity.FindComponent(SCR_GadgetManagerComponent));
		if (!gadgetMgr)
			return;

		array<SCR_GadgetComponent> gadgets = {};
		gadgetMgr.GetGadgetsByType(EGadgetType.RADIO, gadgets);
		gadgetMgr.GetGadgetsByType(EGadgetType.RADIO_BACKPACK, gadgets);

		int cycledCount = 0;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget)
				continue;

			IEntity owner = gadget.GetOwner();
			if (!owner)
				continue;

			BaseRadioComponent radio = BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
			if (radio && radio.IsPowered())
			{
				radio.SetPower(false);
				cycledCount++;
				GetGame().GetCallqueue().CallLater(RestorePower, POWER_OFF_MS, false, radio);
			}
		}

		if (s_bDebug && cycledCount > 0)
			Print(string.Format("[PS_Radio] PS_RadioVoiceFix: power cycling %1 radio(s)...", cycledCount), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	private static void RestorePower(BaseRadioComponent radio)
	{
		if (radio)
		{
			radio.SetPower(true);
			if (s_bDebug)
				Print(string.Format("[PS_Radio] PS_RadioVoiceFix: restored power for radio %1", radio), LogLevel.NORMAL);
		}
	}
}
