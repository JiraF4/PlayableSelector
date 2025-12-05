modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CutsceneMenu
}

//------------------------------------------------------------------------------------------------
class PS_CutsceneMenu : ChimeraMenuBase
{
	ChimeraWorld m_World;
	
	int m_PrevPointNum = 1;
	
	PS_Cutscene m_Cutscene;
	PS_CutsceneManager m_CutsceneManager;
	
	OverlayWidget m_wBlackScreen;
	TextWidget m_wLoadText;
	
	CinematicEntity m_CinematicEntity;
	bool m_bFadeOn = true;
	
	
	void RaiseEvent(string eventName)
	{
		array<string> events = {};
		eventName.Split("|", events, true);
		switch (events[0])
		{
			case "FadeOut":
				m_bFadeOn = true;
				break;
			case "FadeIn":
				m_bFadeOn = false;
				break;
			case "CreateLayout":
				CreateLayout(events[1]);
				break;
		}
	}
	
	void CreateLayout(string layoutName)
	{
		GetGame().GetWorkspace().CreateWidgets(layoutName, GetRootWidget());
	}
	
	override void OnMenuOpen()
	{
		m_CutsceneManager = PS_CutsceneManager.GetInstance();
		m_Cutscene = m_CutsceneManager.GetCutscene(0);
		
		m_wBlackScreen = OverlayWidget.Cast(GetRootWidget().FindAnyWidget("BlackScreen"));
		m_wLoadText = TextWidget.Cast(GetRootWidget().FindAnyWidget("LoadText"));
		m_World = GetGame().GetWorld();
		
		m_CinematicEntity = CinematicEntity.Cast(GetGame().GetWorld().FindEntityByName(m_Cutscene.m_sCutsceneEntityName));
		m_CinematicEntity.Play();
		GetGame().GetCallqueue().CallLater(PreloadStop, 200, false);
	}
	
	void PreloadStop()
	{
		m_CinematicEntity.Pause();
		GetGame().GetCallqueue().CallLater(ResumeAnimation, 6000);
	}
	
	void ResumeAnimation()
	{
		AudioSystem.PlayEventInitialize(m_Cutscene.m_sCutsceneSound);
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (entity)
			entity.GetWorldTransform(mat);
		AudioHandle handle = AudioSystem.PlayEvent(m_Cutscene.m_sCutsceneSound, "SOUND_VOICE", mat);
		AudioSystem.SetBoundingVolumeParams(handle, AudioSystem.BV_Sphere, 9999999, 9999999, 9999999);
		m_bFadeOn = false;
		m_wLoadText.SetVisible(false);
		m_CinematicEntity.Play();
	}
	
	override void OnMenuClose()
	{
		if (m_CinematicEntity)
			m_CinematicEntity.Stop();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (!m_bFadeOn)
			m_wBlackScreen.SetOpacity(Math.Max(0, m_wBlackScreen.GetOpacity() - tDelta*2.0));
		else
			m_wBlackScreen.SetOpacity(Math.Min(1, m_wBlackScreen.GetOpacity() + tDelta*2.0));
		super.OnMenuUpdate(tDelta);
		
		if (m_CinematicEntity.Isfinished())
		{
			m_wBlackScreen.SetVisible(true);
			m_wLoadText.SetVisible(true);
			m_wBlackScreen.SetOpacity(1);
			m_bFadeOn = false;
		}
	}
}