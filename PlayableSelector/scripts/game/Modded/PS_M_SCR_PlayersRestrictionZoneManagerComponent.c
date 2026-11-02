modded class SCR_PlayersRestrictionZoneManagerComponent
{
	//! Server-only body history: vanilla clears m_PlayerEntity after an exit, so it cannot identify a replacement body.
	protected ref map<int, IEntity> m_mCharactersSeenInZone_S = new map<int, IEntity>();

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
		// Freeze zones restrict movement after the batch instead of killing on a boundary crossing.
	}

	/** @brief Publish movement state only after vanilla finishes geometry and its temporary data resets. */
	override protected void ZoneCheckUpdate(bool updateAll, SCR_EditorRestrictionZoneEntity removedZone = null, array<int> playerIDZoneMoved = null)
	{
		if (!Replication.IsServer() || !m_PlayerManager)
			return;

		array<int> players = {};
		int playerCount = m_PlayerManager.GetPlayers(players);
		int start = 0;
		if (!updateAll && m_iPlayerCheckIndex < playerCount)
			start = m_iPlayerCheckIndex;

		super.ZoneCheckUpdate(updateAll, removedZone, playerIDZoneMoved);

		// Removal/movement checks temporarily exclude a still-registered zone.
		if (playerCount == 0 || removedZone)
			return;

		int end = playerCount;
		if (!updateAll)
			end = m_iPlayerCheckIndex;

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		bool freezeActive = freezeManager && !m_aRestrictionZones.IsEmpty()
			&& freezeManager.GetFreezePhase() != PS_EFreezePhase.NOT_STARTED
			&& !freezeManager.IsFreezeTimeEnd();

		for (int i = start; i < end && i < playerCount; i++)
		{
			int playerID = players[i];
			bool restrict = false;
			PlayerController playerController = m_PlayerManager.GetPlayerController(playerID);
			PS_PlayableControllerComponent playableControllerComponent = null;
			if (playerController)
				playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

			ChimeraCharacter character = ChimeraCharacter.Cast(m_PlayerManager.GetPlayerControlledEntity(playerID));
			SCR_DamageManagerComponent damageManager = null;
			if (character)
				damageManager = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));

			SCR_PlayerRestrictionZoneData zoneData;
			if (m_aRestrictionZones.IsEmpty() || !character || !playableControllerComponent || !damageManager
				|| damageManager.GetState() == EDamageState.DESTROYED
				|| !m_PlayerRestrictionZoneData.Find(playerID, zoneData) || !zoneData)
			{
				m_mCharactersSeenInZone_S.Remove(playerID);
			}
			else
			{
				IEntity trackedCharacter = null;
				m_mCharactersSeenInZone_S.Find(playerID, trackedCharacter);
				if (trackedCharacter != character)
				{
					m_mCharactersSeenInZone_S.Remove(playerID);
					trackedCharacter = null;
				}

				if (zoneData.m_bInZone)
				{
					m_mCharactersSeenInZone_S.Set(playerID, character);
				}
				else if (freezeActive && trackedCharacter == character)
				{
					restrict = playableControllerComponent.IsOutFreezeTime();
					if (freezeManager.GetFreezePhase() == PS_EFreezePhase.ORDINARY && !freezeManager.IsHardFreezeActive() && freezeManager.GetCurrentFreezeTime() > 2000.0)
						restrict = true;
				}
			}

			RestrictMovement(playerID, restrict);
		}
	}

	override void RemoveRestrictionZone(SCR_EditorRestrictionZoneEntity zone)
	{
		super.RemoveRestrictionZone(zone);

		if (!Replication.IsServer() || !m_PlayerManager || !m_aRestrictionZones.IsEmpty())
			return;

		m_mCharactersSeenInZone_S.Clear();
		array<int> players = {};
		m_PlayerManager.GetPlayers(players);
		foreach (int playerID : players)
			RestrictMovement(playerID, false);
	}
	
	void ResetPlayerZoneData(int playerID)
	{
		m_mCharactersSeenInZone_S.Remove(playerID);
		SetPlayerZoneData(playerID, null, false, false, -1);
		RestrictMovement(playerID, false);
	}
	
	override protected void OnPlayerDisconnect(int playerID)
	{
		m_mCharactersSeenInZone_S.Remove(playerID);
		super.OnPlayerDisconnect(playerID);
	}

	void RestrictMovement(int playerID, bool restrict)
	{
		if (!Replication.IsServer() || !m_PlayerManager)
			return;

		PlayerController playerController = m_PlayerManager.GetPlayerController(playerID);
		
		if (!playerController)
			return;
		
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableControllerComponent)
			return;

		playableControllerComponent.SetOutFreezeTime(restrict);
	}
}
