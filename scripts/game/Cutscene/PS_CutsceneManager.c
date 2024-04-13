[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManager : ScriptComponent
{	
	ChimeraWorld m_World;
	
	[Attribute("", UIWidgets.EditBoxMultiline)]
	string m_sCutsceneData;
	
	[Attribute("")]
	ResourceName m_sCutsceneSound;
	
	int m_iCutsceneTime;
	int m_iCutscenePreloadTime = 6000;
	
	ref array<ref PS_CutscenePoint> m_CutscenePoints;
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_CutsceneManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_CutsceneManager.Cast(gameMode.FindComponent(PS_CutsceneManager));
		else
			return null;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		m_CutscenePoints = {};
		
		array<string> lines = {};
		m_sCutsceneData.Split("\n", lines, true);
		foreach (string line : lines)
		{
			array<string> parts = {};
			PS_CutscenePoint point = new PS_CutscenePoint();
			point.m_fTime = (float)m_iStepTime * 0.001;
			line.Split(" ", parts, false);
			
			for (int x = 0; x < 4; x++)
				for (int y = 0; y < 3; y++)
					point.m_Mat[x][y] = parts[x*3 + y].ToFloat();
			
			m_CutscenePoints.Insert(point);
		}
		m_iCutsceneTime = lines.Count() * m_iStepTime;
		
		if (!Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().Call(AddCutsceneAction);
	}
	
	void PreloadWorld()
	{
		if (m_CutscenePoints && m_CutscenePoints.Count() > 0)
			GetGame().GetWorld().SchedulePreload(m_CutscenePoints[0].m_Mat[3], 100);
	}
	
	int GetPreloadTime()
		return m_iCutscenePreloadTime;
	
	void AddCutsceneAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("cur");
		invoker.Insert(CutsceneRecord_Callback);
		
		invoker = chatPanelManager.GetCommandInvoker("cup");
		invoker.Insert(CutscenePlay_Callback);
		
		invoker = chatPanelManager.GetCommandInvoker("cuj");
		invoker.Insert(CutsceneJump_Callback);
	}
	
	static const int m_iStepTime = 150;
	static const string m_sCutsceneFilePath = "$profile:PS_tempCutScene.json";
	bool m_bRecording = false;
	void CutsceneRecord_Callback(SCR_ChatPanel panel, string data)
	{
		if (!m_bRecording)
			GetGame().GetCallqueue().CallLater(RecordCamera, m_iStepTime, true);
		else
			GetGame().GetCallqueue().Remove(RecordCamera);
		
		m_bRecording = !m_bRecording;
	}
	
	void RecordCamera()
	{
		FileHandle cutsceneFile = FileIO.OpenFile(m_sCutsceneFilePath, FileMode.APPEND);
		
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		
		vector mat[4];
		camera.GetTransform(mat);
		
		string line = string.Format("%1 %2 %3 %4", mat[0], mat[1], mat[2], mat[3]);
		line.Replace("<", "");
		line.Replace(">", "");
		line.Replace(",", " ");
		cutsceneFile.WriteLine(
			line
		);
	}
	
	int m_iStartTime;
	ref array<ref PS_CutscenePoint> m_points;
	float m_fMultiply = 1.0;
	void CutscenePlay_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "")
			m_fMultiply = 1.0;
		else
			m_fMultiply = data.ToFloat();
		m_iStartTime = GetGame().GetWorld().GetWorldTime();
		m_points = {};
		
		FileHandle cutsceneFile = FileIO.OpenFile(m_sCutsceneFilePath, FileMode.READ);
		while (!cutsceneFile.IsEOF())
		{
			string line;
			cutsceneFile.ReadLine(line);
			
			if (line == "")
				continue;
			
			array<string> parts = {};
			PS_CutscenePoint point = new PS_CutscenePoint();
			line.Split(" ", parts, false);
			
			for (int x = 0; x < 4; x++)
				for (int y = 0; y < 3; y++)
					point.m_Mat[x][y] = parts[x*3 + y].ToFloat();
			
			m_points.Insert(point);
		}
		
		GetGame().GetCallqueue().CallLater(PlayCamera, 0, true);
	}
	
	void CutsceneJump_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "")
			return;
		
		int pos = data.ToInt();
		if (pos == 0)
			return;
		
		FileHandle cutsceneFile = FileIO.OpenFile(m_sCutsceneFilePath, FileMode.READ);
		while (pos > 0)
		{
			string line;
			cutsceneFile.ReadLine(line);
			pos--;
			if (cutsceneFile.IsEOF())
				return;
		}
		
		string line;
		cutsceneFile.ReadLine(line);
		
		array<string> parts = {};
			PS_CutscenePoint point = new PS_CutscenePoint();
		line.Split(" ", parts, false);
		
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 3; y++)
				point.m_Mat[x][y] = parts[x*3 + y].ToFloat();
		
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		camera.SetTransform(point.m_Mat);
	}
	
	void PlayCamera()
	{
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		
		int currentTime = GetGame().GetWorld().GetWorldTime();
		int diffTime = (currentTime - m_iStartTime) * m_fMultiply;
		int firstSegment = diffTime / m_iStepTime;
		float timePart = Math.Mod(diffTime, m_iStepTime) / (float)m_iStepTime;
		
		if (firstSegment + 1 >= m_points.Count())
		{
			GetGame().GetCallqueue().Remove(PlayCamera);
			return;	
		}	
		
		PS_CutscenePoint pointFirst = m_points[firstSegment];
		PS_CutscenePoint pointSecond = m_points[firstSegment + 1];
		
		vector mat[4]
		float quatPrev[4];
		float quatNext[4];
		float quat[4];
		vector positionPrev;
		vector positionNext;
		vector position;
		
		positionPrev = pointFirst.m_Mat[3];
		positionNext = pointSecond.m_Mat[3];
		Math3D.MatrixToQuat(pointFirst.m_Mat, quatPrev);
		Math3D.MatrixToQuat(pointSecond.m_Mat, quatNext);
		
		position = vector.Lerp(positionPrev, positionNext, timePart);
		Math3D.QuatLerp(quat, quatPrev, quatNext, timePart);
		
		mat[3] = position;
		Math3D.QuatToMatrix(quat, mat);
		
		camera.SetTransform(mat);
	}
	
	ResourceName GetCutsceneSound()
	{
		return m_sCutsceneSound;
	}
	
	void GetPoints(out notnull array<ref PS_CutscenePoint> pointsOut)
	{
		foreach (PS_CutscenePoint point : m_CutscenePoints)
			pointsOut.Insert(point);
	}
	
	int GetCutsceneTime()
	{
		return m_iCutsceneTime + m_iCutscenePreloadTime;
	}
};