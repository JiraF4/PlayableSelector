modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CutsceneMenu
}

//------------------------------------------------------------------------------------------------
class PS_CutsceneMenu : ChimeraMenuBase
{
	ChimeraWorld m_World;
	CameraManager m_CameraManager;
	CameraBase m_CameraBase;
	CameraBase m_CameraBaseOld;
	
	float m_fStartTime;
	float m_fPointTime;
	float m_fNextPointTime;
	PS_CuscenePoint m_PrevPoint;
	PS_CuscenePoint m_NextPoint;
	int m_PrevPointNum = 1;
	ref array<ref PS_CuscenePoint> m_aCuscenePoints = {};
	
	override void OnMenuOpen()
	{
		m_CameraManager = GetGame().GetCameraManager();
		m_World = GetGame().GetWorld();
		
		EntitySpawnParams params = new EntitySpawnParams();
		Math3D.MatrixIdentity4(params.Transform);
		m_CameraBase = CameraBase.Cast(GetGame().SpawnEntity(CameraBase, m_World, params));
		
		m_CameraBaseOld = m_CameraManager.CurrentCamera();
		m_CameraManager.SetCamera(m_CameraBase);
		
		m_fStartTime = m_World.GetWorldTime() / 1000;
		
		PS_CuscenePoint point1 = new PS_CuscenePoint();
		m_World.FindEntityByName("Test1").GetTransform(point1.m_Mat);
		point1.m_fTime = 3;
		m_aCuscenePoints.Insert(point1);
		PS_CuscenePoint point2 = new PS_CuscenePoint();
		m_World.FindEntityByName("Test2").GetTransform(point2.m_Mat);
		point2.m_fTime = 3;
		m_aCuscenePoints.Insert(point2);
		PS_CuscenePoint point3 = new PS_CuscenePoint();
		m_World.FindEntityByName("Test3").GetTransform(point3.m_Mat);
		point3.m_fTime = 1;
		m_aCuscenePoints.Insert(point3);
		
		m_PrevPoint = m_aCuscenePoints[0];
		m_NextPoint = m_aCuscenePoints[1];
		m_fNextPointTime = m_PrevPoint.m_fTime;
		m_fPointTime = m_PrevPoint.m_fTime;
	}
	
	override void OnMenuClose()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(m_CameraBase);
		m_CameraManager.SetCamera(m_CameraBaseOld);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		float time = (m_World.GetWorldTime() / 1000) - m_fStartTime;
		
		if (time > m_fNextPointTime)
		{
			m_PrevPointNum++;
			
			m_PrevPoint = m_NextPoint;
			if (m_PrevPointNum < m_aCuscenePoints.Count())
				m_NextPoint = m_aCuscenePoints[m_PrevPointNum];
			
			m_fNextPointTime += m_PrevPoint.m_fTime;
			m_fPointTime = m_PrevPoint.m_fTime;
		}
		
		float timePart = 1.0 + ((time - m_fNextPointTime) / m_fPointTime);
		
		vector mat[4]
		float quatPrev[4];
		float quatNext[4];
		float quat[4];
		vector positionPrev;
		vector positionNext;
		vector position;
		
		positionPrev = m_PrevPoint.m_Mat[3];
		positionNext = m_NextPoint.m_Mat[3];
		Math3D.MatrixToQuat(m_PrevPoint.m_Mat, quatPrev);
		Math3D.MatrixToQuat(m_NextPoint.m_Mat, quatNext);
		
		position = vector.Lerp(positionPrev, positionNext, timePart);
		Math3D.QuatLerp(quat, quatPrev, quatNext, timePart);
		
		mat[3] = position;
		Math3D.QuatToMatrix(quat, mat);
		
		m_CameraBase.SetTransform(mat);
	}
}

class PS_CuscenePoint
{
	vector m_Mat[4];
	float m_fTime;
}