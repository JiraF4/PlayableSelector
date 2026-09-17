/**
 * @brief Виджет обратного отсчета времени заморозки (Hard Freeze и Soft Freeze).
 * @subsystem UI
 * @context UI / Client
 * @entity UI/layouts/FreezeTime/FreezeTimeCounter.layout
 * @depends PlayableSelector
 * @listens None
 * @details Отображает шкалу прогресса и время в двух режимах:
 *          красный (Hard Freeze / #PS-Load_time) и оранжевый (Soft Freeze / #PS-Freeze_time)
 *          с независимым расчетом заполнения для каждой фазы.
 */
class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wFreezeTimeCounterText;
	protected TextWidget m_wFreezeTimeText;
	protected ImageWidget m_wFill;
	protected ImageWidget m_wEmpty;

	protected float m_fFullFreezeTime;
	protected float m_fFullFreezeTimeCurrent;
	protected float m_fActivePhaseMaxTime;
	protected bool m_bIsHardFreezeMode;
	protected bool m_bUpdateActive;
	protected int m_iLastDisplayedSecond = -1;

	protected ref Color m_ColorTransparent;
	protected ref Color m_ColorRed;
	protected ref Color m_ColorOrange;

	protected void EnsureColors()
	{
		if (!m_ColorTransparent)
			m_ColorTransparent = new Color(0.0, 0.0, 0.0, 0.0);
		if (!m_ColorRed)
			m_ColorRed = new Color(1.0, 0.0, 0.0, 1.0);
		if (!m_ColorOrange)
			m_ColorOrange = new Color(0.760998, 0.386007, 0.078004, 1.0);
	}

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
		m_wFreezeTimeText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeText"));
		m_wFill = ImageWidget.Cast(w.FindAnyWidget("Fill"));
		m_wEmpty = ImageWidget.Cast(w.FindAnyWidget("Empty"));
		EnsureColors();
	}

	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		m_bUpdateActive = false;
		GetGame().GetCallqueue().Remove(Update);
	}

	/**
	 * @brief Переключение режима отображения шкалы и цветовой схемы.
	 * @param isHardFreeze Флаг режима полной блокировки
	 * @param maxDuration Максимальная длительность активной фазы в мс для корректного прогресс-бара
	 */
	void UpdateDisplayMode(bool isHardFreeze, float maxDuration)
	{
		EnsureColors();
		m_bIsHardFreezeMode = isHardFreeze;
		m_fActivePhaseMaxTime = maxDuration;

		if (m_bIsHardFreezeMode)
		{
			if (m_wFreezeTimeText)
				m_wFreezeTimeText.SetText("#PS-Load_time");
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(m_ColorRed);
			if (m_wFill)
				m_wFill.SetColor(m_ColorRed);
			if (m_wEmpty)
				m_wEmpty.SetColor(m_ColorTransparent);
		}
		else
		{
			if (m_wFreezeTimeText)
				m_wFreezeTimeText.SetText("#PS-Freeze_time");
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(m_ColorOrange);
			if (m_wFill)
				m_wFill.SetColor(m_ColorOrange);
			if (m_wEmpty)
				m_wEmpty.SetColor(m_ColorTransparent);
		}
	}

	protected void UpdateCounterText(int totalSeconds)
	{
		if (!m_wFreezeTimeCounterText)
			return;

		if (totalSeconds < 0)
			totalSeconds = 0;

		int seconds = Math.Mod(totalSeconds, 60);
		int minutes = totalSeconds / 60;
		m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}

	void Update()
	{
		float maxTime = m_fActivePhaseMaxTime;
		if (maxTime <= 0)
			maxTime = m_fFullFreezeTime;
		if (maxTime <= 0)
			return;

		m_fFullFreezeTimeCurrent -= GetGame().GetWorld().GetTimeSlice() * 1000;
		float percent = m_fFullFreezeTimeCurrent / maxTime;
		percent = Math.Clamp(percent, 0.0, 1.0);
		HorizontalLayoutSlot.SetFillWeight(m_wFill, percent);
		HorizontalLayoutSlot.SetFillWeight(m_wEmpty, 1.0 - percent);

		int sec = Math.Floor(m_fFullFreezeTimeCurrent / 1000);
		if (sec != m_iLastDisplayedSecond)
		{
			m_iLastDisplayedSecond = sec;
			UpdateCounterText(sec);
		}
	}

	void SetTime(int time)
	{
		m_fFullFreezeTimeCurrent = time;
		if (m_fFullFreezeTime <= 0)
			m_fFullFreezeTime = time;

		int sec = Math.Floor(((float)time) / 1000);
		m_iLastDisplayedSecond = sec;
		UpdateCounterText(sec);

		if (!m_bUpdateActive)
		{
			m_bUpdateActive = true;
			GetGame().GetCallqueue().CallLater(Update, 0, true);
		}
	}
}