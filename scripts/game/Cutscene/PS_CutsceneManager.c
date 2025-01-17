[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManagerClass : ScriptComponentClass
{

}


class PS_CutsceneManager : ScriptComponent
{
	ChimeraWorld m_World;

	[Attribute("")]
	ref array<ref PS_Cutscene> m_aCutscenes;

	// more singletons for singletons god, make our spagetie kingdom great
	static PS_CutsceneManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_CutsceneManager.Cast(gameMode.FindComponent(PS_CutsceneManager));
		else
			return null;
	}

	void RaiseEvent(string eventName)
	{
		PS_CutsceneMenu cutsceneMenu = PS_CutsceneMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CutsceneMenu));
		if (cutsceneMenu)
			cutsceneMenu.RaiseEvent(eventName);
	}

	void RunCutscene(int num)
	{
		CinematicEntity cinematicEntity = CinematicEntity.Cast(GetGame().GetWorld().FindEntityByName(GetCutscene(num).m_sCutsceneEntityName));
		cinematicEntity.Play();
	}

	override protected void OnPostInit(IEntity owner)
	{
		//m_iCutsceneTime = 102000;
	}

	PS_Cutscene GetCutscene(int i)
	{
		if (m_aCutscenes.Count() <= i)
			return null;
		return m_aCutscenes[i];
	}

	int GetCutsceneTime()
	{
		return m_aCutscenes[0].m_iCutsceneTime;
	}
}

[CinematicTrackAttribute(name:"Cutscene manager events")]
class PS_CutsceneManagerEventsTrack : CinematicTrackBase
{
	[Attribute("")]
	string m_sEventName;

	[CinematicEventAttribute()]
	void RaiseEvent()
	{
		PS_CutsceneManager.GetInstance().RaiseEvent(m_sEventName);
	}
}

[BaseContainerProps("")]
class PS_Cutscene
{
	[Attribute("")]
	string m_sCutsceneEntityName;
	[Attribute("")]
	ResourceName m_sCutsceneSound;
	[Attribute("")]
	int m_iCutsceneTime;
}
