class PS_MissionDataConfig : JsonApiStruct
{
	string MissinPath;
	ref array<ref PS_MissionDataDescription> Descriptions = new array<ref PS_MissionDataDescription>;
	ref array<ref PS_MissionDataFaction> Factions = new array<ref PS_MissionDataFaction>;
	
	void PS_MissionDataConfig()
	{
		RegV("MissinPath");
		RegV("Descriptions");
		RegV("Factions");
	}
}

class PS_MissionDataDescription : JsonApiStruct
{
	string Title;
	string DescriptionLayout;
	string TextData;
	ref array<FactionKey> VisibleForFactions;
	bool EmptyFactionVisibility;
	
	void PS_MissionDataConfig()
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
	
	void PS_MissionDataConfig()
	{
		RegV("Key");
		RegV("Name");
		RegV("Groups");
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
	int GroupOrder;
	string Name;
	string RoleName;
	
	void PS_MissionDataGroup()
	{
		RegV("GroupOrder");
		RegV("Name");
		RegV("RoleName");
	}
}