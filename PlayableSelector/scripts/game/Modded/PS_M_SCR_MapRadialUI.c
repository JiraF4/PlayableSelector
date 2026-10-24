modded class SCR_MapRadialUI
{
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		if (!SCR_MapMarkersUI.PS_CanLocalPlayerPlaceMarkers()) return;
		if (m_RadialMenu && m_RadialMenu.IsOpened()) { if (m_RadialController) m_RadialController.OnInputOpen(); return; }
		if (m_CursorModule && (m_CursorModule.GetCursorState() & EMapCursorState.CS_PAN)) return;
		int mouseX, mouseY; WidgetManager.GetMousePos(mouseX, mouseY);
		array<Widget> widgets = {}; WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), widgets);
		foreach (Widget widget : widgets) { if (widget.IsInherited(ScrollLayoutWidget) || widget.IsInherited(ButtonWidget)) return; }
		if (m_RadialController) m_RadialController.OnInputOpen();
	}
}
