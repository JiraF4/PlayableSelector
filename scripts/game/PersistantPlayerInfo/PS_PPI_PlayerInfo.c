void PS_PPI_PlayerInfo_OnPlayerInfoChanged(notnull PS_PPI_PlayerInfo playerInfochracterComponent);
typedef func PS_PPI_PlayerInfo_OnPlayerInfoChanged;
typedef ScriptInvokerBase<PS_PPI_PlayerInfo_OnPlayerInfoChanged> PS_PPI_PlayerInfo_OnPlayerInfoChangedInvoker;

class PS_PPI_PlayerInfo {
    protected int playerId;

    protected string playerIdentityId;

    protected string playerName;

    protected PlatformKind playerPlatformKind;

    protected ref PS_PPI_PlayerInfo_OnPlayerInfoChangedInvoker onPlayerInfoChangedInvoker;

    
    
    protected void PS_PPI_PlayerInfo() {
        playerId = -1;
        playerIdentityId = "";
        playerName = "";
        playerPlatformKind = PlatformKind.NONE;
        onPlayerInfoChangedInvoker = new PS_PPI_PlayerInfo_OnPlayerInfoChangedInvoker();
    };

    
    
    int GetPlayerId() {
        return playerId;
    };

    void SetPlayerId(int playerId) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerId(%2)", this, playerId, level: LogLevel.NORMAL);
        this.playerId = playerId;
    };

    string GetPlayerIdentityId() {
        return playerIdentityId;
    };

    string GetPlayerName() {
        return playerName;
    };

    void SetPlayerName(string playerName) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerName(\"%2\")", this, playerName, level: LogLevel.NORMAL);
        this.playerName = playerName;
    };

    PlatformKind GetPlayerPlatformKind() {
        return playerPlatformKind;
    };

    void SetPlayerPlatformKind(PlatformKind playerPlatformKind) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerPlatformKind(%2.%3)", this, "PlatformKind", SCR_Enum.GetEnumName(PlatformKind, playerPlatformKind), level: LogLevel.NORMAL);
        this.playerPlatformKind = playerPlatformKind;
    };

    
    
    void OnPlayerInfoChanged() {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoChanged()", this, level: LogLevel.NORMAL);
        onPlayerInfoChangedInvoker.Invoke(this);
    };

    PS_PPI_PlayerInfo_OnPlayerInfoChangedInvoker GetOnPlayerInfoChanged() {
        return onPlayerInfoChangedInvoker;
    };



    bool IsPlayerConnected() {
        return playerId != -1;
    };



    string ToStringExt() {
        return string.Format(
            "%1 {id = %2, identityId = \"%3\", name = \"%4\", platformKind = %5.%6}",
            this,
            playerId,
            playerIdentityId,
            playerName,
            "PlatformKind",
            SCR_Enum.GetEnumName(PlatformKind, playerPlatformKind)
        );
    };



    event bool RplSave(ScriptBitWriter writer) {
        writer.WriteInt(playerId);
        
        writer.WriteString(playerIdentityId);
        
        writer.WriteString(playerName);
        
        int minPlatformKind, maxPlatformKind;
        SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        writer.WriteIntRange(playerPlatformKind, minPlatformKind, maxPlatformKind);

        return true;
    };

    static bool New(ScriptBitReader reader, out PS_PPI_PlayerInfo playerInfo) {
        int playerId;
        if (!reader.ReadInt(playerId))
            return false;
        
        string playerIdentityId;
        if (!reader.ReadString(playerIdentityId))
            return false;
        
        string playerName;
        if (!reader.ReadString(playerName))
            return false;

        int playerPlatformKind;
        int minPlatformKind, maxPlatformKind;
        SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        if (!reader.ReadIntRange(playerPlatformKind, minPlatformKind, maxPlatformKind))
            return false;
        
        playerInfo = PS_PPI_PlayerInfo.New(playerId, playerIdentityId, playerName, playerPlatformKind);
        
        return true;
    };
	
	static PS_PPI_PlayerInfo New(int playerId, string playerIdentityId, string playerName, PlatformKind playerPlatformKind) {
		auto instance = new PS_PPI_PlayerInfo();
		instance.playerId = playerId;
		instance.playerIdentityId = playerIdentityId;
		instance.playerName = playerName;
		instance.playerPlatformKind = playerPlatformKind;
		return instance;
	};



    static bool Extract(PS_PPI_PlayerInfo instance, ScriptCtx ctx, SSnapSerializerBase snapshot) {
        snapshot.SerializeInt(instance.playerId);
        snapshot.SerializeString(instance.playerIdentityId);
        snapshot.SerializeString(instance.playerName);
        snapshot.SerializeInt(instance.playerPlatformKind);
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        // snapshot.SerializeIntRange(instance.playerPlatformKind, minPlatformKind, maxPlatformKind);
        return true;
    };

    static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PPI_PlayerInfo instance) {
        snapshot.SerializeInt(instance.playerId);
        snapshot.SerializeString(instance.playerIdentityId);
        snapshot.SerializeString(instance.playerName);
        snapshot.SerializeInt(instance.playerPlatformKind);
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        // snapshot.SerializeIntRange(instance.playerPlatformKind, minPlatformKind, maxPlatformKind);
        return true;
    };

    static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet) {
        snapshot.EncodeInt(packet);
        snapshot.EncodeString(packet);
        snapshot.EncodeString(packet);
        snapshot.EncodeInt(packet);
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        // snapshot.EncodeIntRange(packet, minPlatformKind, maxPlatformKind);
    };

    static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot) {
        snapshot.DecodeInt(packet);
        snapshot.DecodeString(packet);
        snapshot.DecodeString(packet);
        snapshot.DecodeInt(packet);
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        // snapshot.DecodeIntRange(packet, minPlatformKind, maxPlatformKind);
        return true;
    };

    static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx) {
        int lhsPlayerId, rhsPlayerId;
        lhs.SerializeInt(lhsPlayerId);
        rhs.SerializeInt(rhsPlayerId);
        if (lhsPlayerId != rhsPlayerId)
            return false;

        string lhsPlayerIdentityId, rhsPlayerIdentityId;
        lhs.SerializeString(lhsPlayerIdentityId);
        rhs.SerializeString(rhsPlayerIdentityId);
        if (lhsPlayerIdentityId != rhsPlayerIdentityId)
            return false;

        string lhsPlayerName, rhsPlayerName;
        lhs.SerializeString(lhsPlayerName);
        rhs.SerializeString(rhsPlayerName);
        if (lhsPlayerName != rhsPlayerName)
            return false;

        PlatformKind lhsPlayerPlatformKind, rhsPlayerPlatformKind;
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        lhs.SerializeInt(lhsPlayerPlatformKind);
        // lhs.SerializeIntRange(lhsPlayerPlatformKind, minPlatformKind, maxPlatformKind);
        rhs.SerializeInt(rhsPlayerPlatformKind);
        // rhs.SerializeIntRange(rhsPlayerPlatformKind, minPlatformKind, maxPlatformKind);
        if (lhsPlayerPlatformKind != rhsPlayerPlatformKind)
            return false;
        
        return true;
    };

    static bool PropCompare(PS_PPI_PlayerInfo instance, SSnapSerializerBase snapshot, ScriptCtx ctx) {
        // int minPlatformKind, maxPlatformKind;
        // SCR_Enum.GetRange(PlatformKind, minPlatformKind, maxPlatformKind);
        return snapshot.CompareInt(instance.playerId)
            && snapshot.CompareString(instance.playerIdentityId)
            && snapshot.CompareString(instance.playerName)
            && snapshot.CompareInt(instance.playerPlatformKind)
            // && snapshot.CompareIntRange(instance.playerPlatformKind, minPlatformKind, maxPlatformKind)
        ;
    };
};
