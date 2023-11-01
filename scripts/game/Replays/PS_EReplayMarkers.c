enum PS_EReplayType
{
	WorldTime,
	EntityMove,
	CharacterPossess,
	CharacterRegistration,
	VehicleRegistration,
	PlayerRegistration,
	EntityDamageStateChanged,
	CharacterBoardVehicle,
	CharacterUnBoardVehicle,
	ProjectileShoot,
	Explosion,
	//...
}

enum PS_EReplayVehicleType
{
	BTR70,
}

/*
	WorldTime
	int - timeMS	

	EntityMove
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
	
	VehicleRegistration
	int - RplId
	int - vehicleName length
	string - vehicleName
	EVehicleType - vehicleType
	int - factionKey length
	FactionKey - factionKey

	PlayerRegistration
	int - playerId
	int - player name length
	string - player name

	EntityDamageStateChanged
	int - RplId
	int - EDamageState
	
	CharacterBoardVehicle
	int - RplId vehicle id
	int - playerId

	CharacterUnBoardVehicle
	int - RplId vehicle id
	int - playerId

	ProjectileShoot
	int - RplId shoot entity
	float - hit position x
	float - hit position z

	Explosion
	float - hit position x
	float - hit position z
	float - Impulse distance
*/