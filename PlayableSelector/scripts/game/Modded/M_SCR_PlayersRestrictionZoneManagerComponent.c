modded class SCR_PlayersRestrictionZoneManagerComponent
{
	ref set<SCR_EditorRestrictionZoneEntity> GetZones()
	{
		return m_aRestrictionZones;
	}
	
	override protected void KillPlayerOutOfZone(int playerID, IEntity playerEntity)
	{
		if (!playerEntity)
			return;
		
		RestrictMovement(playerID, true);
	}
	
	override void SetPlayerZoneData(int playerID, IEntity playerEntity, bool inZone, bool inWarningZone, ERestrictionZoneWarningType warningType, vector zoneCenter = vector.Zero, float warningRadiusSq = -1, float zoneRadiusSq = -1)
	{	
		super.SetPlayerZoneData(playerID, playerEntity, inZone, inWarningZone, warningType, zoneCenter, warningRadiusSq, zoneRadiusSq);
		
		if (inZone)
			RestrictMovement(playerID, false);
	}
	
	void ResetPlayerZoneData(int playerID)
	{
		SetPlayerZoneData(playerID, null, false, false, -1);
		RestrictMovement(playerID, false);
	}
	
	void RestrictMovement(int playerID, bool restrict)
	{
		PlayerController playerController = m_PlayerManager.GetPlayerController(playerID);
		
		if (!playerController)
			return;
		
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableControllerComponent)
			return;
		
		playableControllerComponent.SetOutFreezeTime(restrict);
	}
}
