void PS_PPI_GameModeComponent_OnPlayerInfoRegistered(PS_PPI_GameModeComponent gameModeComponent, int registeredPlayerInfoId, notnull PS_PPI_PlayerInfo registeredPlayerInfo);
typedef func PS_PPI_GameModeComponent_OnPlayerInfoRegistered;
typedef ScriptInvokerBase<PS_PPI_GameModeComponent_OnPlayerInfoRegistered> PS_PPI_GameModeComponent_OnPlayerInfoRegisteredInvoker;



class PS_PPI_GameModeComponentClass : SCR_BaseGameModeComponentClass {};

class PS_PPI_GameModeComponent : SCR_BaseGameModeComponent {
    protected ref array<ref PS_PPI_PlayerInfo> playerInfos;

    protected ref array<int> registeringPlayerIds;

    protected ref PS_PPI_GameModeComponent_OnPlayerInfoRegisteredInvoker onPlayerInfoRegisteredInvoker;

	private void PS_PPI_GameModeComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		playerInfos = new array<ref PS_PPI_PlayerInfo>();
        registeringPlayerIds = new array<int>();
        onPlayerInfoRegisteredInvoker = new PS_PPI_GameModeComponent_OnPlayerInfoRegisteredInvoker();
	};

    override void OnPlayerConnected(int playerId) {
        auto owner = GetOwner();
        if (SCR_EntityHelper.IsProxy(owner))
            return;
        
        registeringPlayerIds.Insert(playerId);

        SetEventMask(owner, EntityEvent.FRAME);
    };

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout) {
        auto owner = GetOwner();
        if (SCR_EntityHelper.IsProxy(owner))
            return;
        
        auto playerInfoId = FindPlayerInfoByPlayerId(playerId);
        if (playerInfoId != -1)
            OnPlayerInfoDisconnect(playerInfoId);
        else
            registeringPlayerIds.RemoveItem(playerId);
    };
	
	override event protected void EOnFrame(IEntity owner, float timeSlice) {
        auto registeredPlayerIds = new array<int>();
        foreach (auto playerId : registeringPlayerIds)
            if (TryRegisterPlayer(playerId))
                registeredPlayerIds.Insert(playerId);

        foreach (auto playerId : registeredPlayerIds)
            registeringPlayerIds.RemoveItem(playerId);

        if (registeringPlayerIds.IsEmpty())
            ClearEventMask(owner, EntityEvent.FRAME);
    };



    protected bool TryRegisterPlayer(int playerId) {
        auto playerIdentityId = GetPlayerIdentityId(playerId);
        if (playerIdentityId.IsEmpty())
            return false;

        RegisterPlayer(playerId);

        return true;
    };

    protected void RegisterPlayer(int playerId) {
        auto playerInfoId = FindPlayerInfoByPlayerId(playerId);
        if (playerInfoId == -1) {
            auto playerIdentityId = GetPlayerIdentityId(playerId);
            auto playerName = GetPlayerName(playerId);
            auto playerPlatformKind = GetPlayerPlatformKind(playerId);

            auto playerInfo = PS_PPI_PlayerInfo.New(playerId, playerIdentityId, playerName, playerPlatformKind);
            RegisterPlayerInfo(playerInfo);
        }
        else {
            auto playerInfo = playerInfos.Get(playerInfoId);
            OnPlayerInfoConnect(playerInfoId, playerId);
            
            auto playerName = GetPlayerName(playerId);
            if (playerInfo.GetPlayerName() != playerName)
                SetPlayerInfoName(playerInfoId, playerName);
            
            auto playerPlatformKind = GetPlayerPlatformKind(playerId);
            if (playerInfo.GetPlayerPlatformKind() != playerPlatformKind)
                SetPlayerInfoPlatformKind(playerInfoId, playerPlatformKind);
        };
    };

    protected int RegisterPlayerInfo(notnull PS_PPI_PlayerInfo playerInfo) {
        PrintFormat("[PS][PPI] <%1>.RegisterPlayerInfo(<%2>)", this, playerInfo.ToStringExt(), level: LogLevel.NORMAL);
        Rpc(RpcDo_RegisterPlayerInfo, playerInfo);
        return RegisterPlayerInfoLocal(playerInfo);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_RegisterPlayerInfo(notnull PS_PPI_PlayerInfo playerInfo) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_RegisterPlayerInfo(<%2>)", this, playerInfo.ToStringExt(), level: LogLevel.NORMAL);
        RegisterPlayerInfoLocal(playerInfo);
    };

    protected int RegisterPlayerInfoLocal(notnull PS_PPI_PlayerInfo playerInfo) {
        auto playerInfoId = playerInfos.Insert(playerInfo);
        OnPlayerInfoRegistered(playerInfoId, playerInfo);

        auto owner = GetOwner();
        if (!SCR_EntityHelper.IsProxy(owner)) {
            auto game = GetGame();
            auto playerManager = game.GetPlayerManager();
            auto playerId = playerInfo.GetPlayerId();
            auto playerController = playerManager.GetPlayerController(playerId);
            auto playerControllerComponent = PS_PPI_PlayerControllerComponent.GetInstance(playerController);
            playerControllerComponent.RegisterPlayerInfoId(playerInfoId);
        };

        return playerInfoId;
    };

    protected event void OnPlayerInfoRegistered(int playerInfoId, notnull PS_PPI_PlayerInfo playerInfo) {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoRegistered(%2, <%3>)", this, playerInfoId, playerInfo.ToStringExt(), level: LogLevel.NORMAL);
        onPlayerInfoRegisteredInvoker.Invoke(this, playerInfoId, playerInfo);
    };



    protected string GetPlayerIdentityId(int playerId) {
        #ifndef WORKBENCH
        auto game = GetGame();
        auto backendApi = game.GetBackendApi();
        auto playerName = backendApi.GetPlayerIdentityId(playerId);
        #else
        auto playerName = string.Format("#%1", playerId);
        #endif
        return playerName;
    };

    protected string GetPlayerName(int playerId) {
        auto game = GetGame();
        auto playerManager = game.GetPlayerManager();
        auto playerName = playerManager.GetPlayerName(playerId);
        return playerName;
    };

    protected PlatformKind GetPlayerPlatformKind(int playerId) {
        auto game = GetGame();
        auto playerManager = game.GetPlayerManager();
        auto playerPlatformKind = playerManager.GetPlatformKind(playerId);
        return playerPlatformKind;
    };



    int CountPlayerInfos() {
        return playerInfos.Count();
    };

    PS_PPI_PlayerInfo GetPlayerInfo(int playerInfoId) {
        return playerInfos.Get(playerInfoId);
    };

    int FindPlayerInfoByPlayerIdentityId(string playerIdentityId) {
        foreach (auto playerInfoId, auto playerInfo : playerInfos)
            if (playerInfo.GetPlayerIdentityId() == playerIdentityId)
                return playerInfoId;
        return -1;
    };

    bool FindPlayerInfoByPlayerIdentityId(string playerIdentityId, out PS_PPI_PlayerInfo outPlayerInfo) {
        foreach (auto playerInfo : playerInfos)
            if (playerInfo.GetPlayerIdentityId() == playerIdentityId) {
                outPlayerInfo = playerInfo;
                return true;
            };
        return false;
    };

    int FindPlayerInfoByPlayerId(int playerId) {
        foreach (auto playerInfoId, auto playerInfo : playerInfos)
            if (playerInfo.GetPlayerId() == playerId)
                return playerInfoId;
        return -1;
    };

    bool FindPlayerInfoByPlayerId(int playerId, out PS_PPI_PlayerInfo outPlayerInfo) {
        foreach (auto playerInfo : playerInfos)
            if (playerInfo.GetPlayerId() == playerId) {
                outPlayerInfo = playerInfo;
                return true;
            };
        return false;
    };



    protected void SetPlayerInfoName(int playerInfoId, string playerName) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerInfoName(%2, \"%3\")", this, playerInfoId, playerName, level: LogLevel.NORMAL);
        Rpc(RpcDo_SetPlayerInfoName, playerInfoId, playerName);
        SetPlayerInfoNameLocal(playerInfoId, playerName);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_SetPlayerInfoName(int playerInfoId, string playerName) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_SetPlayerInfoName(%2, \"%3\")", this, playerInfoId, playerName, level: LogLevel.NORMAL);
        SetPlayerInfoNameLocal(playerInfoId, playerName);
    };

    protected void SetPlayerInfoNameLocal(int playerInfoId, string playerName) {
        auto playerInfo = playerInfos.Get(playerInfoId);
        playerInfo.SetPlayerName(playerName);
        playerInfo.OnPlayerInfoChanged();
    };

    protected void SetPlayerInfoPlatformKind(int playerInfoId, PlatformKind platformKind) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerInfoPlatformKind(%2, %3)", this, playerInfoId, string.Format("PlatformKind.%1", SCR_Enum.GetEnumName(PlatformKind, platformKind)), level: LogLevel.NORMAL);
        Rpc(RpcDo_SetPlayerInfoPlatformKind, playerInfoId, platformKind);
        SetPlayerInfoPlatformKindLocal(playerInfoId, platformKind);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_SetPlayerInfoPlatformKind(int playerInfoId, PlatformKind platformKind) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_SetPlayerInfoPlatformKind(%2, %3)", this, playerInfoId, string.Format("PlatformKind.%1", SCR_Enum.GetEnumName(PlatformKind, platformKind)), level: LogLevel.NORMAL);
        SetPlayerInfoPlatformKindLocal(playerInfoId, platformKind);
    };

    protected void SetPlayerInfoPlatformKindLocal(int playerInfoId, PlatformKind platformKind) {
        auto playerInfo = playerInfos.Get(playerInfoId);
        playerInfo.SetPlayerPlatformKind(platformKind);
        playerInfo.OnPlayerInfoChanged();
    };

    protected void OnPlayerInfoConnect(int playerInfoId, int playerId) {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoConnect(%2, %3)", this, playerInfoId, playerId, level: LogLevel.NORMAL);
        Rpc(RpcDo_OnPlayerInfoConnect, playerInfoId, playerId);
        OnPlayerInfoConnectLocal(playerInfoId, playerId);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_OnPlayerInfoConnect(int playerInfoId, int playerId) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_OnPlayerInfoConnect(%2, %3)", this, playerInfoId, playerId, level: LogLevel.NORMAL);
        OnPlayerInfoConnectLocal(playerInfoId, playerId);
    };

    protected void OnPlayerInfoConnectLocal(int playerInfoId, int playerId) {
        auto playerInfo = playerInfos.Get(playerInfoId);
        playerInfo.SetPlayerId(playerId);
        playerInfo.OnPlayerInfoChanged();
    };

    protected void OnPlayerInfoDisconnect(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoDisconnect(%2)", this, playerInfoId, level: LogLevel.NORMAL);
        Rpc(RpcDo_OnPlayerInfoDisconnect, playerInfoId);
        OnPlayerInfoDisconnectLocal(playerInfoId);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_OnPlayerInfoDisconnect(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_PlayerInfoDisconnect(%2)", this, playerInfoId, level: LogLevel.NORMAL);
        OnPlayerInfoDisconnectLocal(playerInfoId);
    };

    protected void OnPlayerInfoDisconnectLocal(int playerInfoId) {
        auto playerInfo = playerInfos.Get(playerInfoId);
        playerInfo.SetPlayerId(-1);
        playerInfo.OnPlayerInfoChanged();
    };



	override event protected bool RplSave(ScriptBitWriter writer) {
        PrintFormat("[PS][PPI] <%1>.RplSave()", this, level: LogLevel.NORMAL);

        writer.WriteInt(playerInfos.Count());

        foreach (auto playerInfo : playerInfos)
            if (!playerInfo.RplSave(writer))
                return false;
        
        return true;
	};

    override event protected bool RplLoad(ScriptBitReader reader) {
        PrintFormat("[PS][PPI] <%1>.RplLoad()", this, level: LogLevel.NORMAL);

        int count;
        if (!reader.ReadInt(count))
            return false;
        
        for (int i = 0; i < count; ++i) {
            PS_PPI_PlayerInfo playerInfo;
            if (!PS_PPI_PlayerInfo.New(reader, playerInfo))
                return false;
            auto playerInfoId = playerInfos.Insert(playerInfo);
            OnPlayerInfoRegistered(playerInfoId, playerInfo);
        };

        return true;
    };


    
    static PS_PPI_GameModeComponent GetInstance(notnull BaseGameMode gameMode) {
        auto component = gameMode.FindComponent(PS_PPI_GameModeComponent);
        return PS_PPI_GameModeComponent.Cast(component);
    };
    
    static PS_PPI_GameModeComponent GetInstance() {
        auto game = GetGame();
        auto gameMode = game.GetGameMode();
        return GetInstance(gameMode);
    };
};