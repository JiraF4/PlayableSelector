class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	TextWidget m_wRoomName;
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
	}
	
	void SetRoomName(string name)
	{
		m_wRoomName.SetText(name);
	}
}