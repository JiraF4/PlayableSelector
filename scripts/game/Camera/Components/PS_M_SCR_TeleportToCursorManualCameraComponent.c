[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
modded class SCR_TeleportToCursorManualCameraComponent
{
	static SCR_TeleportToCursorManualCameraComponent s_PS_Instance;

	override bool EOnCameraInit()
	{
		s_PS_Instance = this;
		return super.EOnCameraInit();
	}
}
