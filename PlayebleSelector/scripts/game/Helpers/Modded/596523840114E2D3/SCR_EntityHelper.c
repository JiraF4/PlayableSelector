modded class SCR_EntityHelper {
    static bool IsAuthority(notnull IEntity entity) {
        return !IsProxy(entity);
    };

    static bool IsProxy(notnull IEntity entity) {
        auto rplComponent = GetEntityRplComponent(entity);
        if (!rplComponent)
            return false;

        return rplComponent.IsProxy();
    };

    static bool IsOwner(notnull IEntity entity) {
        auto rplComponent = GetEntityRplComponent(entity);
        if (!rplComponent)
            return true;
        
        return rplComponent.IsOwner();
    };

    static bool IsOwnerProxy(notnull IEntity entity) {
        auto rplComponent = GetEntityRplComponent(entity);
        if (!rplComponent)
            return true;
        
        return rplComponent.IsOwnerProxy();
    };

    static bool IsRemoteProxy(notnull IEntity entity) {
        auto rplComponent = GetEntityRplComponent(entity);
        if (!rplComponent)
            return true;
        
        return rplComponent.IsRemoteProxy();
    };

    static bool IsMaster(notnull IEntity entity) {
        auto rplComponent = GetEntityRplComponent(entity);
        if (!rplComponent)
            return true;
        
        return rplComponent.IsMaster();
    };
};