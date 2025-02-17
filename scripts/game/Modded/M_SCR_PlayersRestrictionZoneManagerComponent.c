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
		
		/*
		PS_PlayableContainer playableComponent = PS_PlayableContainer.Cast(playerEntity.FindComponent(PS_PlayableContainer));
		if (!playableComponent)
			return;
		*/
		
		//super.KillPlayerOutOfZone(playerID, playerEntity);
		RestrictMovement(playerID);
	}
	
	void ResetPlayerZoneData(int playerID)
	{
		SetPlayerZoneData(playerID, null, false, false, -1);
	}
	
	void RestrictMovement(int playerID)
	{
		PlayerController playerController = m_PlayerManager.GetPlayerController(playerID);
		
		if (!playerController)
			return;
		
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(PS_PlayableControllerComponent));
		if (!playableControllerComponent)
			return;
		
		playableControllerComponent.SetOutFreezeTime(true);
	}
}
