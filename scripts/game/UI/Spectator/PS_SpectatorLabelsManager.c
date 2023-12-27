//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelsManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelsManager : ScriptComponent
{
	ref array<PS_SpectatorLabel> m_aSpectatorLabels = new array<PS_SpectatorLabel>();
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_SpectatorLabelsManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_SpectatorLabelsManager.Cast(gameMode.FindComponent(PS_SpectatorLabelsManager));
		else
			return null;
	}
	
	void RegistrateLabel(PS_SpectatorLabel label)
	{
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("RegistrateLabel " + label.ToString());
		Print("RegistrateLabel " + label.ToString());
		Print("RegistrateLabel " + label.ToString());
		Print("RegistrateLabel " + label.ToString());
		Print("RegistrateLabel " + label.ToString());
		m_aSpectatorLabels.Insert(label);
	}
	
	void UnRegistrateLabel(PS_SpectatorLabel label)
	{
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("----------------------------------------");
		Print("UnRegistrateLabel " + label.ToString());
		Print("UnRegistrateLabel " + label.ToString());
		Print("UnRegistrateLabel " + label.ToString());
		Print("UnRegistrateLabel " + label.ToString());
		Print("UnRegistrateLabel " + label.ToString());
		m_aSpectatorLabels.RemoveItem(label);
	}
}