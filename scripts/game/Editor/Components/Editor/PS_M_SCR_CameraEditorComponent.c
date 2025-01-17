modded class SCR_CameraEditorComponent
{
	vector m_vLastCameraPosition[4];

	override protected void CreateCamera()
	{
		if (GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu))
		{
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.SpectatorMenu);
			GetGame().GetCameraManager().CurrentCamera().GetTransform(m_vLastCameraPosition);
			super.CreateCamera();
			m_Camera.SetTransform(m_vLastCameraPosition);
			return;
		}
		super.CreateCamera();
	}
	override protected void EOnEditorDeactivate()
	{
		if (m_Camera)
			m_Camera.GetTransform(m_vLastCameraPosition);
		super.EOnEditorDeactivate();
	}

	void GetLastCameraTransform(out vector mat[4])
	{
		Math3D.MatrixCopy(m_vLastCameraPosition, mat);
	}
}
