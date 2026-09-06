modded class SCR_MapMarkerEntryConfig
{
	override void InitClientSettings(SCR_MapMarkerBase marker, SCR_MapMarkerWidgetComponent widgetComp, bool skipProfanityFilter = false)
	{
		if (!marker || !widgetComp)
			return;

		int ownerID = marker.GetMarkerOwnerID();
		if (ownerID <= 0)
			return;

		PlayerController localPC = GetGame().GetPlayerController();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(localPC);
		ImageWidget icon = widgetComp.GetAuthorPlatformIcon();
		PlayerManager manager = GetGame().GetPlayerManager();
		if (playerController && icon)
		{
			PlatformKind kind = PlatformKind.STEAM;
			if (manager)
				kind = manager.GetPlatformKind(ownerID);
			playerController.SetPlatformImageToKind(kind, icon, showOnPC: true, showOnXbox: true);
		}

		if (localPC && localPC.GetPlayerId() == ownerID)
		{
			widgetComp.SetModeIcon(true, marker.GetMarkerID() != -1);
			widgetComp.SetAuthorVisible(false);
		}
		else
		{
			widgetComp.SetAuthorVisible(true);
		}

		SCR_PlayerNamesFilterCache cache = SCR_PlayerNamesFilterCache.GetInstance();
		if (cache)
			widgetComp.SetAuthor(cache.GetPlayerDisplayName(ownerID));
		else if (manager)
			widgetComp.SetAuthor(manager.GetPlayerName(ownerID));
		else
			widgetComp.SetAuthor(ownerID.ToString());
	}
}
