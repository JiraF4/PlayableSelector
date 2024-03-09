// Enable markers only for briefing if gamemode coop and flag is set
modded class SCR_MapRadialUI
{
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetMarkersOnlyOnBriefing())
			return;
		if (m_RadialMenu && m_RadialMenu.IsOpened())
			return;
		
		m_RadialController.OnInputOpen();
	}
}