// PS_VoNModeHud.c
// Client UI popup indicating the active direct voice mode.

class PS_VoNModeHud : ScriptedWidgetComponent
{
	protected const ResourceName LAYOUT = "{67C8A94DF58B0006}UI/HUD/PS_VoNModePopup.layout";
	protected const float HOLD_S = 1.2;   // fully visible after the last change
	protected const float FADE_S = 0.4;   // fade-out duration after the hold
	protected const int TICK_MS = 32;

	protected static PS_VoNModeHud s_Active;

	protected Widget m_wRoot;
	protected TextWidget m_wModeName;
	protected float m_fElapsed;

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Отображение или обновление всплывающего индикатора режима громкости голоса.
	 * @param[in] mode Активный режим голоса
	 */
	static void Show(PS_EVoNMode mode)
	{
		// A still-visible popup just gets the new mode and a fresh timer.
		if (s_Active)
		{
			s_Active.Begin(mode);
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		Widget root = workspace.CreateWidgets(LAYOUT, null);
		if (!root)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] PS_VoNModeHud.Show: FAILED to create widgets for layout '%1'!", LAYOUT), LogLevel.ERROR);
			return;
		}

		root.SetZOrder(100);

		// Stretch the root so the pill's left-center anchors resolve.
		FrameSlot.SetAnchorMin(root, 0, 0);
		FrameSlot.SetAnchorMax(root, 1, 1);
		FrameSlot.SetOffsets(root, 0, 0, 0, 0);

		PS_VoNModeHud comp = PS_VoNModeHud.Cast(root.FindHandler(PS_VoNModeHud));
		if (!comp)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] PS_VoNModeHud.Show: PS_VoNModeHud handler not found on root widget!", LogLevel.ERROR);
			root.RemoveFromHierarchy();
			return;
		}

		s_Active = comp;
		comp.Begin(mode);
		GetGame().GetCallqueue().CallLater(comp.Tick, TICK_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Инициализация обработчика виджета всплывающего индикатора.
	 * @param[in] w Корневой виджет
	 */
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wRoot = w;
		m_wModeName = TextWidget.Cast(w.FindAnyWidget("ModeName"));
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Очистка обработчика виджета и остановка таймера анимации затухания.
	 * @param[in] w Закрывающийся виджет
	 */
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);

		GetGame().GetCallqueue().Remove(Tick);

		if (s_Active == this)
			s_Active = null;

		m_wRoot = null;
		m_wModeName = null;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Запуск показа нового режима громкости с полным сбросом прозрачности.
	 * @param[in] mode Выбранный режим громкости
	 */
	protected void Begin(PS_EVoNMode mode)
	{
		m_fElapsed = 0;

		if (m_wModeName)
			m_wModeName.SetText(PS_VoNModes.GetDisplayName(mode));

		if (m_wRoot)
			m_wRoot.SetOpacity(1);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Периодический тик анимации удержания и плавного растворения виджета.
	 */
	protected void Tick()
	{
		m_fElapsed += TICK_MS / 1000.0;

		if (m_fElapsed <= HOLD_S)
			return;

		float fade = 1.0 - (m_fElapsed - HOLD_S) / FADE_S;
		if (fade <= 0)
		{
			Dismiss();
			return;
		}

		if (m_wRoot)
			m_wRoot.SetOpacity(fade);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Удаление виджета с экрана по окончании анимации.
	 */
	protected void Dismiss()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}
}
