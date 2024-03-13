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
				
				SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
				SCR_MapMarkersUI mapMarkersUI = SCR_MapMarkersUI.Cast(mapEnt.GetMapUIComponent(SCR_MapMarkersUI));
				
				SCR_MapMarkerMenuEntry markerEntry = new SCR_MapMarkerMenuEntry();
				markerEntry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM);
				mapMarkersUI.PS_OnEntryPerformed(markerEntry);
				
				float wX, wY, sX, sY;
				mapEnt.GetMapCursorWorldPosition(wX, wY);
				mapEnt.WorldToScreen(wX, wY, sX, sY);
				mapEnt.PanSmooth(sX, sY, 0.05);
				
				return;
			}
		}
		
		SCR_MapRadialUI mapRadial = SCR_MapRadialUI.GetInstance();
		if (!mapRadial)
			return;
		
		mapRadial.GetRadialController().OnInputOpen();
		mapRadial.GetRadialController().GetRadialMenu().PerformEntry(m_RootCategoryEntry);
	
	}
	
	void PS_OnEntryPerformed(SCR_MapMarkerMenuEntry entry)
	{
		OnEntryPerformed(entry);
	}
	
	override void OnDragWidget(Widget widget)
	{
		PS_GameModeCoop psGameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (psGameMode)
		{
			if (psGameMode.GetMarkersOnlyOnBriefing())
			{
				if (psGameMode.GetState() != SCR_EGameModeState.GAME)
					super.OnDragWidget(widget);
				return;
			}
		}
		super.OnDragWidget(widget);
	}
}