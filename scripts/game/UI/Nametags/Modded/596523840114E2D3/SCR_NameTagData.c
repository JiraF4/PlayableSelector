modded class SCR_NameTagData {
    protected PS_PPI_GameModeComponent m_PS_PPI_GameModeComponent;

    protected PS_PPI_CharacterComponent m_PS_PPI_CharacterComponent;

    protected PS_PPI_PlayerInfo m_PS_PPI_PlayerInfo;

    override void GetName(out string name, out notnull array<string> nameParams) {
        if (!m_PS_PPI_GameModeComponent || !m_PS_PPI_CharacterComponent)
            return super.GetName(name, nameParams);

        m_aNameParams.Clear();

        if (m_PS_PPI_PlayerInfo) {
            m_sName = string.Format(
                "%3",
                m_PS_PPI_PlayerInfo.GetPlayerId(),
                m_PS_PPI_PlayerInfo.GetPlayerIdentityId(),
                m_PS_PPI_PlayerInfo.GetPlayerName(),
                SCR_Enum.GetEnumName(PlatformKind, m_PS_PPI_PlayerInfo.GetPlayerPlatformKind())
            );

            if (PS_PPI_IsAlive())
            if (!m_PS_PPI_PlayerInfo.IsPlayerConnected())
                m_sName = string.Format("[Disconnected] %1", m_sName);
            else if (!m_PS_PPI_CharacterComponent.IsPlayerControlled())
                m_sName = string.Format("[AI] %1", m_sName);
        }
        else {
            m_sName = "AI";
        };

        if (!PS_PPI_IsAlive())
            m_sName = string.Format("[Dead] %1", m_sName);

        name = m_sName;
        nameParams.Copy(m_aNameParams);
	};

    protected bool PS_PPI_IsAlive() {
        if (!m_CharController)
            return true;

        return m_CharController.GetLifeState() == ECharacterLifeState.ALIVE;
    };



	override bool InitTag(SCR_NameTagDisplay display, IEntity entity, SCR_NameTagConfig config, bool IsCurrentPlayer = false) {
        if (!super.InitTag(display, entity, config, IsCurrentPlayer))
            return false;
        
        m_PS_PPI_GameModeComponent = PS_PPI_GameModeComponent.GetInstance();
        if (!m_PS_PPI_GameModeComponent)
            return true;

        m_PS_PPI_CharacterComponent = PS_PPI_CharacterComponent.GetInstance(entity);
        if (!m_PS_PPI_CharacterComponent)
            return true;
        
        m_PS_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Insert(PS_PPI_OnPlayerInfoIdChanged);
        m_PS_PPI_CharacterComponent.GetOnPlayerControlledChanged().Insert(PS_PPI_OnPlayerControlledChanged);
        if (m_PS_PPI_CharacterComponent.HasPlayerInfoId()) {
            auto playerInfoId = m_PS_PPI_CharacterComponent.GetPlayerInfoId();
            PS_PPI_OnPlayerInfoIdChanged(m_PS_PPI_CharacterComponent, playerInfoId, -1);
        };
        if (m_PS_PPI_CharacterComponent.IsPlayerControlled())
            PS_PPI_OnPlayerControlledChanged(m_PS_PPI_CharacterComponent, true);

        if (m_CharController) {
            m_CharController.m_OnLifeStateChanged.Insert(PS_PPI_OnLifeStateChanged);
        };
            
        return true;
    };
	
	override void ResetTag() {
		if (m_PS_PPI_PlayerInfo) {
			m_PS_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PS_PPI_OnPlayerInfoChanged);
			m_PS_PPI_PlayerInfo = null;
		};
		
		if (m_PS_PPI_CharacterComponent) {
			m_PS_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Remove(PS_PPI_OnPlayerInfoIdChanged);
            m_PS_PPI_CharacterComponent.GetOnPlayerControlledChanged().Remove(PS_PPI_OnPlayerControlledChanged);
			m_PS_PPI_CharacterComponent = null;
		};
		
        if (m_CharController) {
            m_CharController.m_OnLifeStateChanged.Remove(PS_PPI_OnLifeStateChanged);
		};
		
		super.ResetTag();
	};

    protected event void PS_PPI_OnPlayerInfoIdChanged(notnull PS_PPI_CharacterComponent characterComponent, int playerInfoId, int playerInfoIdOld) {
        if (m_PS_PPI_PlayerInfo) {
            m_PS_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PS_PPI_OnPlayerInfoChanged);
            m_PS_PPI_PlayerInfo = null;
        };

        if (playerInfoId != -1) {
            m_PS_PPI_PlayerInfo = m_PS_PPI_GameModeComponent.GetPlayerInfo(playerInfoId);
            m_PS_PPI_PlayerInfo.GetOnPlayerInfoChanged().Insert(PS_PPI_OnPlayerInfoChanged);
        };
        
        PS_PPI_OnTagChanged();
    };

    protected event void PS_PPI_OnPlayerInfoChanged(notnull PS_PPI_PlayerInfo playerInfo) {
        PS_PPI_OnTagChanged();
    };

    protected event void PS_PPI_OnPlayerControlledChanged(notnull PS_PPI_CharacterComponent characterComponent, bool isPlayerControlled) {
        PS_PPI_OnTagChanged();
    };

    protected event void PS_PPI_OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState) {
        auto isAlive = newLifeState == ECharacterLifeState.ALIVE;
        auto isAliveOld = previousLifeState == ECharacterLifeState.ALIVE;
        if (isAlive != isAliveOld)
            return;
        
        PS_PPI_OnTagChanged();
    };

    protected event void PS_PPI_OnTagChanged() {
        m_Flags |= ENameTagFlags.NAME_UPDATE;
    };
};
