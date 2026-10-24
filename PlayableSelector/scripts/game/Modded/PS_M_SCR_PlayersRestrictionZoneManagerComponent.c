modded class SCR_PlayersRestrictionZoneManagerComponent
{
	//! Copy, not the vanilla member. Vanilla's set is a ref container that
	//! RemoveRestrictionZone() shrinks, and PS_FreezeManager.RemoveRestrictedZones_S() walks the
	//! result by index while deleting each zone entity - if that deletion unregisters the zone,
	//! the live set loses entries mid-loop and zones are skipped.
	set<SCR_EditorRestrictionZoneEntity> GetZones()
	{
		set<SCR_EditorRestrictionZoneEntity> zones = new set<SCR_EditorRestrictionZoneEntity>();
		foreach (SCR_EditorRestrictionZoneEntity zone : m_aRestrictionZones)
			zones.Insert(zone);

		return zones;
	}
	
	override protected void KillPlayerOutOfZone(int playerID, IEntity playerEntity)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager || freezeManager.IsFreezeTimeEnd() || freezeManager.IsHardFreezeActive() || freezeManager.GetCurrentFreezeTime() <= 2000.0)
			return;
		
		if (!playerEntity)
			return;
		
		/*
		PS_PlayableContainer playableComponent = PS_PlayableContainer.Cast(playerEntity.FindComponent(PS_PlayableContainer));
		if (!playableComponent)
			return;
		*/
		
		//super.KillPlayerOutOfZone(playerID, playerEntity);
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
