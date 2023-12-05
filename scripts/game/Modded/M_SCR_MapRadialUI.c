// Enable markers only for briefing if gamemode coop and flag is set
modded class SCR_MapRadialUI
{
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetMarkersOnlyOnBriefing())
		{
			if (gameModeCoop.GetState() != SCR_EGameModeState.BRIEFING) return;
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			PlayerController playerController = GetGame().GetPlayerController();
			if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		}
		if (m_RadialMenu && m_RadialMenu.IsOpened())
			return;
		
		m_RadialController.OnInputOpen();
	}
}