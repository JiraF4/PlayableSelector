[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_CutsceneManager : ScriptComponent
{	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_CutsceneManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_CutsceneManager.Cast(gameMode.FindComponent(PS_CutsceneManager));
		else
			return null;
	}
	
	
};