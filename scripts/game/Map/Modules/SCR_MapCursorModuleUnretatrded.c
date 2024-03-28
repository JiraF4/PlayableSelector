[BaseContainerProps()]
class SCR_MapCursorModuleUnretatrded : SCR_MapCursorModule
{	
	//------------------------------------------------------------------------------------------------
	//! Digital zoom mouse wheel
	override protected void OnInputZoomWheelUp( float value, EActionTrigger reason )
	{		
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
		
		// Check widget
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		array<Widget> outWidgets = {};
		WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), outWidgets);			
		foreach (Widget widget : outWidgets)
		{
			if (widget.IsInherited(ScrollLayoutWidget))
				return;
		}
		
		float targetPPU = m_MapEntity.GetTargetZoomPPU();
		value = value * m_fZoomMultiplierWheel;
		m_MapEntity.ZoomSmooth(targetPPU + targetPPU * (value * 0.001), m_fZoomAnimTime, false);	// the const here is adjusting the value to match the input with zoom range
	}
	
	//------------------------------------------------------------------------------------------------
	//! Digital zoom mouse wheel
	override protected void OnInputZoomWheelDown( float value, EActionTrigger reason )
	{		
		//! zoom disabled
		if (m_CursorState & STATE_ZOOM_RESTRICTED)
			return;
		
		// Check widget
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		array<Widget> outWidgets = {};
		WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), outWidgets);			
		foreach (Widget widget : outWidgets)
		{
			if (widget.IsInherited(ScrollLayoutWidget))
				return;
		}
		
		float targetPPU = m_MapEntity.GetTargetZoomPPU();
		value = value * m_fZoomMultiplierWheel;
		m_MapEntity.ZoomSmooth(targetPPU - targetPPU/2 * (value * 0.001), m_fZoomAnimTime, false);	// the const here is adjusting the value to match the input with zoom range
	}
};
