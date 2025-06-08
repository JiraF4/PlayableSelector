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
	
	protected bool PS_PPI_IsDeadEntitiesCleanupEnabled() {
		return m_fDeadEntitiesCleanup >= 0.0;
	};



	override bool InitTag(SCR_NameTagDisplay display, IEntity entity, SCR_NameTagConfig config, bool IsCurrentPlayer = false) {
        if (!super.InitTag(display, entity, config, IsCurrentPlayer)) {
            // !!! OVERRIDE BEHAVIOUR START !!!
            // !!! Watch for updates in super !!!
            // Allow init tag on dead characters
            auto character = ChimeraCharacter.Cast(entity);
            
            if (!character || !m_CharController || !m_CharController.IsDead() || (m_NTDisplay && config.m_aVisibilityRuleset.m_fDeadEntitiesCleanup >= 0.0))
                return false;
            
            m_CharController.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
			
            if (m_bIsCurrentPlayer) {
                auto vonComp = SCR_VoNComponent.Cast(character.FindComponent(SCR_VoNComponent));
                if (vonComp)
                    vonComp.m_OnReceivedVON.Insert(OnReceivedVON);
            };
			
            InitDefaults();			
            InitData(config);
				
            if (!UpdateEntityStateFlags())
                return false;
            // !!! OVERRIDE BEHAVIOUR END !!!
        };
        
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

    override void ActivateEntityState(ENameTagEntityState state) {
        auto isDead = state == ENameTagEntityState.DEAD;
        auto isDeadBefore = (m_eEntityStateFlags & ENameTagEntityState.DEAD) != 0;
        auto isCleanBefore = (m_Flags & ENameTagFlags.CLEANUP) != 0;

        if (m_fDeadEntitiesCleanup >= 0.0 || !isDead || isDeadBefore || isCleanBefore) {
            super.ActivateEntityState(state);
            return;
        };

        // !!! OVERRIDE BEHAVIOUR START !!!
        // !!! Watch for updates in super !!!
        auto attacedhToOld = m_eAttachedTo;

        super.ActivateEntityState(state);

        if (isDeadBefore || isCleanBefore)
            return;
        
        m_Flags &= ~ENameTagFlags.CLEANUP;
        SetTagPosition(attacedhToOld);

        auto queue = GetGame().GetCallqueue();
        if (!queue)
            return;
        
        queue.Remove(Cleanup);
        // !!! OVERRIDE BEHAVIOUR END !!!
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
        PS_PPI_OnTagChanged();
	};

    protected event void PS_PPI_OnTagChanged() {
        m_Flags |= ENameTagFlags.NAME_UPDATE;
    };
};
