[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
modded class SCR_TeleportToCursorManualCameraComponent
{
	static SCR_TeleportToCursorManualCameraComponent PS_m_Instance;
	
	override bool EOnCameraInit()
	{
		PS_m_Instance = this;
		return super.EOnCameraInit();
	}
}