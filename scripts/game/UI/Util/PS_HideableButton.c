class PS_HideableButton : SCR_ScriptedWidgetComponent
{
	ImageWidget m_wImage;
	ButtonWidget m_wButton;
	SCR_ButtonBaseComponent m_wButtonHandler;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wImage = ImageWidget.Cast(w.FindAnyWidget("Image"));
		m_wButton = ButtonWidget.Cast(w.FindAnyWidget("Button"));
		m_wButtonHandler = SCR_ButtonBaseComponent.Cast(m_wButton.FindHandler(SCR_ButtonBaseComponent));
	}
}