// Remove squad markers
modded class SCR_MapMarkerManagerComponent
{
	override protected void EOnInit(IEntity owner)
	{	
		if (!m_pGameMode.IsMaster())	// init server logic below
			return;
		
		// If game mode coop check for flag
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetDisableLeaderSquadMarkers()) return;
		
		array<ref SCR_MapMarkerEntryConfig> entryCfgs = m_MarkerCfg.GetMarkerEntryConfigs();
		foreach ( SCR_MapMarkerEntryConfig cfg : entryCfgs )
		{
			SCR_MapMarkerEntryDynamic entryDynamic = SCR_MapMarkerEntryDynamic.Cast(cfg);
			if (entryDynamic)
				entryDynamic.InitServerLogic();
		}
		
		SCR_FactionManager mgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (mgr)
			mgr.GetOnPlayerFactionChanged_S().Insert(OnPlayerFactionChanged_S);
	}
}