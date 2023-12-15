modded enum ChimeraMenuPreset
{
	LittleInventoryTestMenu
}

class PS_LittleInventoryTestMenu : ChimeraMenuBase
{
	override void OnMenuInit() 
	{
		
	}
}

modded class SCR_BaseGameMode
{
	override void OnGameStart()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.LittleInventoryTestMenu);
		super.OnGameStart();
	}
}