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
			GetGame().GetCallqueue().Remove(PS_ResyncVON);

			if (controlled)
			{
				if (PS_RadioVoiceFix.s_bDebug)
					Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.OnControlledByPlayer: owner=%1, starting JIP radio streaming poll (250ms interval)", owner), LogLevel.NORMAL);

				GetGame().GetCallqueue().CallLater(PS_ResyncVON, 250, false, owner, 0);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Асинхронный опрос стриминга радиооборудования при JIP и регистрация в контроллере VoN.
	 * @param[in] owner Сущность персонажа
	 * @param[in] attempt Номер текущей попытки опроса (до 20 попыток = 5 секунд)
	 */
	protected void PS_ResyncVON(IEntity owner, int attempt = 0)
	{
		if (!owner)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetControlledEntity() != owner)
			return;

		// Проверяем наличие радиоустройств (носимых и ранцевых) в менеджере снаряжения
		array<SCR_GadgetComponent> radioGadgets = GetGadgetsByType(EGadgetType.RADIO);
		array<SCR_GadgetComponent> backpackGadgets = GetGadgetsByType(EGadgetType.RADIO_BACKPACK);
		bool hasRadios = (radioGadgets && !radioGadgets.IsEmpty()) || (backpackGadgets && !backpackGadgets.IsEmpty());

		// Если рации появились в инвентаре или исчерпан лимит ожидания (~5 секунд / 20 попыток)
		if (hasRadios || attempt >= 20)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_ResyncVON: registering VON entries for owner=%1 (attempt=%2, foundRadios=%3)", owner, attempt, hasRadios), LogLevel.NORMAL);

			RegisterVONEntries();

			// Гарантируем включенное питание для обнаруженных радиостанций
			if (hasRadios)
			{
				array<SCR_GadgetComponent> allRadios = {};
				if (radioGadgets)
					allRadios.InsertAll(radioGadgets);
				if (backpackGadgets)
					allRadios.InsertAll(backpackGadgets);

				foreach (SCR_GadgetComponent gadget : allRadios)
				{
					if (!gadget)
						continue;
					IEntity radioEnt = gadget.GetOwner();
					if (!radioEnt)
						continue;
					BaseRadioComponent radioComp = BaseRadioComponent.Cast(radioEnt.FindComponent(BaseRadioComponent));
					if (radioComp && !radioComp.IsPowered())
					{
						radioComp.SetPower(true);
						if (PS_RadioVoiceFix.s_bDebug)
							Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_ResyncVON: guaranteed power ON for radio=%1", radioComp), LogLevel.NORMAL);
					}
				}
			}

			PS_RadioVoiceFix.Request();
			return;
		}

		// Радиостанции еще реплицируются по сети — опрашиваем повторно через 250 мс
		GetGame().GetCallqueue().CallLater(PS_ResyncVON, 250, false, owner, attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Перехват добавления предмета в инвентарь или слот.
	 */
	override void OnItemAdded(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		super.OnItemAdded(item, storageOwner);

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
