modded class SCR_AIGroupPerception
{
	override protected void MarkEnemyOnMap(notnull SCR_AITargetInfo target)
	{
		// Suppress AI spot reports to preserve fog of war unless enabled by the mission maker
		if (GetGame())
		{
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode && !gameMode.AllowAiSpotReports())
				return;
		}

		super.MarkEnemyOnMap(target);
	}
}
