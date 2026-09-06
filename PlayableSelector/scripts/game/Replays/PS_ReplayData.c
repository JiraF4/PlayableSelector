class ReplayState
{
	PS_EReplayType type;
}
class ReplayWorldTime: ReplayState
{
	int WorldTime;

	void ReplayWorldTime(int worldTime)
	{
		WorldTime = worldTime;
		type = PS_EReplayType.WorldTime;
	}
}
class EntityMove: ReplayState
{
	RplId EntityId;
	float PositionX;
	float PositionZ;
	float RotationY;

	void EntityMove(RplId entityId, float positionX, float positionZ, float rotationY)
	{
		EntityId = entityId;
		PositionX = positionX;
		PositionZ = positionZ;
		RotationY = rotationY;
		type = PS_EReplayType.EntityMove;
	}
}
class CharacterPossess: ReplayState
{
	RplId EntityId;
	int Player_Id;

	void CharacterPossess(RplId entityId, int playerId)
	{
		EntityId = entityId;
		Player_Id = playerId;
		type = PS_EReplayType.CharacterPossess;
	}
}
class CharacterRegistration: ReplayState
{
	RplId EntityId;
	FactionKey EntityFactionKey;

	void CharacterRegistration(RplId entityId, FactionKey entityFactionKey)
	{
		EntityId = entityId;
		EntityFactionKey = entityFactionKey;
		type = PS_EReplayType.CharacterRegistration;
	}
}
class PlayerRegistration: ReplayState
{
	int Player_Id;
	string PlayerName;

	void PlayerRegistration(int playerId, string playerName)
	{
		Player_Id = playerId;
		PlayerName = playerName;
		type = PS_EReplayType.PlayerRegistration;
	}
}
class EntityDamageStateChanged: ReplayState
{
	RplId EntityId;
	EDamageState DamageState;

	void EntityDamageStateChanged(RplId entityId, EDamageState damageState)
	{
		EntityId = entityId;
		DamageState = damageState;
		type = PS_EReplayType.EntityDamageStateChanged;
	}
}
class VehicleRegistration: ReplayState
{
	RplId EntityId;
	string VehicleName;
	EVehicleType VehicleType;
	FactionKey EntityFactionKey;

	void VehicleRegistration(RplId entityId, string vehicleName, EVehicleType vehicleType, FactionKey entityFactionKey)
	{
		EntityId = entityId;
		VehicleName = vehicleName;
		VehicleType = vehicleType;
		EntityFactionKey = entityFactionKey;
		type = PS_EReplayType.VehicleRegistration;
	}
}
class CharacterBoardVehicle: ReplayState
{
	RplId EntityId;
	int Player_Id;

	void CharacterBoardVehicle(RplId entityId, int playerId)
	{
		EntityId = entityId;
		Player_Id = playerId;
		type = PS_EReplayType.CharacterBoardVehicle;
	}
}
class CharacterUnBoardVehicle: ReplayState
{
	RplId EntityId;
	int vPlayerId;

	void CharacterUnBoardVehicle(RplId entityId, int playerId)
	{
		EntityId = entityId;
		vPlayerId = playerId;
		type = PS_EReplayType.CharacterUnBoardVehicle;
	}
}
class ProjectileShoot: ReplayState
{
	RplId EntityId;
	float PositionX;
	float PositionZ;

	void ProjectileShoot(RplId entityId, float positionX, float positionZ)
	{
		EntityId = entityId;
		PositionX = positionX;
		PositionZ = positionZ;
		type = PS_EReplayType.ProjectileShoot;
	}
}
class Explosion: ReplayState
{
	float PositionX;
	float PositionZ;
	float ImpulseDistance;

	void Explosion(float positionX, float positionZ, float impulseDistance)
	{
		PositionX = positionX;
		PositionZ = positionZ;
		ImpulseDistance = impulseDistance;
		type = PS_EReplayType.Explosion;
	}
}
class EntityDelete: ReplayState
{
	RplId EntityId;

	void EntityDelete(RplId entityId)
	{
		EntityId = entityId;
		type = PS_EReplayType.EntityDelete;
	}
}
