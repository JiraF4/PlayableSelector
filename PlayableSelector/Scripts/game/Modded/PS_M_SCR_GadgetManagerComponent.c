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
	 * @brief Асинхронный опрос стриминга радиооборудования при JIP для гарантии включения питания.
	 * @param[in] owner Сущность персонажа
	 * @param[in] attempt Номер текущей попытки опроса
	 * @details Быстрая фаза (20 попыток × 250 мс = 5 с) на время обычного JIP-стриминга,
	 *          затем медленный дожим (раз в 1000 мс) вплоть до RESYNC_MAX_ATTEMPTS.
	 * @workaround При админской пересадке в живое тело рация может доехать по сети
	 *             значительно позже 5 секунд. Прежний жёсткий лимит в 20 попыток
	 *             приводил к тому, что опоздавшая рация так и не запитывалась → «замок»
	 *             на записи радиоканала в меню голоса.
	 */
	protected void PS_ResyncVON(IEntity owner, int attempt = 0)
	{
		if (!owner)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetControlledEntity() != owner)
			return;

		// Быстрая фаза: 20 попыток × 250 мс = 5 секунд активного стриминга
		const int RESYNC_FAST_ATTEMPTS = 20;
		// Полный лимит: далее медленный дожим раз в 1000 мс, суммарно ~40 секунд ожидания
		const int RESYNC_MAX_ATTEMPTS = 55;

		// Проверяем наличие радиоустройств (носимых и ранцевых) в менеджере снаряжения
		array<SCR_GadgetComponent> radioGadgets = GetGadgetsByType(EGadgetType.RADIO);
		array<SCR_GadgetComponent> backpackGadgets = GetGadgetsByType(EGadgetType.RADIO_BACKPACK);
		bool hasRadios = (radioGadgets && !radioGadgets.IsEmpty()) || (backpackGadgets && !backpackGadgets.IsEmpty());

		// Рации найдены — гарантируем питание и завершаем опрос
		if (hasRadios)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_ResyncVON: radios resolved for owner=%1 (attempt=%2, foundRadios=1)", owner, attempt), LogLevel.NORMAL);

			PS_PowerOnRadios(radioGadgets, backpackGadgets);
			PS_RadioVoiceFix.Request();
			return;
		}

		// Рации ещё нет и полный лимит ожидания исчерпан — прекращаем опрос
		if (attempt >= RESYNC_MAX_ATTEMPTS)
		{
			if (PS_RadioVoiceFix.s_bDebug)
				Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_ResyncVON: gave up for owner=%1 (attempt=%2, foundRadios=0)", owner, attempt), LogLevel.NORMAL);
			return;
		}

		// Радиостанции ещё реплицируются по сети — повторяем опрос
		int delay;
		if (attempt < RESYNC_FAST_ATTEMPTS)
			delay = 250;
		else
			delay = 1000;

		GetGame().GetCallqueue().CallLater(PS_ResyncVON, delay, false, owner, attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Гарантирует включённое питание для всех переданных радиостанций.
	 * @param[in] radioGadgets Носимые радиостанции (EGadgetType.RADIO)
	 * @param[in] backpackGadgets Ранцевые радиостанции (EGadgetType.RADIO_BACKPACK)
	 */
	protected void PS_PowerOnRadios(array<SCR_GadgetComponent> radioGadgets, array<SCR_GadgetComponent> backpackGadgets)
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
					Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_PowerOnRadios: guaranteed power ON for radio=%1", radioComp), LogLevel.NORMAL);
			}
		}
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

			// Подхватываем рацию, доехавшую к локальному телу позже окна опроса PS_ResyncVON.
			PS_EnsureLocalRadioBound(item);

			PS_RadioVoiceFix.Request();
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Подхватывает радиостанцию, доехавшую по сети позже окна опроса PS_ResyncVON.
	 * @param[in] radioItem Сущность только что добавленной радиостанции
	 * @details Если рация добавлена к телу, которым сейчас управляет локальный игрок,
	 *          немедленно включает ей питание и перезапускает опрос привязки VON с нуля.
	 *          НЕ зависит от PS_RadioVoiceFix.s_bEnabled (тот выключен по умолчанию),
	 *          поэтому «опоздавшая» рация больше не остаётся с иконкой замка на радиоканале.
	 */
	protected void PS_EnsureLocalRadioBound(IEntity radioItem)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity controlled = pc.GetControlledEntity();
		// Реагируем только если рация принадлежит локально управляемому телу
		if (!controlled || controlled != GetOwner())
			return;

		if (radioItem)
		{
			BaseRadioComponent radioComp = BaseRadioComponent.Cast(radioItem.FindComponent(BaseRadioComponent));
			if (radioComp && !radioComp.IsPowered())
			{
				radioComp.SetPower(true);
				if (PS_RadioVoiceFix.s_bDebug)
					Print(string.Format("[PS_Radio] SCR_GadgetManagerComponent.PS_EnsureLocalRadioBound: powered late-streamed radio=%1", radioComp), LogLevel.NORMAL);
			}
		}

		// Перезапускаем опрос привязки: рация уже в инвентаре и будет найдена на attempt=0
		GetGame().GetCallqueue().Remove(PS_ResyncVON);
		GetGame().GetCallqueue().CallLater(PS_ResyncVON, 0, false, controlled, 0);
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
