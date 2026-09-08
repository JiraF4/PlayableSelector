// PS_VoNModes.c
// Direct voice volume modes (whisper / normal / shout).

//------------------------------------------------------------------------------------------------
/**
 * @brief Режимы дальности прямого голоса (Direct VoN).
 * @description В движке Enfusion дальность слышимости голоса жестко зашита в ACP-аудиопроект компонента
 *              SCR_VoNComponent и не имеет динамического runtime API. Для реализации переключения
 *              громкости каждый режим представлен отдельным компонентом на базовом префабе персонажа:
 *              - WHISPER (~5 м)
 *              - NORMAL  (~30 м, ваниль ~68 м)
 *              - SHOUT   (~90-120 м)
 */
enum PS_EVoNMode
{
	WHISPER,
	NORMAL,
	SHOUT
}

class PS_VoNModes
{
	//! Глобальный флаг отладки режимов голоса (вывод в лог и экранные подсказки)
	static bool s_bDebugVoN = false;

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Получение следующего режима по циклу переключения.
	 * @description Порядок переключения: Normal -> Shout -> Whisper -> Normal.
	 * @param[in] mode Текущий режим
	 * @return Следующий режим
	 */
	static PS_EVoNMode GetNext(PS_EVoNMode mode)
	{
		switch (mode)
		{
			case PS_EVoNMode.NORMAL: return PS_EVoNMode.SHOUT;
			case PS_EVoNMode.SHOUT:  return PS_EVoNMode.WHISPER;
		}

		return PS_EVoNMode.NORMAL;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Сопоставление режима голоса с соответствующим типом компонента SCR_VoNComponent.
	 * @param[in] mode Режим громкости
	 * @return Тип класса компонента
	 */
	static typename GetComponentType(PS_EVoNMode mode)
	{
		switch (mode)
		{
			case PS_EVoNMode.WHISPER: return PS_VoNWhisperComponent;
			case PS_EVoNMode.SHOUT:   return PS_VoNShoutComponent;
		}

		return SCR_VoNComponent;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Получение строки локализации для отображения названия режима в интерфейсе.
	 * @param[in] mode Режим громкости
	 * @return Ключ локализации строки
	 */
	static string GetDisplayName(PS_EVoNMode mode)
	{
		switch (mode)
		{
			case PS_EVoNMode.WHISPER: return "#PS-VoN_ModeWhisper";
			case PS_EVoNMode.SHOUT:   return "#PS-VoN_ModeShout";
		}

		return "#PS-VoN_ModeNormal";
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class PS_VoNWhisperComponentClass : SCR_VoNComponentClass
{
}

//! Компонент прямого шепота (~5 м)
class PS_VoNWhisperComponent : SCR_VoNComponent
{
}

class PS_VoNNormalComponentClass : SCR_VoNComponentClass
{
}

//! @deprecated Устаревший класс; режим NORMAL использует базовый SCR_VoNComponent
class PS_VoNNormalComponent : SCR_VoNComponent
{
}

class PS_VoNShoutComponentClass : SCR_VoNComponentClass
{
}

//! Компонент крика (~90-120 м)
class PS_VoNShoutComponent : SCR_VoNComponent
{
}
