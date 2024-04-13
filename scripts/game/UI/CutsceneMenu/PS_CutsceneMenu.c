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
	PS_CutscenePoint m_PrevPoint;
	PS_CutscenePoint m_NextPoint;
	int m_PrevPointNum = 1;
	ref array<ref PS_CutscenePoint> m_aCutscenePoints = {};
	bool m_bSoundStarted;
	
	PS_CutsceneManager m_CutsceneManager;
	
	OverlayWidget m_wBlackScreen;
	
	override void OnMenuOpen()
	{
		m_wBlackScreen = OverlayWidget.Cast(GetRootWidget().FindAnyWidget("BlackScreen"));
		
		m_CameraManager = GetGame().GetCameraManager();
		m_World = GetGame().GetWorld();
		
		EntitySpawnParams params = new EntitySpawnParams();
		Math3D.MatrixIdentity4(params.Transform);
		m_CameraBase = CameraBase.Cast(GetGame().SpawnEntity(CameraBase, m_World, params));
		
		m_CameraBaseOld = m_CameraManager.CurrentCamera();
		m_CameraManager.SetCamera(m_CameraBase);
		
		m_fStartTime = m_World.GetWorldTime() / 1000;
		
		m_CutsceneManager = PS_CutsceneManager.GetInstance();
		m_CutsceneManager.GetPoints(m_aCutscenePoints);
		m_bSoundStarted = false;
		
		m_PrevPoint = m_aCutscenePoints[0];
		m_NextPoint = m_aCutscenePoints[1];
		m_fNextPointTime = m_PrevPoint.m_fTime;
		m_fPointTime = m_PrevPoint.m_fTime;
	}
	
	override void OnMenuClose()
	{
		if (m_CameraBase)
			delete m_CameraBase;
		m_CameraManager.SetPreviousCamera();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		float time = (m_World.GetWorldTime() / 1000) - m_fStartTime - (m_CutsceneManager.GetPreloadTime()/1000);
		
		if (time < 0)
		{
			m_wBlackScreen.SetOpacity(Math.AbsFloat(time) * 4.0);
			m_CameraBase.SetTransform(m_aCutscenePoints[0].m_Mat);
			return;
		}
		else
		{
			if (!m_bSoundStarted)
			{
				m_wBlackScreen.SetVisible(false);
		
				ResourceName sound = m_CutsceneManager.GetCutsceneSound();
				if (sound != "")
					AudioSystem.PlaySound(sound);
				m_bSoundStarted = true;
			}
		}
		
		if (time > m_fNextPointTime)
		{
			m_PrevPointNum++;
			
			m_PrevPoint = m_NextPoint;
			if (m_PrevPointNum < m_aCutscenePoints.Count())
				m_NextPoint = m_aCutscenePoints[m_PrevPointNum];
			
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

class PS_CutscenePoint
{
	vector m_Mat[4];
	float m_fTime;
}