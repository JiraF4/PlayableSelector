modded class SCR_CameraEditorComponent
{
	vector m_vLastCameraPosition[4];

	override protected void EOnEditorDeactivate()
	{
		m_Camera.GetTransform(m_vLastCameraPosition);
		super.EOnEditorDeactivate();
	}
	
	void GetLastCameraTransform(out vector mat[4])
	{
		Math3D.MatrixCopy(m_vLastCameraPosition, mat);
	}
}