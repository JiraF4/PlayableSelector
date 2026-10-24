class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wFreezeTimeCounterText;
	protected TextWidget m_wFreezeTimeText;
	
	protected ImageWidget m_wFill;
	protected ImageWidget m_wEmpty;

	protected float m_fSmoothRemaining = -1.0;
	protected float m_fLastServerRemaining = -1.0;
	protected int m_iLastDisplayedSecond = -1;
	protected bool m_bLastHardFreeze = false;
	protected PS_EFreezePhase m_eLastFreezePhase = PS_EFreezePhase.NOT_STARTED;
	protected string m_sLastTitleText = "";
	protected ref Color m_ColorOrange;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
		m_wFreezeTimeText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeText"));
		
		m_wEmpty = ImageWidget.Cast(w.FindAnyWidget("Fill"));
		m_wFill = ImageWidget.Cast(w.FindAnyWidget("Empty"));

		m_ColorOrange = new Color(0.760998, 0.386007, 0.078004, 1.000000);
		ResetSmoothState();

		UpdateDisplay();
		GetGame().GetCallqueue().CallLater(UpdateDisplay, 0, true);
	}

	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		GetGame().GetCallqueue().Remove(UpdateDisplay);

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.ClearHUDReference();
	}

	protected void ResetSmoothState()
	{
		m_fSmoothRemaining = -1.0;
		m_fLastServerRemaining = -1.0;
		m_iLastDisplayedSecond = -1;
		m_bLastHardFreeze = false;
		m_eLastFreezePhase = PS_EFreezePhase.NOT_STARTED;
		m_sLastTitleText = "";
	}
	
	protected void UpdateDisplay()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager || !m_wRoot)
			return;

		if (!freezeManager.IsHardFreezeActive() && freezeManager.IsFreezeTimeEnd())
		{
			m_wRoot.RemoveFromHierarchy();
			return;
		}

		bool isHardFreeze = freezeManager.IsHardFreezeActive();
		PS_EFreezePhase phase = freezeManager.GetFreezePhase();

		if (isHardFreeze)
		{
			float remSec = freezeManager.GetHardFreezeRemaining();
			float durSec = freezeManager.GetHardFreezeDuration();

			if (!m_bLastHardFreeze || m_fSmoothRemaining < 0 || m_fLastServerRemaining < 0)
			{
				m_fSmoothRemaining = remSec;
				m_fLastServerRemaining = remSec;
				m_bLastHardFreeze = true;
				m_eLastFreezePhase = phase;
				m_iLastDisplayedSecond = -1;

				if (m_wFreezeTimeCounterText)
					m_wFreezeTimeCounterText.SetColor(Color.Red);
				if (m_wEmpty)
					m_wEmpty.SetColor(Color.Red);
			}

			string title;
			if (freezeManager.IsStartupFreeze() || phase == PS_EFreezePhase.STARTUP)
				title = "#PS-Load_Time";
			else
				title = "#PS-Freeze_Pause";

			if (m_sLastTitleText != title)
			{
				m_sLastTitleText = title;
				if (m_wFreezeTimeText)
					m_wFreezeTimeText.SetText(title);
			}

			ApplySmoothCountdown(remSec, durSec);
			return;
		}

		if (phase == PS_EFreezePhase.ORDINARY)
		{
			float remMs = freezeManager.GetCurrentFreezeTime();
			if (remMs <= 0)
			{
				m_wRoot.RemoveFromHierarchy();
				return;
			}

			float remSec = remMs / 1000.0;
			float durSec = freezeManager.GetEffectiveFreezeTime() / 1000.0;

			if (m_bLastHardFreeze || m_eLastFreezePhase != PS_EFreezePhase.ORDINARY || m_fSmoothRemaining < 0 || m_fLastServerRemaining < 0)
			{
				m_fSmoothRemaining = remSec;
				m_fLastServerRemaining = remSec;
				m_bLastHardFreeze = false;
				m_eLastFreezePhase = PS_EFreezePhase.ORDINARY;
				m_iLastDisplayedSecond = -1;

				if (m_wFreezeTimeCounterText)
					m_wFreezeTimeCounterText.SetColor(m_ColorOrange);
				if (m_wEmpty)
					m_wEmpty.SetColor(m_ColorOrange);
			}

			if (m_sLastTitleText != "#PS-Freeze_Time")
			{
				m_sLastTitleText = "#PS-Freeze_Time";
				if (m_wFreezeTimeText)
					m_wFreezeTimeText.SetText("#PS-Freeze_Time");
			}

			ApplySmoothCountdown(remSec, durSec);
			return;
		}

		m_wRoot.RemoveFromHierarchy();
	}

	protected void ApplySmoothCountdown(float serverRemSec, float durSec)
	{
		float dt = 0.0;
		if (GetGame() && GetGame().GetWorld())
			dt = GetGame().GetWorld().GetTimeSlice();

		if (serverRemSec != m_fLastServerRemaining)
		{
			float delta = serverRemSec - m_fSmoothRemaining;
			if (Math.AbsFloat(delta) > 1.5)
				m_fSmoothRemaining = serverRemSec;
			else
				m_fSmoothRemaining = Math.Lerp(m_fSmoothRemaining, serverRemSec, 0.2);

			m_fLastServerRemaining = serverRemSec;
		}

		m_fSmoothRemaining -= dt;
		if (m_fSmoothRemaining < 0)
			m_fSmoothRemaining = 0;

		if (m_wEmpty && m_wFill)
		{
			float percent = 1.0;
			if (durSec > 0)
				percent = Math.Clamp(m_fSmoothRemaining / durSec, 0.0, 1.0);

			HorizontalLayoutSlot.SetFillWeight(m_wEmpty, percent);
			HorizontalLayoutSlot.SetFillWeight(m_wFill, 1.0 - percent);
		}

		int totalSec = Math.Ceil(m_fSmoothRemaining);
		if (totalSec < 0)
			totalSec = 0;

		if (totalSec != m_iLastDisplayedSecond)
		{
			m_iLastDisplayedSecond = totalSec;
			int min = totalSec / 60;
			int sec = totalSec - min * 60;
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetTextFormat("%1:%2", min.ToString(2), sec.ToString(2));
		}
	}
}