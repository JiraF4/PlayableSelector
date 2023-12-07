class PS_FreezeTimeCounter : SCR_ScriptedWidgetComponent
{
	TextWidget m_wFreezeTimeCounterText;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFreezeTimeCounterText = TextWidget.Cast(w.FindAnyWidget("FreezeTimeCounterText"));
	}
	
	void SetTime(int time)
	{
		float timeSeconds = ((float) time) / 1000;
		int seconds = Math.Mod(timeSeconds, 60);
		int minutes = (timeSeconds / 60);
		m_wFreezeTimeCounterText.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}
}