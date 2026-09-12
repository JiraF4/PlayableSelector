/**
 * @brief Модификация менеджера снаряжения для стабильной регистрации радиостанций и JIP синхронизации.
 * @subsystem Lobby | Gadgets | Radio
 * @context Client | Authority
 * @depends PSCore
 * @details Предотвращает состояние гонки при JIP переподключении и инициирует сторож питания
 *          радиостанций при добавлении или удалении радиоустройств из слотов инвентаря.
 */
modded class SCR_GadgetManagerComponent
{
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Перехват события смены владения персонажем игроком.
	 * @workaround При JIP/реконнекте локальный PlayerController на клиенте может не успеть
	 *             обновить GetLocalControlledEntity(), что приводит к ошибочному вызову UnregisterVONEntries().
	 *             Отложенная повторная проверка гарантирует корректную регистрацию радиостанций.
	 */
	override void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		super.OnControlledByPlayer(owner, controlled);

		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		if (owner)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.OnControlledByPlayer: owner=%1, controlled=%2 (scheduling resync in 350ms)", owner, controlled), LogLevel.NORMAL);

			GetGame().GetCallqueue().Remove(PS_ResyncVON);
			GetGame().GetCallqueue().CallLater(PS_ResyncVON, 350, false, owner);
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Отложенная валидация и повторная регистрация радиостанций в контроллере VoN.
	 * @param[in] owner Сущность персонажа
	 */
	protected void PS_ResyncVON(IEntity owner)
	{
		if (!owner)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetControlledEntity() != owner)
			return;

		if (PS_RadioVoiceFix.s_bDebug)
			Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_ResyncVON: registering VON entries for owner=%1", owner), LogLevel.NORMAL);

		RegisterVONEntries();
		PS_RadioVoiceFix.Request();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Перехват добавления предмета в инвентарь или слот.
	 */
	override void OnItemAdded(InventoryItemComponent item, BaseInventoryStorageComponent storageOwner)
	{
		super.OnItemAdded(item, storageOwner);

		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		if (!item)
			return;

		SCR_GadgetComponent gadgetComp = SCR_GadgetComponent.Cast(item.GetOwner().FindComponent(SCR_GadgetComponent));
		if (!gadgetComp)
			return;

		EGadgetType type = gadgetComp.GetType();
		if (type == EGadgetType.RADIO || type == EGadgetType.RADIO_BACKPACK)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.OnItemAdded: radio gadget %1 (type=%2) added, requesting power cycle", gadgetComp, typename.EnumToString(EGadgetType, type)), LogLevel.NORMAL);
			PS_RadioVoiceFix.Request();
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Перехват извлечения предмета из инвентаря или слота.
	 */
	override void OnItemRemoved(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		super.OnItemRemoved(item, storageOwner);

		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		if (!item)
			return;

		SCR_GadgetComponent gadgetComp = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
		if (!gadgetComp)
			return;

		EGadgetType type = gadgetComp.GetType();
		if (type == EGadgetType.RADIO || type == EGadgetType.RADIO_BACKPACK)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.OnItemRemoved: radio gadget %1 (type=%2) removed, requesting power cycle", gadgetComp, typename.EnumToString(EGadgetType, type)), LogLevel.NORMAL);
			PS_RadioVoiceFix.Request();
		}
	}
}
