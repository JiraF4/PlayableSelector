class PS_MissionDataConfig : JsonApiStruct
{
	string MissionName;
	string MissionAuthor;
	string MissionDescription;
	
	string MissionWeather;
	string MissionWeatherIcon;
	float MissionDayTime;
	
	string MissionPath;
	string WorldPath;
	string ReplayPath;
	
	ref array<ref PS_MissionDataDescription> Descriptions = new array<ref PS_MissionDataDescription>;
	ref array<ref PS_MissionDataFaction> Factions = new array<ref PS_MissionDataFaction>;
	ref array<ref PS_MissionDataVehicle> Vehicles = new array<ref PS_MissionDataVehicle>;
	ref array<ref PS_MissionDataPlayerToEntity> PlayersToPlayables = new array<ref PS_MissionDataPlayerToEntity>;
	ref array<ref PS_MissionDataPlayer> Players = new array<ref PS_MissionDataPlayer>;
	ref array<ref PS_MissionDataDamageEvent> DamageEvents = new array<ref PS_MissionDataDamageEvent>;
	ref array<ref PS_MissionDataStateChangeEvent> StateEvents = new array<ref PS_MissionDataStateChangeEvent>;
	ref array<ref PS_MissionDataFactionResult> FactionResults = new array<ref PS_MissionDataFactionResult>;
	ref array<ref PS_MissionDataPlayerKill> Kills = new array<ref PS_MissionDataPlayerKill>;
	
	void PS_MissionDataConfig()
	{
		RegAll();
	}
}

class PS_MissionDataDescription : JsonApiStruct
{
	string Title;
	string DescriptionLayout;
	string TextData;
	ref array<FactionKey> VisibleForFactions;
	bool EmptyFactionVisibility;
	
	void PS_MissionDataDescription()
	{
		RegAll();
	}
}

class PS_MissionDataFaction : JsonApiStruct
{
	FactionKey Key;
	string Name;
	ref array<ref PS_MissionDataGroup> Groups = new array<ref PS_MissionDataGroup>;
	string FactionColor;
	string FactionOutlineColor;
	
	void PS_MissionDataFaction()
	{
		RegAll();
	}
}

class PS_MissionDataGroup : JsonApiStruct
{
	int Callsign;
	string CallsignName;
	string Name;
	ref array<ref PS_MissionDataPlayable> Playables = new array<ref PS_MissionDataPlayable>;
	
	void PS_MissionDataGroup()
	{
		RegAll();
	}
}

class PS_MissionDataPlayable : JsonApiStruct
{
	int EntityId;
	int GroupOrder;
	string Name;
	string RoleName;
	string PrefabPath;
	
	void PS_MissionDataPlayable()
	{
		RegAll();
	}
}

class PS_MissionDataVehicle : JsonApiStruct
{
	int EntityId;
	string PrefabPath;
	string EditableName;
	string VehicleFactionKey;
	
	void PS_MissionDataVehicle()
	{
		RegAll();
	}
}
class PS_MissionDataPlayer : JsonApiStruct
{
	int m_iPlayerId;
	string GUID;
	string Name;
	
	void PS_MissionDataPlayer()
	{
		RegAll();
	}
}

class PS_MissionDataPlayerToEntity : JsonApiStruct
{
	int m_iPlayerId;
	int EntityId;
	
	void PS_MissionDataPlayerToEntity()
	{
		RegAll();
	}
}

class PS_MissionDataDamageEvent : JsonApiStruct
{
	int m_iPlayerId;
	int TargetId;
	float DamageValue;
	int TargetState;
	int Time;
	
	void PS_MissionDataDamageEvent()
	{
		RegAll();
	}
}

class PS_MissionDataFactionResult : JsonApiStruct
{
	FactionKey ResultFactionKey;
	string ResultName;
	int ResultScore;
	ref array<ref PS_MissionDataObjective> Objectives = new array<ref PS_MissionDataObjective>();
	
	void PS_MissionDataFactionResult()
	{
		RegAll();
	}
}

class PS_MissionDataObjective : JsonApiStruct
{
	string Name;
	bool Completed;
	int Score;
	
	void PS_MissionDataObjective()
	{
		RegAll();
	}
}

class PS_MissionDataStateChangeEvent : JsonApiStruct
{
	int State;
	int Time;
	int SystemTime;
	
	void PS_MissionDataStateChangeEvent()
	{
		RegAll();
	}
}

class PS_MissionDataPlayerKill : JsonApiStruct
{
	int InstigatorId;
	int m_iPlayerId;
	int Time;
	int SystemTime;
	
	void PS_MissionDataPlayerKill()
	{
		RegAll();
	}
}