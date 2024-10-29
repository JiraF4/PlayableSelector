class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	TextWidget m_wFreezeTimeCounterText;
	float m_fFullFreezeTime;
	float m_fFullFreezeTimeCurrent;
	
	ImageWidget m_wFill;
	ImageWidget m_wEmpty;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
		
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
		
		m_fFullFreezeTimeCurrent = time;
		
		float timeSeconds = ((float) time) / 1000;
		int seconds = Math.Mod(timeSeconds, 60);
		int minutes = (timeSeconds / 60);
		m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}
}