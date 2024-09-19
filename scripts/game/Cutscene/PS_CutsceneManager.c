[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
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
};

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