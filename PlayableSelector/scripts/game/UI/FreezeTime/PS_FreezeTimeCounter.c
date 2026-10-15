class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wFreezeTimeCounterText;
	protected TextWidget m_wFreezeTimeText;
	protected float m_fFullFreezeTime;
	protected float m_fFullFreezeTimeCurrent;
	
	protected ImageWidget m_wFill;
	protected ImageWidget m_wEmpty;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
		m_wFreezeTimeText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeText"));
		
		m_wEmpty = ImageWidget.Cast(w.FindAnyWidget("Fill"));
		m_wFill = ImageWidget.Cast(w.FindAnyWidget("Empty"));

		UpdateDisplay();
		GetGame().GetCallqueue().CallLater(UpdateDisplay, 200, true);
	}

	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		GetGame().GetCallqueue().Remove(UpdateDisplay);
		GetGame().GetCallqueue().Remove(Update);

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
			freezeManager.ClearHUDReference();
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

		if (freezeManager.IsHardFreezeActive())
		{
			if (m_wFreezeTimeText)
			{
				if (freezeManager.IsStartupFreeze() || freezeManager.GetFreezePhase() == PS_EFreezePhase.STARTUP)
					m_wFreezeTimeText.SetText("#PS-Load_Time");
				else
					m_wFreezeTimeText.SetText("#PS-Freeze_Pause");
			}
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(Color.Red);

			float remSec = freezeManager.GetHardFreezeRemaining();
			int total = Math.Ceil(remSec);
			if (total < 0)
				total = 0;
			int min = total / 60;
			int sec = total - min * 60;
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetTextFormat("%1:%2", min.ToString(2), sec.ToString(2));

			if (m_wEmpty && m_wFill)
			{
				m_wEmpty.SetColor(Color.Red);
				float dur = freezeManager.GetHardFreezeDuration();
				float percent = 1.0;
				if (dur > 0)
					percent = Math.Clamp(remSec / dur, 0.0, 1.0);

				HorizontalLayoutSlot.SetFillWeight(m_wEmpty, percent);
				HorizontalLayoutSlot.SetFillWeight(m_wFill, 1.0 - percent);
			}
			return;
		}

		if (freezeManager.GetFreezePhase() == PS_EFreezePhase.ORDINARY)
		{
			float remMs = freezeManager.GetCurrentFreezeTime();
			if (remMs <= 0)
			{
				m_wRoot.RemoveFromHierarchy();
				return;
			}

			if (m_wFreezeTimeText)
				m_wFreezeTimeText.SetText("#PS-Freeze_time");
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(new Color(0.760998, 0.386007, 0.078004, 1.000000));

			int totalSec = Math.Ceil(remMs / 1000.0);
			int minOrd = totalSec / 60;
			int secOrd = totalSec - minOrd * 60;
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minOrd.ToString(2), secOrd.ToString(2));

			float eff = freezeManager.GetEffectiveFreezeTime();
			float percent = 1.0;
			if (eff > 0)
				percent = Math.Clamp(remMs / eff, 0.0, 1.0);

			if (m_wEmpty && m_wFill)
			{
				m_wEmpty.SetColor(new Color(0.760998, 0.386007, 0.078004, 1.000000));
				HorizontalLayoutSlot.SetFillWeight(m_wEmpty, percent);
				HorizontalLayoutSlot.SetFillWeight(m_wFill, 1.0 - percent);
			}
		}
	}

	// Legacy backward compatibility for older manual callers
	void Update()
	{
		if (m_fFullFreezeTime == 0)
			return;
		
		m_fFullFreezeTimeCurrent -= GetGame().GetWorld().GetTimeSlice() * 1000;
		float percent = ((float)m_fFullFreezeTimeCurrent) / m_fFullFreezeTime;
		if (percent < 0)
			percent = 0;
		if (m_wEmpty && m_wFill)
		{
			HorizontalLayoutSlot.SetFillWeight(m_wEmpty, percent);
			HorizontalLayoutSlot.SetFillWeight(m_wFill, 1.0 - percent);
		}
		
		if (m_fFullFreezeTimeCurrent < -1 && m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
		}
	}
	
	void SetTime(int time)
	{
		if (m_fFullFreezeTime == 0)
		{
			m_fFullFreezeTime = time;
			GetGame().GetCallqueue().CallLater(Update, 0, true);
		}
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.IsDisableTimeEnd())
		{
			if (m_wFreezeTimeText)
				m_wFreezeTimeText.SetText("#PS-Freeze_time");
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(new Color(0.760998, 0.386007, 0.078004, 1.000000));
		}
		else
		{
			if (m_wFreezeTimeText)
				m_wFreezeTimeText.SetText("#PS-Load_Time");
			if (m_wFreezeTimeCounterText)
				m_wFreezeTimeCounterText.SetColor(Color.Red);
			if (gameModeCoop)
				time -= (gameModeCoop.GetFreezeTime() - gameModeCoop.GetDisableTime());
		}
		
		m_fFullFreezeTimeCurrent = time;
		
		float timeSeconds = ((float) time) / 1000;
		int seconds = Math.Mod(timeSeconds, 60);
		int minutes = (timeSeconds / 60);
		if (m_wFreezeTimeCounterText)
			m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}
}