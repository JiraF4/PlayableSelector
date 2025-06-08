void PS_PPI_CharacterComponent_OnPlayerInfoIdChanged(notnull PS_PPI_CharacterComponent characterComponent, int newPlayerInfoId, int oldPlayerInfoId);
typedef func PS_PPI_CharacterComponent_OnPlayerInfoIdChanged;
typedef ScriptInvokerBase<PS_PPI_CharacterComponent_OnPlayerInfoIdChanged> PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker;

void PS_PPI_CharacterComponent_OnPlayerControlledChanged(notnull PS_PPI_CharacterComponent characterComponent, bool isPlayerControlled);
typedef func PS_PPI_CharacterComponent_OnPlayerControlledChanged;
typedef ScriptInvokerBase<PS_PPI_CharacterComponent_OnPlayerControlledChanged> PS_PPI_CharacterComponent_OnPlayerControlledChangedInvoker;

class PS_PPI_CharacterComponentClass : ScriptComponentClass {};

class PS_PPI_CharacterComponent : ScriptComponent {
    protected int playerInfoId;

    protected bool isPlayerControlled;

    protected ref PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker onPlayerInfoIdChanged;

    protected ref PS_PPI_CharacterComponent_OnPlayerControlledChangedInvoker onPlayerControlledChangedInvoker;
	


	private void PS_PPI_CharacterComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		playerInfoId = -1;
		onPlayerInfoIdChanged = new PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker();
        onPlayerControlledChangedInvoker = new PS_PPI_CharacterComponent_OnPlayerControlledChangedInvoker();
	};



    int GetPlayerInfoId() {
        return playerInfoId;
    };

    bool HasPlayerInfoId() {
        return playerInfoId != -1;
    };

    void SetPlayerInfoId(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerInfoId(%2)", this, playerInfoId, level: LogLevel.NORMAL);
        Rpc(RpcDo_SetPlayerInfoId, playerInfoId);
        SetPlayerInfoIdLocal(playerInfoId);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_SetPlayerInfoId(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_SetPlayerInfoId(%2)", this, playerInfoId, level: LogLevel.NORMAL);
        SetPlayerInfoIdLocal(playerInfoId);
    };

    protected void SetPlayerInfoIdLocal(int playerInfoId) {
        auto oldPlayerInfoId = this.playerInfoId;
        if (oldPlayerInfoId == playerInfoId)
            return;

        this.playerInfoId = playerInfoId;

        OnPlayerInfoIdChanged(playerInfoId, oldPlayerInfoId);
    };

    protected event void OnPlayerInfoIdChanged(int newPlayerInfoId, int oldPlayerInfoId) {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoIdChanged(%2, %3)", this, newPlayerInfoId, oldPlayerInfoId, level: LogLevel.NORMAL);
        onPlayerInfoIdChanged.Invoke(this, newPlayerInfoId, oldPlayerInfoId);
    };

    bool IsPlayerControlled() {
        return isPlayerControlled;
    };

    void SetPlayerControlled(bool isPlayerControlled) {
        PrintFormat("[PS][PPI] <%1>.SetPlayerControlled(%2)", this, isPlayerControlled.ToString(), level: LogLevel.NORMAL);
        Rpc(RpcDo_SetPlayerControlled, isPlayerControlled);
        SetPlayerControlledLocal(isPlayerControlled);
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_SetPlayerControlled(bool isPlayerControlled) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_SetPlayerControlled(%2)", this, isPlayerControlled.ToString(), level: LogLevel.NORMAL);
        SetPlayerControlledLocal(isPlayerControlled);
    };

    protected void SetPlayerControlledLocal(bool isPlayerControlled) {
        if (this.isPlayerControlled == isPlayerControlled)
            return;
        
        this.isPlayerControlled = isPlayerControlled;
        OnPlayerControlledChanged(isPlayerControlled);
    };

    protected event void OnPlayerControlledChanged(bool isPlayerControlled) {
        PrintFormat("[PS][PPI] <%1>.OnPlayerControlledChanged(%2)", this, isPlayerControlled.ToString(), level: LogLevel.NORMAL);
        onPlayerControlledChangedInvoker.Invoke(this, isPlayerControlled);
    };

    PS_PPI_CharacterComponent_OnPlayerControlledChangedInvoker GetOnPlayerControlledChanged() {
        return onPlayerControlledChangedInvoker;
    };



	override event protected bool RplSave(ScriptBitWriter writer) {
        PrintFormat("[PS][PPI] <%1>.RplSave()", this, level: LogLevel.NORMAL);

		writer.WriteInt(playerInfoId);
        writer.WriteBool(isPlayerControlled);

        return true;
	};

    override event protected bool RplLoad(ScriptBitReader reader) {
        PrintFormat("[PS][PPI] <%1>.RplLoad()", this, level: LogLevel.NORMAL);

        if (!reader.ReadInt(playerInfoId))
            return false;
        if (!reader.ReadBool(isPlayerControlled))
            return false;

        if (HasPlayerInfoId())
            OnPlayerInfoIdChanged(playerInfoId, -1);
        if (IsPlayerControlled())
            OnPlayerControlledChanged(isPlayerControlled);
            
        return true;
    };



    PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker GetOnPlayerInfoIdChanged() {
        return onPlayerInfoIdChanged;
    };

    static PS_PPI_CharacterComponent GetInstance(notnull IEntity entity) {
        return PS_PPI_CharacterComponent.Cast(entity.FindComponent(PS_PPI_CharacterComponent));
    };

    static PS_PPI_CharacterComponent GetLocalControlledEntityInstance() {
        return GetInstance(GetGame().GetPlayerController().GetControlledEntity());
    };
};
