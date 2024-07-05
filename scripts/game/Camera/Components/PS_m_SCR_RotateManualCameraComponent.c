[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
modded class SCR_RotateManualCameraComponent : SCR_BaseManualCameraComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		if (!param.isManualInputEnabled)
			return;
		
		float yaw = GetInputManager().GetActionValue("ManualCameraRotateYaw");
		float pitch = GetInputManager().GetActionValue("ManualCameraRotatePitch");
		
		//--- With mouse, the input is activated only every second frame. Correct it by using previous value.
		if (GetInputManager().IsUsingMouseAndKeyboard())
		{
			float yawPrev = m_fYaw;
			m_fYaw = yaw;
			if (yaw == 0 && yawPrev != 0) yaw = yawPrev;
			
			float pitchPrev = m_fPitch;
			m_fPitch = pitch;
			if (pitch == 0 && pitchPrev != 0) pitch = pitchPrev;
		}

		if (yaw == 0 && pitch == 0)
			return;
		
		PS_SpectatorMenu.ResetTarget();
		
		param.rotDelta += Vector(yaw, pitch, 0) * m_fSpeed * param.fov / Math.Max(GetCameraEntity().GetDefaultFOV(), 1);
		param.isManualInput = true;
		param.isDirty = true;
	}
}
