modded class SCR_PlayersRestrictionZoneManagerComponent
{
	ref set<SCR_EditorRestrictionZoneEntity> GetZones()
	{
		return m_aRestrictionZones;
	}
	
	override protected void KillPlayerOutOfZone(int playerID, IEntity playerEntity)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetElapsedTime() > (gameMode.m_iFreezeTime - 2000))
			return;
		
		if (!playerEntity)
			return;
		
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(playerEntity.FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return;
		
		super.KillPlayerOutOfZone(playerID, playerEntity);
	}
	
	void ResetPlayerZoneData(int playerID)
	{
		SetPlayerZoneData(playerID, null, false, false, -1);
	}
}
