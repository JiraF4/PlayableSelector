[ComponentEditorProps(category: "GameScripted/Replays", description: "Writes replay data to binary file")]
class PS_ReplayWriterClass: ScriptComponentClass
{
};

class PS_ReplayWriter : ScriptComponent
{
	int m_iLastWorldTime;
	string m_sReplayFileName;

	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;
		if (!Replication.IsServer()) return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.GetOnPlayerConnected().Insert(WritePlayerRegistration);
		CreateFile();
	}

	void CreateFile()
	{
		if (!Replication.IsServer()) return;

		int year, month, day;
		System.GetYearMonthDay(year, month, day);
		auto date = string.Format("%1-%2-%3", year, month.ToString(2), day.ToString(2));

		int hour, minute, second;
		System.GetHourMinuteSecond(hour, minute, second);
		auto time = string.Format("%1-%2-%3", hour.ToString(2), minute.ToString(2), second.ToString(2));

		auto scenarioWorldName = GetScenarioWorldName();
		auto replayFolderName = "Replays";
		auto replayFileName = string.Format("%1_%2_%3.bin", date, time, SCR_FileIOHelper.SanitiseFileName(scenarioWorldName));

		auto replayFolderPath = string.Format("$profile:%1", replayFolderName);
		FileIO.MakeDirectory(replayFolderPath);
		m_sReplayFileName = string.Format("%1/%2", replayFolderPath, replayFileName);
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.WRITE);
		m_iLastWorldTime = 0;
	}

	string GetScenarioWorldName()
	{
		string worldPath;
		auto game = GetGame();
		auto missionHeader = game.GetMissionHeader();
		if (!missionHeader) {
			string missionHeaderPath;
			if (!System.GetCLIParam("MissionHeader", missionHeaderPath))
				return "Unknown";
			missionHeader = MissionHeader.ReadMissionHeader(missionHeaderPath);
		};
		if (!missionHeader) {
			worldPath = game.GetWorldFile();
		}
		else {
			ResourceName missionHeaderWorldPath = missionHeader.GetWorldPath();
			worldPath = missionHeaderWorldPath.GetPath();
		};

		auto worldPathTokens = new array<string>();
		worldPath.Split("/", worldPathTokens, true);
		auto worldFileName = worldPathTokens.Get(worldPathTokens.Count() - 1);
		return worldFileName.Substring(0, worldFileName.Length() - ".ent".Length());
	}

	static PS_ReplayWriter GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_ReplayWriter.Cast(gameMode.FindComponent(PS_ReplayWriter));
		else
			return null;
	}

	void TryInsertTimeStamp()
	{
		if (!Replication.IsServer()) return;
		int timeStamp = GetGame().GetWorld().GetWorldTime();
		if (m_iLastWorldTime == timeStamp) return;
		m_iLastWorldTime = timeStamp;
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.WorldTime, 1);
		replayFile.Write(timeStamp, 4);
	}

	void WriteEntityMove(RplId characteRplId, IEntity character)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		vector position = character.GetOrigin();
		vector rotation = character.GetYawPitchRoll();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.EntityMove, 1);
		replayFile.Write(characteRplId, 4);
		replayFile.Write(position[0], -1);
		replayFile.Write(position[2], -1);
		replayFile.Write(rotation[0], -1);
	}

	void WriteCharacterPossess(RplId characteRplId, int Player_Id)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterPossess, 1);
		replayFile.Write(characteRplId, 4);
		replayFile.Write(Player_Id, 4);
	}

	void WriteCharacterRegistration(RplId characteRplId, SCR_ChimeraCharacter character)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FactionKey factionKey = "";
		Faction faction = character.GetFaction();
		if (faction) factionKey = faction.GetFactionKey();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterRegistration, 1);
		replayFile.Write(characteRplId, 4);
		int factionKeyLength = factionKey.Length();
		replayFile.Write(factionKeyLength, 4);
		replayFile.Write(factionKey, factionKeyLength);
	}

	void WritePlayerRegistration(int Player_Id)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		string playerName = playerManager.GetPlayerName(Player_Id);

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.PlayerRegistration, 1);
		replayFile.Write(Player_Id, 4);
		int playerNameLength = playerName.Length();
		replayFile.Write(playerNameLength, 4);
		replayFile.Write(playerName, playerNameLength);
	}

	void WriteCharacterDamageStateChanged(RplId characteRplId, EDamageState state)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.EntityDamageStateChanged, 1);
		replayFile.Write(characteRplId, 4);
		replayFile.Write(state, 4);
	}

	void WriteVehicleRegistration(RplId vehicleRplId, string vehicleName, EVehicleType vehicleType, FactionKey factionKey)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.VehicleRegistration, 1);
		replayFile.Write(vehicleRplId, 4);
		int vehicleNameLength = vehicleName.Length();
		replayFile.Write(vehicleNameLength, 4);
		replayFile.Write(vehicleName, vehicleNameLength);
		replayFile.Write(vehicleType, 4);
		int factionKeyLength = factionKey.Length();
		replayFile.Write(factionKeyLength, 4);
		replayFile.Write(factionKey, factionKeyLength);
	}

	void WriteCharacterBoardVehicle(RplId vehicleRplId, int Player_Id)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterBoardVehicle, 1);
		replayFile.Write(vehicleRplId, 4);
		replayFile.Write(Player_Id, 4);
	}

	void WriteCharacterUnBoardVehicle(RplId vehicleRplId, int Player_Id)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterUnBoardVehicle, 1);
		replayFile.Write(vehicleRplId, 4);
		replayFile.Write(Player_Id, 4);
	}

	void WriteProjectileShoot(RplId entityId, float hitPositionX, float hitPositionz)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.ProjectileShoot, 1);
		replayFile.Write(entityId, 4);
		replayFile.Write(hitPositionX, 4);
		replayFile.Write(hitPositionz, 4);
	}

	void WriteExplosion(float hitPositionX, float hitPositionz, float impulseDistance)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.Explosion, 1);
		replayFile.Write(hitPositionX, 4);
		replayFile.Write(hitPositionz, 4);
		replayFile.Write(impulseDistance, 4);
	}

	void WriteEntityDelete(RplId characteRplId)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();

		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.EntityDelete, 1);
		replayFile.Write(characteRplId, 4);
	}
}
