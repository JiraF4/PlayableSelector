enum PS_EReplayType
{
	WorldTime,
	CharacterMove,
	CharacterPossess,
	CharacterRegistration,
	PlayerRegistration,
	CharacterDamageStateChanged,
	//...
}

/*
	WorldTime
	int - timeMS	

	CharacterMove
	int - RplId
	float - position x
	float - position z
	float - rotation y
	
	CharacterPossess
	int - RplId
	int - playerId
	
	CharacterRegistration
	int - RplId
	int - factionKey length
	FactionKey - factionKey

	PlayerRegistration
	int - playerId
	int - player name length
	string - player name

	CharacterDamageStateChanged
	int - RplId
	int - EDamageState
*/