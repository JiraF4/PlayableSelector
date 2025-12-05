class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	TextWidget m_wFreezeTimeCounterText;
	TextWidget m_wFreezeTimeText;
	float m_fFullFreezeTime;
	float m_fFullFreezeTimeCurrent;
	
	ImageWidget m_wFill;
	ImageWidget m_wEmpty;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
		m_wFreezeTimeText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeText"));
		
		m_wEmpty = ImageWidget.Cast(w.FindAnyWidget("Fill"));
		m_wFill = ImageWidget.Cast(w.FindAnyWidget("Empty"));
	}
	
	void Update()
	{
		if (m_fFullFreezeTime == 0)
			return;
		
		m_fFullFreezeTimeCurrent -= GetGame().GetWorld().GetTimeSlice() * 1000;
		float percent = ((float)m_fFullFreezeTimeCurrent) / m_fFullFreezeTime;
		if (percent < 0)
			percent = 0;
		HorizontalLayoutSlot.SetFillWeight(m_wEmpty, percent);
		HorizontalLayoutSlot.SetFillWeight(m_wFill, 1.0 - percent);
		
		if (m_fFullFreezeTimeCurrent < -1)
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
		if (gameModeCoop.IsDisableTimeEnd())
		{
			m_wFreezeTimeText.SetText("#PS-Freeze_time");
			m_wFreezeTimeCounterText.SetColor(new Color(0.760998, 0.386007, 0.078004, 1.000000));
		}
		else
		{
			m_wFreezeTimeText.SetText("#PS-Load_time");
			m_wFreezeTimeCounterText.SetColor(Color.Red);
			time -= (gameModeCoop.GetFreezeTime() - gameModeCoop.GetDisableTime());
		}
		
		m_fFullFreezeTimeCurrent = time;
		
		float timeSeconds = ((float) time) / 1000;
		int seconds = Math.Mod(timeSeconds, 60);
		int minutes = (timeSeconds / 60);
		m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}
}