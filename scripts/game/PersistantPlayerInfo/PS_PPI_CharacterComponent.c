void PS_PPI_CharacterComponent_OnPlayerInfoIdChanged(PS_PPI_CharacterComponent chracterComponent, int newPlayerInfoId, int oldPlayerInfoId);
typedef func PS_PPI_CharacterComponent_OnPlayerInfoIdChanged;
typedef ScriptInvokerBase<PS_PPI_CharacterComponent_OnPlayerInfoIdChanged> PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker;

class PS_PPI_CharacterComponentClass : ScriptComponentClass {};

class PS_PPI_CharacterComponent : ScriptComponent {
    protected int playerInfoId;

    protected ref PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker onPlayerInfoIdChanged;
	


	private void PS_PPI_CharacterComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		playerInfoId = -1;
		onPlayerInfoIdChanged = new PS_PPI_CharacterComponent_OnPlayerInfoIdChangedInvoker();
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

    

	override event protected bool RplSave(ScriptBitWriter writer) {
        PrintFormat("[PS][PPI] <%1>.RplSave()", this, level: LogLevel.NORMAL);

		writer.WriteInt(playerInfoId);

        return true;
	};

    override event protected bool RplLoad(ScriptBitReader reader) {
        PrintFormat("[PS][PPI] <%1>.RplLoad()", this, level: LogLevel.NORMAL);

        if (!reader.ReadInt(playerInfoId))
            return false;
            
        if (HasPlayerInfoId())
            OnPlayerInfoIdChanged(playerInfoId, -1);
            
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
