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
		RegV("MissionName");
		RegV("MissionAuthor");
		RegV("MissionDescription");
		
		RegV("MissionWeather");
		RegV("MissionData");
		
		RegV("MissionPath");
		RegV("WorldPath");
		RegV("ReplayPath");
		
		RegV("Descriptions");
		RegV("Factions");
		RegV("Vehicles");
		RegV("PlayersToPlayables");
		RegV("Players");
		RegV("DamageEvents");
		RegV("StateEvents");
		RegV("FactionResults");
		
		RegV("Kills");
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
		RegV("Title");
		RegV("DescriptionLayout");
		RegV("TextData");
		RegV("VisibleForFactions");
		RegV("EmptyFactionVisibility");
	}
}

class PS_MissionDataFaction : JsonApiStruct
{
	FactionKey Key;
	string Name;
	ref array<ref PS_MissionDataGroup> Groups = new array<ref PS_MissionDataGroup>;
	Color FactionColor;
	Color FactionOutlineColor;
	
	void PS_MissionDataFaction()
	{
		RegV("Key");
		RegV("Name");
		RegV("Groups");
		
		RegV("FactionColor");
		RegV("FactionOutlineColor");
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
		RegV("Callsign");
		RegV("CallsignName");
		RegV("Name");
		RegV("Playables");
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
		RegV("EntityId");
		RegV("GroupOrder");
		RegV("Name");
		RegV("RoleName");
		RegV("PrefabPath");
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
		RegV("EntityId");
		RegV("PrefabPath");
		RegV("EditableName");
		RegV("VehicleFactionKey");
	}
}
class PS_MissionDataPlayer : JsonApiStruct
{
	int PlayerId;
	string GUID;
	string Name;
	
	void PS_MissionDataPlayer()
	{
		RegV("PlayerId");
		RegV("GUID");
		RegV("Name");
	}
}

class PS_MissionDataPlayerToEntity : JsonApiStruct
{
	int PlayerId;
	int EntityId;
	
	void PS_MissionDataPlayerToEntity()
	{
		RegV("PlayerId");
		RegV("EntityId");
	}
}

class PS_MissionDataDamageEvent : JsonApiStruct
{
	int PlayerId;
	int TargetId;
	float DamageValue;
	int TargetState;
	int Time;
	
	void PS_MissionDataDamageEvent()
	{
		RegV("PlayerId");
		RegV("TargetId");
		RegV("DamageValue");
		RegV("TargetState");
		RegV("Time");
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
		RegV("ResultFactionKey");
		RegV("ResultName");
		RegV("ResultScore");
		RegV("Objectives");
	}
}

class PS_MissionDataObjective : JsonApiStruct
{
	string Name;
	bool Completed;
	int Score;
	
	void PS_MissionDataObjective()
	{
		RegV("Name");
		RegV("Completed");
		RegV("Score");
	}
}

class PS_MissionDataStateChangeEvent : JsonApiStruct
{
	int State;
	int Time;
	int SystemTime;
	
	void PS_MissionDataStateChangeEvent()
	{
		RegV("State");
		RegV("Time");
		RegV("SystemTime");
	}
}

class PS_MissionDataPlayerKill : JsonApiStruct
{
	int InstigatorId;
	int PlayerId;
	
	void PS_MissionDataPlayerKill()
	{
		RegV("InstigatorId");
		RegV("PlayerId");
	}
}