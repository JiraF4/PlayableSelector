modded class SCR_MapMarkerSquadLeader
{
	override void OnCreateMarker()
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameModeCoop)
		{
			super.OnCreateMarker();
			return;
		}
		
		if (!gameModeCoop.GetDisableLeaderSquadMarkers())
		{
			super.OnCreateMarker();
			return;
		}
	}
}