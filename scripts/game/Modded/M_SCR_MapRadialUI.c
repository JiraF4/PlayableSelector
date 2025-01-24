// Enable markers only for briefing if gamemode coop and flag is set
modded class SCR_MapRadialUI
{
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetMarkersOnlyOnBriefing())
			return;
		if (m_RadialMenu && m_RadialMenu.IsOpened())
			return;
		
		// Check widget
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		array<Widget> outWidgets = {};
		WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), outWidgets);			
		foreach (Widget widget : outWidgets)
		{
			if (widget.IsInherited(ScrollLayoutWidget) || widget.IsInherited(ButtonWidget))
				return;
		}
		
		m_RadialController.OnInputOpen();
	}
}