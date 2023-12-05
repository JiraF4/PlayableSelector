// Remove squad markers
modded class SCR_MapMarkerEntrySquadLeader
{
	override void RegisterMarker(SCR_MapMarkerSquadLeader marker, SCR_AIGroup group)
	{
		// If game mode coop check for flag
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetDisableLeaderSquadMarkers()) return;
		
		Print("DEBUG LINE | " + FilePath.StripPath(__FILE__) + ":" + __LINE__, LogLevel.ERROR);
		super.RegisterMarker(marker, group);
	}
}