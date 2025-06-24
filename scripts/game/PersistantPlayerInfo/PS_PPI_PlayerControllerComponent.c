void PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegistered(notnull PS_PPI_PlayerControllerComponent playerControllerComponent);
typedef func PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegistered;
typedef ScriptInvokerBase<PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegistered> PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegisteredInvoker;



class PS_PPI_PlayerControllerComponentClass : ScriptComponentClass {};

class PS_PPI_PlayerControllerComponent : ScriptComponent {
    protected int playerInfoId;

    protected ref PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegisteredInvoker onPlayerInfoIdRegisteredInvoker;



	private void PS_PPI_PlayerControllerComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		playerInfoId = -1;
        onPlayerInfoIdRegisteredInvoker = new PS_PPI_PlayerControllerComponent_OnPlayerInfoIdRegisteredInvoker();
	};

    override event protected void OnPostInit(IEntity owner) {
		super.OnPostInit(owner);
		
        if (!GetGame().InPlayMode())
            return;

        SetEventMask(owner, EntityEvent.ALL);
    };

    override event protected void EOnInit(IEntity owner) {
		super.EOnInit(owner);
		
        if (!SCR_PlayerController.Cast(owner)) {
            PrintFormat("[PS][PPI] <%1>: Owner <%2> is not a `SCR_PlayerController`!", this, owner, level: LogLevel.ERROR);
            return;
        };
    };

    override event void EOnActivate(IEntity owner) {
		super.EOnActivate(owner);
		
        if (SCR_EntityHelper.IsProxy(owner))
            return;

        auto playerController = SCR_PlayerController.Cast(owner);

        auto onControlledEntityChanged = playerController.m_OnControlledEntityChanged;
        onControlledEntityChanged.Insert(OnControlledEntityChanged);

        auto controlledEntity = playerController.GetControlledEntity();
        if (controlledEntity)
            OnControlledEntityChanged(null, controlledEntity);
    };

    override event void EOnDeactivate(IEntity owner) {
        if (SCR_EntityHelper.IsProxy(owner))
            return;
        
        auto playerController = SCR_PlayerController.Cast(owner);
        auto onControlledEntityChanged = playerController.m_OnControlledEntityChanged;
        onControlledEntityChanged.Remove(OnControlledEntityChanged);
    };



    void RegisterPlayerInfoId(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.RegisterPlayerInfoId(%2)", this, playerInfoId, level: LogLevel.NORMAL);

        Rpc(RpcDo_RegisterPlayerInfoId, playerInfoId);
        RegisterPlayerInfoIdLocal(playerInfoId);

        auto owner = GetOwner();
        auto playerController = SCR_PlayerController.Cast(owner);
        auto controlledEntity = playerController.GetControlledEntity();
        if (controlledEntity)
            SetEntityPlayerInfoId(controlledEntity);
    };

    bool IsPlayerInfoIdRegistered() {
        return playerInfoId != -1;
    };

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_RegisterPlayerInfoId(int playerInfoId) {
        PrintFormat("[PS][PPI] <%1>.RpcDo_RegisterPlayerInfoId(%2)", this, playerInfoId, level: LogLevel.NORMAL);

        RegisterPlayerInfoIdLocal(playerInfoId);
    };

    protected void RegisterPlayerInfoIdLocal(int playerInfoId) {
        this.playerInfoId = playerInfoId;

        OnPlayerInfoIdRegistered();
    };

    protected event void OnPlayerInfoIdRegistered() {
        PrintFormat("[PS][PPI] <%1>.OnPlayerInfoIdRegistered()", this, level: LogLevel.NORMAL);

        onPlayerInfoIdRegisteredInvoker.Invoke(this);
    };



    protected void OnControlledEntityChanged(IEntity from, IEntity to) {
        PrintFormat("[PS][PPI] <%1>.OnControlledEntityChanged(<%2>, <%3>)", this, from, to, level: LogLevel.NORMAL);

        if (from) {
            auto characterComponent = PS_PPI_CharacterComponent.GetInstance(from);
            if (characterComponent)
                characterComponent.SetPlayerControlled(false);
        };

        if (!to)
            return;

        auto characterComponent = PS_PPI_CharacterComponent.GetInstance(to);
        if (characterComponent)
            characterComponent.SetPlayerControlled(true);

        if (!IsPlayerInfoIdRegistered()) {
            PrintFormat("[PS][PPI] <%1>: is not yet registered (no `playerInfoId` is set)! Setting entity player info id is delayed until registration!", this, level: LogLevel.WARNING);
            return;
        };

        SetEntityPlayerInfoId(to);
    };

    protected void SetEntityPlayerInfoId(IEntity entity) {
        auto characterComponent = PS_PPI_CharacterComponent.GetInstance(entity);
        if (!characterComponent) {
            PrintFormat("[PS][PPI] <%1>: <%2> does not have a `PS_PPI_CharacterComponent`!", this, entity, level: LogLevel.ERROR);
            return;
        };
        
        characterComponent.SetPlayerInfoId(playerInfoId);
    };

    

	override event protected bool RplSave(ScriptBitWriter writer) {
        PrintFormat("[PS][PPI] <%1>.RplSave()", this, playerInfoId, level: LogLevel.NORMAL);

		writer.WriteInt(playerInfoId);

        return true;
	};

    override event protected bool RplLoad(ScriptBitReader reader) {
        PrintFormat("[PS][PPI] <%1>.RplLoad()", this, playerInfoId, level: LogLevel.NORMAL);

        if (!reader.ReadInt(playerInfoId))
            return false;
            
        if (IsPlayerInfoIdRegistered())
            OnPlayerInfoIdRegistered();

        return true;
    };



    static PS_PPI_PlayerControllerComponent GetInstance(notnull PlayerController playerController) {
        return PS_PPI_PlayerControllerComponent.Cast(playerController.FindComponent(PS_PPI_PlayerControllerComponent));
    };

    static PS_PPI_PlayerControllerComponent GetLocalInstance() {
        return GetInstance(GetGame().GetPlayerController());
    };

    static PS_PPI_PlayerControllerComponent GetPlayerInstance(int playerId) {
        return GetInstance(GetGame().GetPlayerManager().GetPlayerController(playerId));
    };
};
