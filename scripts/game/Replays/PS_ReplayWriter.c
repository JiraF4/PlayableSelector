[ComponentEditorProps(category: "GameScripted/Callsign", description: "")]
class PS_ReplayWriterClass: ScriptComponentClass
{
	
};


// Replay file main handler
class PS_ReplayWriter : ScriptComponent
{
	int m_iLastWorldTime;
	string m_sReplayFileName;
	//FileHandle m_fReplayFile;
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;
		CreateFile();
	}
	void CreateFile()
	{
		if (!Replication.IsServer()) return;
		string timeStamp = System.GetUnixTime().ToString();
		m_sReplayFileName = "$profile:Replays/Replay" + timeStamp + ".bin";
		FileIO.MakeDirectory("$profile:Replays");
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.WRITE);
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_ReplayWriter GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_ReplayWriter.Cast(gameMode.FindComponent(PS_ReplayWriter));
		else
			return null;
	}
	
	// PS_EReplayType.WorldTime
	void TryInsertTimeStamp()
	{
		if (!Replication.IsServer()) return;
		int timeStamp = GetGame().GetWorld().GetWorldTime();
		if (m_iLastWorldTime != timeStamp) return;
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.WorldTime, 1); // only one byte for type
		replayFile.Write(timeStamp, 4);
	}
	
	// PS_EReplayType.CharacterMove
	void WriteCharacterMove(RplId characteRplId, IEntity character)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();
		
		vector position = character.GetOrigin();
		vector rotation = character.GetYawPitchRoll();
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterMove, 1); // only one byte for type
		replayFile.Write(characteRplId, 4); // rplId
		replayFile.Write(position[0], -1); // position x
		replayFile.Write(position[2], -1); // position z
		replayFile.Write(rotation[0], -1); // rotation y
	}
	
	// PS_EReplayType.CharacterPossess
	void WriteCharacterPossess(RplId characteRplId, int PlayerId)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterPossess, 1); // only one byte for type
		replayFile.Write(characteRplId, 4); // rplId
		replayFile.Write(PlayerId, 4); // position x
	}
	
	// PS_EReplayType.CharacterRegistration
	void WriteCharacterRegistration(RplId characteRplId, SCR_ChimeraCharacter character)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();
		
		FactionKey factionKey = character.GetFaction().GetFactionKey();
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterRegistration, 1); // only one byte for type
		replayFile.Write(characteRplId, 4); // rplId
		int factionKeyLength = factionKey.Length();
		replayFile.Write(factionKeyLength, 4); // factionKey
		replayFile.Write(factionKey, factionKeyLength); // factionKey
	}
	
	// PS_EReplayType.PlayerRegistration
	void WritePlayerRegistration(int PlayerId)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		string playerName = playerManager.GetPlayerName(PlayerId);
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.PlayerRegistration, 1); // only one byte for type
		replayFile.Write(PlayerId, 4); // playerId
		int playerNameLength = playerName.Length();
		replayFile.Write(playerNameLength, 4); // playerName
		replayFile.Write(playerName, playerNameLength); // playerName
	}
	
	void WriteCharacterDamageStateChanged(RplId characteRplId, EDamageState state)
	{
		if (!Replication.IsServer()) return;
		TryInsertTimeStamp();
		
		FileHandle replayFile = FileIO.OpenFile(m_sReplayFileName, FileMode.APPEND);
		replayFile.Write(PS_EReplayType.CharacterDamageStateChanged, 1); // only one byte for type
		replayFile.Write(characteRplId, 4); // rplId
		replayFile.Write(state, 4); // EDamageState
	}
}