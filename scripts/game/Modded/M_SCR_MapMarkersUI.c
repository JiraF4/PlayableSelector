// Enable markers only for briefing if gamemode coop and flag is set
modded class SCR_MapMarkersUI
{
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		PS_GameModeCoop psGameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (psGameMode)
		{
			if (psGameMode.GetMarkersOnlyOnBriefing())
			{
				if (psGameMode.GetState() != SCR_EGameModeState.BRIEFING) return;
				PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
				PlayerController playerController = GetGame().GetPlayerController();
				if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
			}
		}
		
		SCR_MapRadialUI mapRadial = SCR_MapRadialUI.GetInstance();
		if (!mapRadial)
			return;
		
		mapRadial.GetRadialController().OnInputOpen();
		mapRadial.GetRadialController().GetRadialMenu().PerformEntry(m_RootCategoryEntry);
	
	}
}