void PS_ScriptInvokerFactionChangeMethod(int playerId, FactionKey factionKey, FactionKey factionKeyOld);
typedef func PS_ScriptInvokerFactionChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionChangeMethod> PS_ScriptInvokerFactionChange;

void PS_ScriptInvokerPlayableMethod(RplId id, PS_PlayableContainer playableComponent);
typedef func PS_ScriptInvokerPlayableMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableMethod> PS_ScriptInvokerPlayable;

void PS_ScriptInvokerPinChangeMethod(int playerId, bool pin);
typedef func PS_ScriptInvokerPinChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPinChangeMethod> PS_ScriptInvokerPinChange;

void PS_ScriptInvokerPlayerStateChangeMethod(int playerId, PS_EPlayableControllerState state);
typedef func PS_ScriptInvokerPlayerStateChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerStateChangeMethod> PS_ScriptInvokerPlayerStateChange;

void PS_ScriptInvokerPlayerPlayableChangeMethod(int playerId, RplId playbleId);
typedef func PS_ScriptInvokerPlayerPlayableChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayerPlayableChangeMethod> PS_ScriptInvokerPlayerPlayableChange;

void PS_ScriptInvokerPlayableChangeGroupMethod(RplId id, PS_PlayableContainer playableComponent, SCR_AIGroup aiGroup);
typedef func PS_ScriptInvokerPlayableChangeGroupMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerPlayableChangeGroupMethod> PS_ScriptInvokerPlayableChangeGroup;

void PS_ScriptInvokerFactionReadyChangeMethod(FactionKey factionKey, int readyValue);
typedef func PS_ScriptInvokerFactionReadyChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerFactionReadyChangeMethod> PS_ScriptInvokerFactionReadyChangeGroup;

[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableManagerClass : ScriptComponentClass
{

}

class PS_PlayableManager : ScriptComponent
{
	// Map of our playables
	ref map<RplId, ref PS_PlayableContainer> m_aPlayables = new map<RplId, ref PS_PlayableContainer>(); // We NOW sync it!
	ref array<PS_PlayableContainer> m_aPlayablesSorted = {};
	ref map<RplId, ref PS_PlayableVehicleContainer> m_mPlayableVehicles = new map<RplId, ref PS_PlayableVehicleContainer>();

	// Maps for saving players staff, player controllers local to client
	ref map<int, PS_EPlayableControllerState> m_playersStates = new map<int, PS_EPlayableControllerState>();
	ref map<int, RplId> m_playersPlayableRemembered = new map<int, RplId>();
	ref map<int, RplId> m_playersPlayable = new map<int, RplId>();
	ref map<RplId, int> m_playablePlayersRemembered = new map<RplId, int>();
	ref map<RplId, int> m_playablePlayers = new map<RplId, int>(); // reversed m_playersPlayable for fast search
	ref map<int, bool> m_playersPin = new map<int, bool>(); // is player pined
	ref map<int, FactionKey> m_playersFaction = new map<int, FactionKey>(); // player factions
	ref map<int, FactionKey> m_playersFactionRemembered = new map<int, FactionKey>(); // player factions persistant
	ref map<RplId, int> m_playablePlayerGroupId = new map<RplId, int>(); // playable to player group
	ref map<int, string> m_playersLastName = new map<int, string>(); // playerid to player name (persistant)
	ref map<FactionKey, int> m_mFactionReady = new map<FactionKey, int>(); // faction ready state
	ref map<RplId, string> m_mPlayablePrefabs = new map<RplId, string>();
	// Role display info keyed by PREFAB (deduplicated - many playables share a role prefab), instead of
	// shipping the same long icon path/name on all 128 containers.
	ref map<string, string> m_mPrefabRoleIcon = new map<string, string>();
	ref map<string, string> m_mPrefabRoleQuad = new map<string, string>();
	ref map<string, string> m_mPrefabRoleName = new map<string, string>();

	// Server-only reconnect cache, keyed by player GUID (a reconnecting player gets a NEW playerId,
	// so all the playerId-keyed state above is lost - this lets us restore faction/slot/pin/name).
	protected ref map<string, RplId> m_mReconnectPlayable = new map<string, RplId>();
	protected ref map<string, FactionKey> m_mReconnectFaction = new map<string, FactionKey>();
	protected ref map<string, bool> m_mReconnectPin = new map<string, bool>();
	protected ref map<string, string> m_mReconnectName = new map<string, string>();
	protected ref map<int, UUID> m_mPlayerIdentityGuid = new map<int, UUID>();
	protected ref map<int, int> m_mDisconnectGeneration = new map<int, int>();
	protected ref map<string, int> m_mReconnectSourcePlayerId = new map<string, int>();
	protected ref map<string, float> m_mReconnectExpiresAt = new map<string, float>();
	protected ref map<string, int> m_mReconnectGeneration = new map<string, int>();
	protected ref map<int, int> m_mPendingReconnectGeneration = new map<int, int>();
	protected int m_iConnectionGeneration;
	// Diagnostic only: connect-time world-time (ms) per RECONNECTING playerId, so the reconnect -> Global ->
	// group/Command VoN transition can be logged with elapsed timing (TraceReconnectConnect + RestorePlayerReconnectData).
	protected ref map<int, float> m_mReconnectTraceTime = new map<int, float>();

	// Invokers
	ref ScriptInvokerInt m_eOnPlayerConnected = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerConnected()
	{
		return m_eOnPlayerConnected;
	}
	ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
	{
		return m_eOnPlayerDisconnected;
	}
	ref PS_ScriptInvokerFactionChange m_eOnFactionChange = new PS_ScriptInvokerFactionChange(); // int playerId, FactionKey factionKey, FactionKey factionKeyOld
	PS_ScriptInvokerFactionChange GetOnFactionChange()
	{
		return m_eOnFactionChange;
	}
	ref PS_ScriptInvokerPlayable m_eOnPlayableRegistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableRegistered()
	{
		return m_eOnPlayableRegistered;
	}
	ref PS_ScriptInvokerPlayable m_eOnPlayableUnregistered = new PS_ScriptInvokerPlayable();
	PS_ScriptInvokerPlayable GetOnPlayableUnregistered()
	{
		return m_eOnPlayableUnregistered;
	}
	ref PS_ScriptInvokerPinChange m_eOnPlayerPinChange = new PS_ScriptInvokerPinChange();
	PS_ScriptInvokerPinChange GetOnPlayerPinChange()
	{
		return m_eOnPlayerPinChange;
	}
	ref PS_ScriptInvokerPlayerStateChange m_eOnPlayerStateChange = new PS_ScriptInvokerPlayerStateChange();
	PS_ScriptInvokerPlayerStateChange GetOnPlayerStateChange()
	{
		return m_eOnPlayerStateChange;
	}
	/**
	 * @brief Изменение назначения игрока, включая вытесненного владельца.
	 * @event После согласованной мутации обеих maps; один вызов на реально изменённого участника.
	 */
	ref PS_ScriptInvokerPlayerPlayableChange m_eOnPlayerPlayableChange = new PS_ScriptInvokerPlayerPlayableChange();
	PS_ScriptInvokerPlayerPlayableChange GetOnPlayerPlayableChange()
	{
		return m_eOnPlayerPlayableChange;
	}
	ref PS_ScriptInvokerPlayableChangeGroup m_eOnPlayableChangeGroup = new PS_ScriptInvokerPlayableChangeGroup();
	PS_ScriptInvokerPlayableChangeGroup GetOnPlayableChangeGroup()
	{
		return m_eOnPlayableChangeGroup;
	}
	ref ScriptInvokerInt m_eOnStartTimerCounterChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnStartTimerCounterChanged()
	{
		return m_eOnStartTimerCounterChanged;
	}
	ref PS_ScriptInvokerFactionReadyChangeGroup m_eFactionReadyChanged = new PS_ScriptInvokerFactionReadyChangeGroup();
	PS_ScriptInvokerFactionReadyChangeGroup GetOnFactionReadyChanged()
	{
		return m_eFactionReadyChanged;
	}
	/**
	 * @brief Изменение/получение текущей вместимости сессии.
	 * @event Локально после изменения scalar на authority или получения property/JIP на клиенте.
	 */
	ref ScriptInvokerInt m_eOnMaxPlayersCountChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnMaxPlayersCountChanged()
	{
		return m_eOnMaxPlayersCountChanged;
	}

	//Global cache
	protected PS_GameModeCoop m_GameModeCoop;
	protected ScriptCallQueue m_CallQueue;
	protected PlayerManager m_PlayerManager;

	protected SCR_PlayerController m_CurrentPlayerController;
	static protected PS_PlayableControllerComponent s_CurrentPlayableController;

	protected static PS_PlayableManager s_Instance;

	[RplProp(onRplName: "OnRpl_MaxPlayersCount")]
	int m_iMaxPlayersCount = 0; // Server limit, or current registered-slot fallback
	protected bool m_bHasSessionPlayerLimit;

	/**
	 * @brief Колбэк репликации лимита игроков сервера.
	 * @sync Server -> All
	 * @issue BUG-87
	 * @cause Клиенты не получали уведомления об обновлении m_iMaxPlayersCount при репликации свойства.
	 * @solution Вызов инвокера m_eOnMaxPlayersCountChanged для реактивного обновления UI.
	 */
	void OnRpl_MaxPlayersCount()
	{
		if (m_eOnMaxPlayersCountChanged)
			m_eOnMaxPlayersCountChanged.Invoke(m_iMaxPlayersCount);
	}

	/**
	 * @brief Флаг завершения фоновой загрузки и регистрации всех слотов миссии.
	 * @sync Server -> All (Broadcast RPC + RplProp + JIP Snapshot)
	 */
	[RplProp()]
	protected bool m_bSlotsFullyLoaded = false;

	/**
	 * @brief Проверка завершения загрузки всех слотов на карте
	 * @context Client | Server | Authority
	 * @return true, если все слоты зарегистрированы и спавн завершён
	 */
	bool IsSlotsFullyLoaded()
	{
		return m_bSlotsFullyLoaded;
	}

	int GetPlayablesCount()
	{
		return m_aPlayables.Count();
	}

	int GetTargetPlayablesCount()
	{
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (missionHeader)
			return missionHeader.m_iPlayerCount;
		return 0;
	}

	/**
	 * @brief Формирует текст системного оповещения о загрузке слотов
	 */
	string GetSlotsLoadingMessage()
	{
		int currentCount = m_aPlayables.Count();
		int targetCount = GetTargetPlayablesCount();
		if (targetCount > 1 && targetCount >= currentCount)
			return string.Format("Слоты ещё загружаются (%1/%2). Подождите завершения загрузки...", currentCount, targetCount);
		else
			return string.Format("Слоты ещё загружаются (%1). Подождите завершения загрузки...", currentCount);
	}

	/**
	 * @brief Выводит простое локальное сообщение в чат администратора
	 * @context Client | UI
	 */
	void ShowSlotsLoadingNotice()
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;

		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		if (!chatPanelManager)
			return;
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("lmsg");
		if (invoker)
			invoker.Invoke(null, GetSlotsLoadingMessage());
	}

	/**
	 * @brief Фиксирует завершение фонового спавна и регистрации всех слотов на карте.
	 * @context Authority | Server
	 * @details Вызывается после затишья в 1.5с при регистрации слотов или по защитному таймауту.
	 */
	void SetSlotsFullyLoaded()
	{
		if (m_bSlotsFullyLoaded)
			return;

		m_bSlotsFullyLoaded = true;
		if (m_CallQueue)
			m_CallQueue.Remove(SetSlotsFullyLoaded);

		if (Replication.IsServer())
		{
			QueueFallbackCapacityUpdate_S();
			Replication.BumpMe();
			Rpc(RPC_SetSlotsFullyLoaded);
		}

		Print(string.Format("[PS_PlayableManager] All slots fully loaded! Total playables registered: %1", m_aPlayables.Count()), LogLevel.NORMAL);
	}

	/**
	 * @brief Сетевое оповещение клиентов о завершении загрузки слотов
	 * @rpc Server -> Broadcast (Reliable)
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetSlotsFullyLoaded()
	{
		m_bSlotsFullyLoaded = true;
	}
	
	// TODO: Remove?
	[RplProp(onRplName: "OnStartTimerCounterChanged")]
	int m_iStartTimerCounter = -1;
	void StartTime()
	{
		m_iStartTimerCounter -= 1;
		Replication.BumpMe();
		OnStartTimerCounterChanged();
		if (m_iStartTimerCounter == 0)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			// Use the CURRENT state, not a hardcoded one — this timer is reused by both
			// slot-selection "all ready" and briefing "faction ready" countdowns.
			gameModeCoop.AdvanceGameState(gameModeCoop.GetState());
			m_CallQueue.Remove(StartTime);
		}
	}
	void OnStartTimerCounterChanged()
	{
		m_eOnStartTimerCounterChanged.Invoke(m_iStartTimerCounter);
	}
	
	bool m_bFactionsReadySended; // Is faction ready message already sended?

	// Is it required?
	bool m_bRplLoaded = false;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}

	// --------------------------------------------------------------------------------------------
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_PlayableManager GetInstance()
	{
		return s_Instance;
	}

	// --------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		s_Instance = this;

		//Cache
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_CallQueue = GetGame().GetCallqueue();
		m_PlayerManager = GetGame().GetPlayerManager();

		if (Replication.IsServer())
		{
			m_bRplLoaded = true;
			// Fallback safety timeout in case map has no playables or streaming completes with 0 slots
			m_CallQueue.CallLater(SetSlotsFullyLoaded, 15000, false);
		}
		if (Replication.IsServer())
			ForceGetSessionMaxPlayersCount();
	
		// Register events
		m_GameModeCoop.GetOnPlayerConnected().Insert(OnPlayerConnected);
		m_GameModeCoop.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		m_GameModeCoop.GetOnPlayerRoleChange().Insert(OnPlayerRoleChange);
		m_CallQueue.Call(LateInit, owner);
	}
	protected void LateInit(IEntity owner)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		m_CurrentPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_CurrentPlayerController)
		{
			m_CallQueue.Call(LateInit, owner);
			return;
		}
		s_CurrentPlayableController = m_CurrentPlayerController.PS_GetPLayableComponent();
	}
	// --------------------------------------------------------------------------------------------
	/**
	 * @brief Запрос лимита игроков сервера из ServerInfo с повторными попытками и fallback на число слотов.
	 * @issue BUG-87
	 * @cause Покадровый опрос (Call) исчерпывал 30 попыток за 300-500 мс до инициализации ServerInfo на выделенном сервере, оставляя лимит равным дефолтному 1.
	 * @solution Ограниченный опрос через 1000 мс; fallback следует событиям registry, confirmed limit сохраняет приоритет.
	 */
	protected int m_iServerInfoRetries;
	protected void ForceGetSessionMaxPlayersCount()
	{
		if (!Replication.IsServer() || m_bIsCleanedUp)
			return;

		ServerInfo serverInfo = GetGame().GetServerInfo();
		int playerLimit = 0;
		if (serverInfo)
			playerLimit = serverInfo.GetPlayerLimit();

		if (playerLimit > 0)
		{
			m_bHasSessionPlayerLimit = true;
			SetMaxPlayersCount_S(playerLimit);
		}
		else
		{
			PublishFallbackCapacity_S();
			if (m_iServerInfoRetries < 15)
			{
				m_iServerInfoRetries = m_iServerInfoRetries + 1;
				m_CallQueue.CallLater(ForceGetSessionMaxPlayersCount, 1000, false);
			}
		}
	}

	/**
	 * @brief Публикация вместимости только при реальном изменении.
	 * @issue BUG-87
	 * @cause Одноразовый fallback устаревал после поздней регистрации слотов.
	 * @solution Event-driven Count и scalar guard исключают застрявший 1 и same-value BumpMe.
	 */
	protected void SetMaxPlayersCount_S(int maxPlayers)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp || m_iMaxPlayersCount == maxPlayers)
			return;
		m_iMaxPlayersCount = maxPlayers;
		Replication.BumpMe();
		OnRpl_MaxPlayersCount();
	}

	protected void PublishFallbackCapacity_S()
	{
		if (Replication.IsServer() && !m_bIsCleanedUp && !m_bHasSessionPlayerLimit)
			SetMaxPlayersCount_S(m_aPlayables.Count());
	}

	/**
	 * @brief Один queued Count для пакета регистраций/удалений без постоянного опроса.
	 * @workaround Call выполняется на следующем Tick; Remove bound-метода coalesces текущий пакет.
	 */
	protected void QueueFallbackCapacityUpdate_S()
	{
		if (Replication.IsServer() && !m_bIsCleanedUp && !m_bHasSessionPlayerLimit)
		{
			m_CallQueue.Remove(PublishFallbackCapacity_S);
			m_CallQueue.Call(PublishFallbackCapacity_S);
		}
	}

	// --------------------------------------------------------------------------------------------
	// Teardown — симметрия к PS_PlayableControllerComponent.OnDelete
	protected bool m_bIsCleanedUp;

	/**
	 * @brief Снятие подписок на инвокеры GameMode, отмена отложенных задач и сброс singleton
	 * @details Вызывается из OnGameEnd() и из OnDelete(). Идемпотентна: повторный вызов
	 *          ничего не делает, поэтому таймеры и подписки снимаются ровно один раз.
	 */
	void Cleanup()
	{
		if (m_bIsCleanedUp)
			return;
		m_bIsCleanedUp = true;

		if (m_GameModeCoop)
		{
			m_GameModeCoop.GetOnPlayerConnected().Remove(OnPlayerConnected);
			m_GameModeCoop.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
			m_GameModeCoop.GetOnPlayerRoleChange().Remove(OnPlayerRoleChange);
		}

		if (m_CallQueue)
		{
			m_CallQueue.Remove(SetSlotsFullyLoaded);
			m_CallQueue.Remove(LateInit);
			m_CallQueue.Remove(ForceGetSessionMaxPlayersCount);
			m_CallQueue.Remove(DelayedSwitchToInitialEntity);
			m_CallQueue.Remove(ChangeGroup);
			m_CallQueue.Remove(UpdateGroupCallsign);
			m_CallQueue.Remove(RegisterGroupName);
			m_CallQueue.Remove(UpdatePlayablesSorted);
			m_CallQueue.Remove(UpdatePlayablesSortedDelayed);
			m_CallQueue.Remove(OnPlayableRegisteredLateInvoke);
			m_CallQueue.Remove(OnPlayableRegisteredLateInvoke2);
			m_CallQueue.Remove(RegisterGroupVehicle);
			m_CallQueue.Remove(StartTime);
			m_CallQueue.Remove(RemoveRedundantUnits);
			m_CallQueue.Remove(SCR_EntityHelper.DeleteEntityAndChildren);
			m_CallQueue.Remove(RestorePlayerReconnectData_S);
			m_CallQueue.Remove(ExpireDisconnectedPlayer_S);
			m_CallQueue.Remove(PublishFallbackCapacity_S);
		}
		m_mPlayerIdentityGuid.Clear();
		m_mDisconnectGeneration.Clear();
		m_mPendingReconnectGeneration.Clear();
		m_mReconnectPlayable.Clear();
		m_mReconnectFaction.Clear();
		m_mReconnectPin.Clear();
		m_mReconnectName.Clear();
		m_mReconnectSourcePlayerId.Clear();
		m_mReconnectExpiresAt.Clear();
		m_mReconnectGeneration.Clear();
		m_mReconnectTraceTime.Clear();
		m_iConnectionGeneration = 0;
		m_bHasSessionPlayerLimit = false;

		if (s_Instance == this)
			s_Instance = null;
	}

	override void OnDelete(IEntity owner)
	{
		Cleanup();
		super.OnDelete(owner);
	}

	// --------------------------------------------------------------------------------------------
	// ----------------------------------- Main entry point ---------------------------------------
	// --------------------------------------------------------------------------------------------
	// Get control on selected playable entity, or initial and become spectator if no playable provided
	// Executed only on server
	void ApplyPlayable(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();

		SetPlayerState(playerId, PS_EPlayableControllerState.Playing);

		// If entity dead, switch to spectator after some delay
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = GetPlayableById(playableId);
			if (playableContainer && playableContainer.GetDamageState() == EDamageState.DESTROYED)
			{
				m_CallQueue.CallLater(DelayedSwitchToInitialEntity, 1000, false, playerId);
			}
		}

		IEntity entity;
		if (playableId == RplId.Invalid()) { // no slot: lobby / spectator
			// Body-less: control NOTHING. Lobby players never had a body; a just-died player keeps
			// their corpse as the controlled entity. The client spectator camera (PS_SpectatorManager)
			// and the VoN proxy (PS_MenuVoN) handle view + voice. The engine has no working
			// SetControlledEntity(null), so we simply do not assign a controlled entity here.
			SCR_AIGroup currentGroup = groupsManagerComponent.GetPlayerGroup(playableId);
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);
			SetPlayerFactionKey(playerId, "");
			return;
		} else {
			PS_PlayableContainer applyContainer = GetPlayableById(playableId);
			PS_PlayableComponent applyPlayable;
			if (applyContainer)
				applyPlayable = applyContainer.GetPlayableComponent();
			if (!applyPlayable || !applyPlayable.GetOwner())
				return; // stale link, playable entity is already gone
			entity = applyPlayable.GetOwner();
		}

		// Delete initial entity if exists
		IEntity initialEntity = playableController.GetInitialEntity();
		if (initialEntity)
			m_CallQueue.Call(SCR_EntityHelper.DeleteEntityAndChildren, initialEntity);

		// Apply entity immediately. At game start / respawn the playables have been force-streamed all
		// through the briefing, so the control switch is not a streaming burst and the player should take
		// control with no delay. The spectator transitions stage their streaming separately via the
		// deferred spectator observer, so no preload handshake is needed on this slot-apply path.
		playerController.SetInitialMainEntity(entity);

		// Set new player faction
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(entity);
		if (playableCharacter)
		{
			SCR_Faction faction = SCR_Faction.Cast(playableCharacter.GetFaction());
			if (faction)
				SetPlayerFactionKey(playerId, faction.GetFactionKey());
		}

		// Requred delay, since entity take one frame to apply controls
		m_CallQueue.CallLater(ChangeGroup, 0, false, playerId, playableId);
	}
	// --------------------------------------------------------------------------------------------
	protected void DelayedSwitchToInitialEntity(int playerId)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameModeCoop.SwitchToInitialEntity(playerId);
	}

	// --------------------------------------------------------------------------------------------
	// Change player group via playable
	void ChangeGroup(int playerId, RplId playableId)
	{
		// Get updated player
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerManager.GetPlayerController(playerId));
		if (!playerController)
			return;
		PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();

		// Get playable container
		SCR_AIGroup playerGroup = GetPlayerGroupByPlayable(playableId);
		SCR_ChimeraCharacter leaderCharacter = null;
		if (playerGroup)
			leaderCharacter = SCR_ChimeraCharacter.Cast(playerGroup.GetLeaderEntity());
		PS_PlayableContainer playableContainerLeader;
		if (leaderCharacter)
			playableContainerLeader = leaderCharacter.PS_GetPlayable().GetPlayableContainer();

		// Join group
		SCR_PlayerControllerGroupComponent playerControllerGroupComponent = SCR_PlayerControllerGroupComponent.Cast(playerController.FindComponent(SCR_PlayerControllerGroupComponent));
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		if (playerGroup)
			playerControllerGroupComponent.PS_AskJoinGroup(playerGroup.GetGroupID());

		// Another workaround
		// Thanks bohem, no one zoomer will be harmed if you remove all text from game.
		// Maybe also multiplayer from arma? It can harm people, very dangerous.
		if (playerGroup && playerGroup.GetNameAuthorID() == -1)
			playerGroup.SetCustomName(playerGroup.GetCustomName(), playerId);


		// Switch leader if need
		if (playableContainerLeader)
			if (playableContainerLeader.GetRplId() > playableId)
				groupsManagerComponent.SetGroupLeader(playerGroup.GetGroupID(), playerId);
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------- Registration -----------------------------------------
	// --------------------------------------------------------------------------------------------
	// Register playable container to global list, replicated across clients
	// Save playable name and prefab for later use
	// - SERVER SIDE: Create new group for players if requred
	void RegisterPlayable(PS_PlayableComponent playableComponent)
	{
		RplId playableId = playableComponent.GetRplId();
		if (m_aPlayables.Contains(playableId)) // Already registered
			return;
		SCR_ChimeraCharacter playableCharacter = playableComponent.GetCharacter();
		if (!playableCharacter)
			return;
		if (!playableCharacter.PS_GetChimeraAIControlComponent())
			return;

		// Save and replicate data
		PS_PlayableContainer container = playableComponent.GetPlayableContainer();
		RPC_RegisterPlayable(container);
		Rpc(RPC_RegisterPlayable, container);
		string prefab = playableComponent.GetOwner().GetPrefabData().GetPrefabName();
		SetPlayablePrefab(playableId, prefab);
		// Role icon/name are static per prefab - stored once per prefab instead of on every container
		SetPrefabRoleInfo(prefab, playableComponent.GetRoleIconPath(), playableComponent.GetRoleIconQuad(), playableComponent.GetRoleName());
		// Name is carried by the container itself, no separate replicated map needed

		// Server side
		if (Replication.IsServer())
		{
			AIControlComponent aiControl = playableCharacter.PS_GetChimeraAIControlComponent();
			SCR_AIGroup playableGroup = SCR_AIGroup.Cast(aiControl.GetControlAIAgent().GetParentGroup());
			SCR_AIGroup playerGroup;

			if (!playableGroup) // Has no group -> broken unit.
				return;

			// If no player, group create new
			if (!playableGroup.m_PlayersGroup)
			{
				SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
				playerGroup = groupsManagerComponent.CreateNewPlayableGroup(playableGroup.GetFaction());

				// Setup link, command system override slave group
				// TODO: somehow move from PSCore
				playerGroup.m_BotsGroup = playableGroup;
				playableGroup.m_PlayersGroup = playerGroup;

				playerGroup.SetMaxMembers(playableGroup.m_aUnitPrefabSlots.Count());
				playerGroup.SetCustomName(playableGroup.GetCustomName(), -1);
				playableGroup.SetCanDeleteIfNoPlayer(false);
				playerGroup.SetCanDeleteIfNoPlayer(false);
				playableGroup.SetDeleteWhenEmpty(false);
				playerGroup.SetDeleteWhenEmpty(false);
			} else {
				playerGroup = playableGroup.m_PlayersGroup;
			}
			SetPlayablePlayerGroupId(playableId, playerGroup.GetGroupID()); // Save link to map for fast search
			m_CallQueue.Call(UpdateGroupCallsign, playableId, playerGroup, playableGroup); // Delay for group init

			// Reset debounce timer on every registered slot (1.5s quiet period after last slot)
			if (!m_bSlotsFullyLoaded)
			{
				m_CallQueue.Remove(SetSlotsFullyLoaded);
				m_CallQueue.CallLater(SetSlotsFullyLoaded, 1500, false);
			}
		}
	}
	protected void UpdateGroupCallsign(RplId playableId, SCR_AIGroup playerGroup, SCR_AIGroup playableGroup)
	{
		if (!playableGroup || !playerGroup)
		{
			m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup);
			return;
		}

		// Assign manualy set callsigns
		PS_GroupCallsignAssigner groupCallsignAssigner = PS_GroupCallsignAssigner.Cast(playableGroup.FindComponent(PS_GroupCallsignAssigner));
		int company, platoon, squad;
		if (groupCallsignAssigner) {
			groupCallsignAssigner.GetCallsign(company, platoon, squad);
		} else {
			SCR_CallsignGroupComponent sourceCallsign = SCR_CallsignGroupComponent.Cast(playableGroup.FindComponent(SCR_CallsignGroupComponent));
			if (sourceCallsign)
				sourceCallsign.GetCallsignIndexes(company, platoon, squad);
		}
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(playerGroup.FindComponent(SCR_CallsignGroupComponent));
		if (callsignComponent)
			callsignComponent.ReAssignGroupCallsign(company, platoon, squad);

		m_CallQueue.CallLater(RegisterGroupName, 0, false, playableId, playerGroup) // Delay for callsign init
	}
	protected void RegisterGroupName(RplId playableId, SCR_AIGroup playerGroup)
	{
		if (!playerGroup)
			return;
		// Create VoN group channels. The GROUP channel is keyed by the UNIQUE GROUP ID (GroupVonRoomName),
		// NOT the callsign number - callsign numbers can collide between groups (or be unassigned), which
		// would collapse different groups onto one channel = cross-group voice leak. (Mirrors LiteLobby.)
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		Faction faction = playerGroup.GetFaction();
		if (!faction)
			return;
		FactionKey factionKey = faction.GetFactionKey();
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, GroupVonRoomName(playerGroup.GetGroupID()));
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, "#PS-VoNRoom_Command");
		VoNRoomsManager.GetOrCreateRoomWithFaction(factionKey, "#PS-VoNRoom_Faction");
	}
	// Execute on both client and server
	/**
	 * @brief Регистрация публичного контейнера с queued обновлением fallback только на authority.
	 * @rpc Server -> Broadcast (Reliable), также локально при JIP load
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_RegisterPlayable(PS_PlayableContainer container)
	{
		m_aPlayables[container.GetRplId()] = container;
		QueueFallbackCapacityUpdate_S();
		m_CallQueue.Remove(UpdatePlayablesSortedDelayed);
		m_CallQueue.Remove(UpdatePlayablesSorted);
		m_CallQueue.Call(UpdatePlayablesSortedDelayed); // List updated resort
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke, container.GetRplId(), container); // 2 frames delayed event invoke, give group time to fully initialize
	}
	protected void OnPlayableRegisteredLateInvoke(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_CallQueue.Call(OnPlayableRegisteredLateInvoke2, playableId, playableComponent);
	}
	protected void OnPlayableRegisteredLateInvoke2(RplId playableId, PS_PlayableContainer playableComponent)
	{
		m_eOnPlayableRegistered.Invoke(playableId, playableComponent);
	}

	// --------------------------------------------------------------------------------------------
	// Remove plyable from list global list replicated
	void UnRegisterPlayable(RplId playableId)
	{
		if (Replication.IsServer())
		{
			RevokePlayableReservation_S(playableId);
			int holder = GetPlayerByPlayable(playableId);
			if (holder > 0 && GetPlayerState(holder) == PS_EPlayableControllerState.Disconected)
			{
				SetPlayerPlayable(holder, RplId.Invalid());
				SetPlayerState(holder, PS_EPlayableControllerState.NotReady);
				SetPlayerFactionKey(holder, "");
				SetPlayerPin(holder, false);
			}
		}
		RPC_UnRegisterPlayable(playableId);
		Rpc(RPC_UnRegisterPlayable, playableId);
	}
	/**
	 * @brief Удаление публичного контейнера; сервер заранее отзывает резерв отключённого владельца.
	 * @rpc Server -> Broadcast (Reliable)
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_UnRegisterPlayable(RplId playableId)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		PS_PlayableContainer playableContainer = m_aPlayables[playableId];
		m_aPlayables.Remove(playableId);
		QueueFallbackCapacityUpdate_S();

		UpdatePlayablesSorted(); // List updated resort
		m_eOnPlayableUnregistered.Invoke(playableId, playableContainer);
		playableContainer.m_eOnUnregister.Invoke();
	}

	// --------------------------------------------------------------------------------------------
	// Register vehicle to global list replicated
	// TODO: attach any entity
	void RegisterGroupVehicle(RplId rplId, SCR_AIGroup group, IEntity vehicle)
	{
		if (!Replication.IsServer())
			return;
		// This can be re-queued onto OnUpdate (below); the vehicle may have been deleted by the time it
		// runs, leaving a null reference - bail rather than dereferencing it.
		if (!vehicle)
			return;
		if (!group.m_PlayersGroup)
		{
			m_CallQueue.Call(RegisterGroupVehicle, rplId, group, vehicle);
			return;
		}
		int groupCallsign = group.GetCallsignNum();
		PS_PlayableVehicleContainer playableVehicleContainer = new PS_PlayableVehicleContainer();
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		SCR_VehicleFactionAffiliationComponent vehicleFactionAffiliationComponent = SCR_VehicleFactionAffiliationComponent.Cast(vehicle.FindComponent(SCR_VehicleFactionAffiliationComponent));
		// A vehicle missing its editable component / faction affiliation can't be turned into a
		// playable slot; bail out instead of throwing a VME from the OnUpdate retry path. Log it so a
		// genuinely-misconfigured vehicle (expected to be playable but missing a component) is findable
		// rather than silently dropped.
		if (!editableVehicleComponent || !vehicleFactionAffiliationComponent)
		{
			Print(string.Format("[PS] RegisterGroupVehicle: skipping vehicle '%1' - not a valid playable vehicle (missing SCR_EditableVehicleComponent or SCR_VehicleFactionAffiliationComponent)", vehicle), LogLevel.WARNING);
			return;
		}
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		if (!uIInfo)
			return;
		ResourceName prefab = vehicle.GetPrefabData().GetPrefabName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetResourceName();
		if (prefab == "")
			prefab = vehicle.GetPrefabData().GetPrefab().GetAncestor().GetAncestor().GetResourceName();
		playableVehicleContainer.Init(rplId, prefab, uIInfo.GetIconPath(), groupCallsign, group.m_PlayersGroup.GetGroupID(), vehicleFactionAffiliationComponent.GetDefaultFactionKey());
		Rpc(RPC_RegisterGroupVehicle, playableVehicleContainer);
		RPC_RegisterGroupVehicle(playableVehicleContainer);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_RegisterGroupVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		m_mPlayableVehicles[playableVehicleContainer.m_iRplId] = playableVehicleContainer;
	}

	// --------------------------------------------------------------------------------------------
	// Remove vehicle from  global list list replicated
	void UnRegisterGroupVehicle(RplId rplId)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RPC_UnRegisterGroupVehicle, rplId);
		RPC_UnRegisterGroupVehicle(rplId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_UnRegisterGroupVehicle(RplId rplId)
	{
		m_mPlayableVehicles.Remove(rplId);
	}

	// --------------------------------------------------------------------------------------------
	// --------------------------------------- Accessors ------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Get playable from global list, or null if no
	// - Synced on clients
	PS_PlayableContainer GetPlayableById(RplId PlayableId)
	{
		PS_PlayableContainer playableComponent;
		m_aPlayables.Find(PlayableId, playableComponent);
		return playableComponent;
	}

	// --------------------------------------------------------------------------------------------
	// Get global map of playables
	// - Synced on clients
	map<RplId, ref PS_PlayableContainer> GetPlayables()
	{
		return m_aPlayables;
	}

	// --------------------------------------------------------------------------------------------
	// Get cached list of playables sorted by CallSign -> Rank -> RplId
	// - Synced on clients
	array<PS_PlayableContainer> GetPlayablesSorted()
	{
		return m_aPlayablesSorted;
	}

	// --------------------------------------------------------------------------------------------
	// Get global map of vehicles
	// - Synced on clients
	map<RplId, ref PS_PlayableVehicleContainer> GetPlayableVehicles()
	{
		return m_mPlayableVehicles;
	}

	// --------------------------------- Player faction key ---------------------------------------
	// Get player Factionkey or empty string if no player found
	// - Synced on clients
	FactionKey GetPlayerFactionKey(int playerId)
	{
		if (!m_playersFaction.Contains(playerId))
			return "";
		return m_playersFaction[playerId];
	}
	// Get last player not null Factionkey or empty string if no player found
	// - Synced on clients
	FactionKey GetPlayerFactionKeyRemembered(int playerId)
	{
		if (!m_playersFactionRemembered.Contains(playerId))
			return "";
		return m_playersFactionRemembered[playerId];
	}
	// Set player FactionKey
	// - Execute ONLY on server
	void SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		if (!Replication.IsServer())
			return;
		// Replicate update
		RPC_SetPlayerFactionKey(playerId, factionKey);
		Rpc(RPC_SetPlayerFactionKey, playerId, factionKey);

		// Update vanilla faction
		PlayerController playerController = m_PlayerManager.GetPlayerController(playerId);
		if (!playerController)
			return;
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		playerFactionAffiliation.SetAffiliatedFactionByKey(factionKey);
		factionManager.UpdatePlayerFaction_S(playerFactionAffiliation);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected protected void RPC_SetPlayerFactionKey(int playerId, FactionKey factionKey)
	{
		FactionKey factionKeyOld = GetPlayerFactionKey(playerId);
		m_playersFaction[playerId] = factionKey;
		if (factionKey != "")
			m_playersFactionRemembered[playerId] = factionKey; // Remember last not null
		m_eOnFactionChange.Invoke(playerId, factionKey, factionKeyOld);
	}

	// ------------------------------------- Reconnect restore -----------------------------------------
	// A reconnecting player is assigned a NEW playerId, so every playerId-keyed map above is empty for
	// them. Vanilla SCR_ReconnectComponent restores the controlled character but sets the faction
	// affiliation from that entity - which is the factionless lobby/VoN body during preview/lobby/
	// briefing - leaving the player with an empty faction and therefore the WRONG side's map markers.
	// We cache the relevant state by GUID on disconnect and re-apply it on reconnect.
	/**
	 * @brief Кэширование данных слота, фракции, PIN и имени игрока по GUID при дисконнекте.
	 * @issue BUG-86
	 * @cause Игроки при реконнекте получают новый playerId, теряя привязку к слоту и фракции.
	 * @solution Захват подтверждённого UUID до потери engine mapping; резерв хранит срок и поколение отключения.
	 * @context Server
	 */
	void StorePlayerReconnectData_S(int playerId, int reconnectTime)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp)
			return;
		UUID guid;
		m_mPlayerIdentityGuid.Find(playerId, guid);
		m_mPlayerIdentityGuid.Remove(playerId);
		m_mPendingReconnectGeneration.Remove(playerId);
		m_mReconnectTraceTime.Remove(playerId);
		int generation = ++m_iConnectionGeneration;
		m_mDisconnectGeneration[playerId] = generation;
		RplId playable = GetPlayableByPlayer(playerId);
		if (!playable.IsValid() || GetPlayerByPlayable(playable) != playerId || !GetPlayableById(playable))
			return;
		float expiresAt;
		if (reconnectTime > 0)
			expiresAt = GetGame().GetWorld().GetWorldTime() + reconnectTime;
		if (!guid.IsNull())
		{
			RevokePlayerReservation_S(guid, true);
			m_mReconnectPlayable[guid] = playable;
			m_mReconnectFaction[guid] = GetPlayerFactionKey(playerId);
			m_mReconnectPin[guid] = GetPlayerPin(playerId);
			m_mReconnectName[guid] = GetPlayerName(playerId);
			m_mReconnectSourcePlayerId[guid] = playerId;
			m_mReconnectExpiresAt[guid] = expiresAt;
			m_mReconnectGeneration[guid] = generation;
		}
		if (reconnectTime > 0)
			m_CallQueue.CallLater(ExpireDisconnectedPlayer_S, reconnectTime, false, playerId, guid, playable, expiresAt, generation);
	}

	/**
	 * @brief Инициализация контекста соединения до audit, включая повторное использование playerId.
	 * @issue BUG-56, BUG-86
	 * @cause Старый callback мог потребить резерв нового соединения с тем же transient ID.
	 * @solution Монотонное поколение инвалидирует старые callbacks; UUID заполняется только после audit.
	 */
	void InitializePlayerConnection_S(int playerId)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp)
			return;
		m_mPlayerIdentityGuid[playerId] = UUID.NULL_UUID;
		m_mDisconnectGeneration[playerId] = ++m_iConnectionGeneration;
		m_mPendingReconnectGeneration.Remove(playerId);
		m_mReconnectTraceTime.Remove(playerId);
		// A reused ID is a new connection, not proof that it owns the previous account's slot.
		if (GetPlayerState(playerId) == PS_EPlayableControllerState.Disconected)
		{
			if (ApplyPlayerPlayableLink(playerId, RplId.Invalid()))
				Rpc(RPC_SetPlayerPlayable, playerId, RplId.Invalid());
			SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
			SetPlayerFactionKey(playerId, "");
			SetPlayerPin(playerId, false);
		}
	}

	void CapturePlayerIdentity_S(int playerId)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp || !m_mPlayerIdentityGuid.Contains(playerId) || !m_PlayerManager.IsPlayerConnected(playerId))
			return;
		UUID guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (!guid.IsNull())
			m_mPlayerIdentityGuid[playerId] = guid;
	}

	void RevokePlayerReservation_S(string guid, bool releaseSource = false)
	{
		if (!Replication.IsServer())
			return;
		RplId playableId = RplId.Invalid();
		int sourcePlayerId;
		int generation;
		m_mReconnectPlayable.Find(guid, playableId);
		m_mReconnectSourcePlayerId.Find(guid, sourcePlayerId);
		m_mReconnectGeneration.Find(guid, generation);
		m_mReconnectPlayable.Remove(guid);
		m_mReconnectFaction.Remove(guid);
		m_mReconnectPin.Remove(guid);
		m_mReconnectName.Remove(guid);
		m_mReconnectSourcePlayerId.Remove(guid);
		m_mReconnectExpiresAt.Remove(guid);
		m_mReconnectGeneration.Remove(guid);
		// Revocation also releases an abandoned source link when the returning player picked another slot.
		if (releaseSource && playableId.IsValid() && m_mDisconnectGeneration.Contains(sourcePlayerId)
			&& m_mDisconnectGeneration[sourcePlayerId] == generation
			&& GetPlayerState(sourcePlayerId) == PS_EPlayableControllerState.Disconected
			&& GetPlayableByPlayer(sourcePlayerId) == playableId && GetPlayerByPlayable(playableId) == sourcePlayerId)
		{
			SetPlayerPlayable(sourcePlayerId, RplId.Invalid());
			SetPlayerState(sourcePlayerId, PS_EPlayableControllerState.NotReady);
			SetPlayerFactionKey(sourcePlayerId, "");
			SetPlayerPin(sourcePlayerId, false);
		}
	}

	void RevokePlayableReservation_S(RplId playableId)
	{
		if (!Replication.IsServer() || !playableId.IsValid())
			return;
		array<string> revoked = {};
		foreach (string guid, RplId reservedPlayable : m_mReconnectPlayable)
		{
			if (reservedPlayable == playableId)
				revoked.Insert(guid);
		}
		foreach (string guid : revoked)
			RevokePlayerReservation_S(guid);
	}

	bool IsPlayableReservedForOther_S(RplId playableId, int playerId)
	{
		if (!Replication.IsServer())
			return true;
		UUID requesterGuid;
		m_mPlayerIdentityGuid.Find(playerId, requesterGuid);
		foreach (string guid, RplId reservedPlayable : m_mReconnectPlayable)
		{
			if (reservedPlayable != playableId)
				continue;
			float expiresAt = m_mReconnectExpiresAt[guid];
			if (expiresAt > 0 && GetGame().GetWorld().GetWorldTime() >= expiresAt)
				continue;
			if (requesterGuid.IsNull() || requesterGuid != guid)
				return true;
		}
		return false;
	}

	protected bool ReservationMatches_S(string guid, RplId playableId, int sourcePlayerId, float expiresAt, int generation)
	{
		return m_mReconnectGeneration.Contains(guid) && m_mReconnectGeneration[guid] == generation
			&& m_mReconnectPlayable[guid] == playableId && m_mReconnectSourcePlayerId[guid] == sourcePlayerId
			&& m_mReconnectExpiresAt[guid] == expiresAt;
	}

	/**
	 * @brief Очистка только слота и резерва захваченного отключения, даже без audited UUID.
	 * @issue BUG-86
	 * @cause Timeout по одному playerId мог удалить новый резерв или зависеть от исчезнувшего GUID mapping.
	 * @solution Сверка поколения, deadline и владельца до мутации; офлайн-голос не добавляется обратно.
	 */
	protected void ExpireDisconnectedPlayer_S(int playerId, UUID guid, RplId playableId, float expiresAt, int generation)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp || GetGame().GetWorld().GetWorldTime() < expiresAt)
			return;
		if (!guid.IsNull())
		{
			if (!ReservationMatches_S(guid, playableId, playerId, expiresAt, generation))
				return;
			RevokePlayerReservation_S(guid, true);
		}
		if (!m_mDisconnectGeneration.Contains(playerId) || m_mDisconnectGeneration[playerId] != generation)
			return;
		if (GetPlayerState(playerId) != PS_EPlayableControllerState.Disconected
			|| GetPlayableByPlayer(playerId) != playableId || GetPlayerByPlayable(playableId) != playerId)
			return;
		SetPlayerPlayable(playerId, RplId.Invalid());
		SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
		SetPlayerFactionKey(playerId, "");
		SetPlayerPin(playerId, false);
	}
	// - Execute ONLY on server
	// Diagnostic (server, once per reconnect): called from SpawnInitialEntity right after the connect-time
	// AssignPhaseVoiceChannel. If this connection is a pending RECONNECT (cached GUID data not yet consumed by
	// RestorePlayerReconnectData), record the connect time and log the IMMEDIATE VoN channel - this is before the
	// slot is restored, so it is expected to be Global. RestorePlayerReconnectData logs the matching "slot restored
	// after Xms" line, making the reconnect -> Global -> group/Command transition + timing explicit in the log.
	void TraceReconnectConnect(int playerId)
	{
		if (!Replication.IsServer())
			return;
		UUID guid;
		m_mPlayerIdentityGuid.Find(playerId, guid);
		if (guid.IsNull() || !m_mReconnectFaction.Contains(guid))
			return; // genuinely fresh join, not a reconnect
		m_mReconnectTraceTime.Set(playerId, GetGame().GetWorld().GetWorldTime());
		string channel = "";
		PS_VoNRoomsManager von = PS_VoNRoomsManager.GetInstance();
		if (von)
			channel = von.GetPlayerChannel(playerId);
		Print(string.Format("[PS_ReconnectTrace] player %1 RECONNECT connected: immediate VoN channel='%2' (slot not restored yet; expect group/Command in ~2.5s)", playerId, channel), LogLevel.NORMAL);
	}

	/**
	 * @brief Отложенный restore только после audit dispatch с контекстом резерва и соединения.
	 * @workaround Remove(fn) не фильтрует CallLater по аргументам; stale callback проверяет поколения.
	 */
	void SchedulePlayerReconnectRestore_S(int playerId)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp || !m_PlayerManager.IsPlayerConnected(playerId))
			return;
		UUID guid;
		m_mPlayerIdentityGuid.Find(playerId, guid);
		if (guid.IsNull() || !m_mReconnectGeneration.Contains(guid))
			return;
		int generation = m_mReconnectGeneration[guid];
		float expiresAt = m_mReconnectExpiresAt[guid];
		if (expiresAt > 0 && GetGame().GetWorld().GetWorldTime() >= expiresAt)
		{
			ExpireDisconnectedPlayer_S(m_mReconnectSourcePlayerId[guid], guid, m_mReconnectPlayable[guid], expiresAt, generation);
			return;
		}
		if (m_mPendingReconnectGeneration.Contains(playerId) && m_mPendingReconnectGeneration[playerId] == generation)
			return;
		m_mPendingReconnectGeneration[playerId] = generation;
		TraceReconnectConnect(playerId);
		m_CallQueue.CallLater(RestorePlayerReconnectData_S, 2500, false, playerId, guid, m_mReconnectPlayable[guid],
			m_mReconnectSourcePlayerId[guid], expiresAt, generation, m_mDisconnectGeneration[playerId]);
	}

	/**
	 * @brief Восстановление зарезервированного слота, фракции и прав управления после реконнекта игрока.
	 * @issue BUG-86
	 * @cause При дисконнекте контроллер мог удаляться раньше вызова OnPlayerDisconnected, что сбрасывало кэш слота. При восстановлении старый ID игрока мог конфликтовать со слотом.
	 * @solution Сверка UUID, срока и поколений, запрет lock/чужого holder; consume только после успешной перепривязки.
	 * @context Server
	 */
	protected void RestorePlayerReconnectData_S(int playerId, UUID guid, RplId playable, int sourcePlayerId, float expiresAt, int generation, int connectionGeneration)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp || !m_PlayerManager.IsPlayerConnected(playerId))
			return;
		if (!m_mDisconnectGeneration.Contains(playerId) || m_mDisconnectGeneration[playerId] != connectionGeneration
			|| !m_mPlayerIdentityGuid.Contains(playerId) || m_mPlayerIdentityGuid[playerId] != guid)
			return;
		if (!m_mPendingReconnectGeneration.Contains(playerId) || m_mPendingReconnectGeneration[playerId] != generation)
			return;
		m_mPendingReconnectGeneration.Remove(playerId);
		if (!ReservationMatches_S(guid, playable, sourcePlayerId, expiresAt, generation))
			return;
		RplId currentPlayable = GetPlayableByPlayer(playerId);
		int holder = GetPlayerByPlayable(playable);
		if ((expiresAt > 0 && GetGame().GetWorld().GetWorldTime() >= expiresAt)
			|| !playable.IsValid() || !GetPlayableById(playable)
			|| (currentPlayable.IsValid() && currentPlayable != playable)
			|| (holder != -1 && holder != playerId && holder != sourcePlayerId))
		{
			if (expiresAt > 0 && GetGame().GetWorld().GetWorldTime() >= expiresAt)
				ExpireDisconnectedPlayer_S(sourcePlayerId, guid, playable, expiresAt, generation);
			RevokePlayerReservation_S(guid, true);
			return;
		}
		FactionKey faction = m_mReconnectFaction[guid];
		bool pin = m_mReconnectPin[guid];
		string reconnectName;
		m_mReconnectName.Find(guid, reconnectName);
		SetPlayerPlayable(playerId, playable);
		if (GetPlayableByPlayer(playerId) != playable || GetPlayerByPlayable(playable) != playerId)
			return;
		RevokePlayerReservation_S(guid);
		if (sourcePlayerId != playerId && m_mDisconnectGeneration.Contains(sourcePlayerId)
			&& m_mDisconnectGeneration[sourcePlayerId] == generation)
		{
			SetPlayerState(sourcePlayerId, PS_EPlayableControllerState.NotReady);
			SetPlayerFactionKey(sourcePlayerId, "");
			SetPlayerPin(sourcePlayerId, false);
		}
		SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
		SetPlayerPin(playerId, pin);

		// Restore the cached player name (incl. PodvalPatches clan tag) under the new playerId
		// so UI shows the correct name immediately, without waiting for PodvalPatches to re-apply it.
		if (reconnectName != "")
			SetPlayerName(playerId, reconnectName);

		// Diagnostic - once per reconnect restore (non-spammy). Pairs with the [PS_VoNDBG] channel line that
		// follows from AssignPhaseVoiceChannel below, so reconnect slot/faction/channel can be traced together.
		Print(string.Format("[PS_ReconnectDBG] player %1 reconnect-restored: faction='%2' hasSlot=%3 pin=%4",
			playerId, faction, GetPlayableByPlayer(playerId) != RplId.Invalid(), pin), LogLevel.NORMAL);

		// If reconnecting mid-GAME into a re-linked slot, put the player back INTO their playable
		// (ApplyPlayable controls it; a dead slot routes to spectator). In lobby/briefing they have no
		// playable yet - they keep the reserved slot and get it at game start. Body-less: a reconnecting
		// player controls nothing until this runs, so without it they would be stuck spectating with
		// their slot restored but never entered.
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode && gameMode.GetState() == SCR_EGameModeState.GAME && GetPlayableByPlayer(playerId) != RplId.Invalid())
			ApplyPlayable(playerId);
		else if (gameMode)
			// Reconnect gets a NEW playerId with an empty VoN channel (RestoreRoom set it to ""); re-assign the
			// proper briefing/lobby channel for the restored slot, or the player is stuck in "" - out of their
			// group, and two such reconnects would share "" (cross-faction leak). GAME+slotted is handled by
			// ApplyPlayable above (alive proxy stays parked, dead routes to spectator -> global).
			gameMode.AssignPhaseVoiceChannel(playerId);

		// Diagnostic: pair with TraceReconnectConnect - log the POST-restore VoN channel + elapsed since connect,
		// so the reconnect -> Global -> group/Command transition + timing is unmistakable in one place.
		float traceT0;
		if (m_mReconnectTraceTime.Find(playerId, traceT0))
		{
			string channelNow = "";
			PS_VoNRoomsManager vonNow = PS_VoNRoomsManager.GetInstance();
			if (vonNow)
				channelNow = vonNow.GetPlayerChannel(playerId);
			Print(string.Format("[PS_ReconnectTrace] player %1 RECONNECT slot restored after %2ms: VoN channel now='%3' hasSlot=%4 faction='%5'",
				playerId, GetGame().GetWorld().GetWorldTime() - traceT0, channelNow, GetPlayableByPlayer(playerId) != RplId.Invalid(), faction), LogLevel.NORMAL);
			m_mReconnectTraceTime.Remove(playerId);
		}
	}

	// ------------------------------------- Player state -----------------------------------------
	// Get current player sloting state. TODO: proper naming
	// - Synced on clients
	PS_EPlayableControllerState GetPlayerState(int playerId)
	{
		PS_EPlayableControllerState state = PS_EPlayableControllerState.NotReady;
		m_playersStates.Find(playerId, state);
		return state;
	}
	// Set player sloting state
	// - Execute ONLY on server
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		if (!Replication.IsServer())
			return;
		RPC_SetPlayerState(playerId, state);
		Rpc(RPC_SetPlayerState, playerId, state);
		
		// Try start counter
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameModeCoop.GetState();
		if (gameModeState == SCR_EGameModeState.SLOTSELECTION)
		{
			m_CallQueue.Remove(StartTime);
			bool adminExist = !gameModeCoop.IsAdminMode();
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int otherPlayerId : players)
			{
				if (!adminExist)
					adminExist = SCR_Global.IsAdmin(otherPlayerId);

				PS_EPlayableControllerState playerState = m_playersStates[otherPlayerId];
				if (playerState != PS_EPlayableControllerState.Ready)
				{
					if (m_iStartTimerCounter != -1)
					{
						m_iStartTimerCounter = -1;
						Replication.BumpMe();
						OnStartTimerCounterChanged();
					}
					return;
				}
			}

			if (adminExist)
			{
				m_iStartTimerCounter = 3;
				Replication.BumpMe();
				OnStartTimerCounterChanged();
				m_CallQueue.CallLater(StartTime, 1000, true);
			}
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		m_playersStates[playerId] = state;
		m_eOnPlayerStateChange.Invoke(playerId, state);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
			if (playableContainer)
				playableContainer.GetOnPlayerStateChange().Invoke(state);
		}
	}

	// ------------------------------------ player name -------------------------------------------
	// Get cached player name or empty string if no player found
	// - Synced on clients
	string GetPlayerName(int playerId)
	{
		// Cache hit with a real name: return it. PodvalLobby/PodvalPatches store the
		// squad-prefixed display name (<b><color..>[PREFIX]</color></b>BaseName) via SetPlayerName,
		// so we must prefer the cache over the raw engine name to preserve clan tags.
		// An empty-string entry means the engine hadn't resolved the Steam name yet when
		// OnPlayerConnected fired (cache poisoned) — treat it as a miss so we re-query.
		string cachedName;
		if (m_playersLastName.Find(playerId, cachedName) && cachedName != "")
			return cachedName;
		// Cache miss or poisoned with empty: fall back to the live engine name so name-driven
		// UI (alive list, 3D label, kill feed, voice list, lobby selectors...) shows the nickname
		// instead of the role name ("Rifleman"). OnPlayerConnected re-seeds the cache with the
		// engine name on every (re)connect before PodvalLobby re-applies the prefix, so
		// cache-first never double-wraps the tag.
		if (playerId > 0)
			return GetGame().GetPlayerManager().GetPlayerName(playerId);
		return "";
	}
	// Set cached player name
	// - Execute ONLY on server
	void SetPlayerName(int playerId, string playerName)
	{
		RPC_SetPlayerName(playerId, playerName);
		Rpc(RPC_SetPlayerName, playerId, playerName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerName(int playerId, string playerName)
	{
		m_playersLastName[playerId] = playerName;
		m_eOnPlayerConnected.Invoke(playerId);
	}

	// -------------------------------- Faction ready state ---------------------------------------
	// Get faction ready state
	// - Synced on clients
	int GetFactionReady(FactionKey factionKey)
	{
		return m_mFactionReady[factionKey];
	}
	// Set faction ready state
	// - Execute ONLY on server
	void SetFactionReady(FactionKey factionKey, int readyValue)
	{
		if (!Replication.IsServer())
			return;
		RPC_SetFactionReady(factionKey, readyValue);
		Rpc(RPC_SetFactionReady, factionKey, readyValue);

		// All factions ready message already sended
		if (m_bFactionsReadySended)
			return;

		// Check is all factions ready
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		m_bFactionsReadySended = true;
		foreach (int playerId : players)
		{
			factionKey = GetPlayerFactionKey(playerId);
			if (factionKey == "")
				continue;
			if (m_mFactionReady[factionKey])
				continue;
			m_bFactionsReadySended = false;
			break;
		}

		// All factions ready — notify admins and start the 3-2-1 countdown
		if (m_bFactionsReadySended)
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("tmsg");
			invoker.Invoke(null, "#PS-Briefing_FactionsReady");

			// Reuse the same countdown as the slot-selection "all ready" timer:
			// 3 → 2 → 1 → 0 → AdvanceGameState(BRIEFING) → StartGame → GAME
			m_iStartTimerCounter = 3;
			Replication.BumpMe();
			OnStartTimerCounterChanged();
			m_CallQueue.Remove(StartTime);
			m_CallQueue.CallLater(StartTime, 1000, true);
		}
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetFactionReady(FactionKey factionKey, int readyValue)
	{
		m_mFactionReady[factionKey] = readyValue;
		m_eFactionReadyChanged.Invoke(factionKey, readyValue);
	}

	// Build a map of groupId → leaderPlayerId by iterating sorted playables.
	// The first occupied playable in each group (sorted by callsign→rank→rplId) is the leader.
	void GetGroupLeaders(out map<int, int> groupLeaders)
	{
		groupLeaders.Clear();
		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (m_PlayerManager && !m_PlayerManager.IsPlayerConnected(playerId))
				continue;
			SCR_AIGroup group = GetPlayerGroupByPlayable(playable.GetRplId());
			if (!group)
				continue;
			int groupId = group.GetGroupID();
			if (!groupLeaders.Contains(groupId))
				groupLeaders.Insert(groupId, playerId);
		}
	}

	// ------------------------------- Playable prefab name ---------------------------------------
	// Get playable prefab name (ResourceName) by PlayableRplId
	// - Synced on clients
	string GetPlayablePrefab(RplId playableId)
	{
		if (!m_mPlayablePrefabs.Contains(playableId))
			return "";
		return m_mPlayablePrefabs[playableId];
	}
	// Set playable cached prefab name (ResourceName)
	// - Execute ONLY on server
	void SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		if (!Replication.IsServer())
			return;
		Rpc(RPC_SetPlayablePrefab, playableId, prefab);
		RPC_SetPlayablePrefab(playableId, prefab);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePrefab(RplId playableId, ResourceName prefab)
	{
		m_mPlayablePrefabs[playableId] = prefab;
	}

	// ------------------------------- Playable role info (per prefab) ------------------------------
	// Store role icon/name for a prefab once (dedup). Execute ONLY on server.
	void SetPrefabRoleInfo(string prefab, string iconPath, string iconQuad, string roleName)
	{
		if (prefab == "" || m_mPrefabRoleName.Contains(prefab))
			return; // already known for this prefab
		Rpc(RPC_SetPrefabRoleInfo, prefab, iconPath, iconQuad, roleName);
		RPC_SetPrefabRoleInfo(prefab, iconPath, iconQuad, roleName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPrefabRoleInfo(string prefab, string iconPath, string iconQuad, string roleName)
	{
		m_mPrefabRoleIcon[prefab] = iconPath;
		m_mPrefabRoleQuad[prefab] = iconQuad;
		m_mPrefabRoleName[prefab] = roleName;
	}
	// Get role info for a playable (resolved playable -> prefab -> role info). Synced on clients.
	string GetPlayableRoleIconPath(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleIcon.Contains(prefab))
			return "";
		return m_mPrefabRoleIcon[prefab];
	}
	string GetPlayableRoleIconQuad(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleQuad.Contains(prefab))
			return "";
		return m_mPrefabRoleQuad[prefab];
	}
	string GetPlayableRoleName(RplId playableId)
	{
		string prefab = GetPlayablePrefab(playableId);
		if (!m_mPrefabRoleName.Contains(prefab))
			return "";
		return m_mPrefabRoleName[prefab];
	}

	// ------------------------------------ Playable name -----------------------------------------
	// Get playable cached name by playable id
	// - Synced on clients (read from the playable container, which already carries the name -
	//   a separate replicated name map would duplicate it in every JIP snapshot)
	string GetPlayableName(RplId playableId)
	{
		PS_PlayableContainer container = GetPlayableById(playableId);
		if (!container)
			return "";
		return container.GetName();
	}

	// ---------------------- playable -> player / player -> playable links -----------------------
	// Player -> Playable link
	// Get player id by playable id or -1 if no player found
	// - Synced on clients
	int GetPlayerByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayers.Contains(PlayableId))
			return -1;
		return m_playablePlayers[PlayableId];
	}
	/**
	 * @brief Получение последнего занявшего слот игрока (восстановление после гибели или дисконнекта).
	 * @issue BUG-01
	 * @cause Метод обращался к m_playersPlayableRemembered вместо m_playablePlayersRemembered, из-за чего поиск по RplId завершался неудачей и отсекал всех погибших игроков из экспорта статистики.
	 * @solution Обращение к m_playablePlayersRemembered по ключу PlayableId.
	 */
	int GetPlayerByPlayableRemembered(RplId PlayableId)
	{
		if (!m_playablePlayersRemembered.Contains(PlayableId))
			return -1;
		return m_playablePlayersRemembered[PlayableId];
	}
	// Set player -> playable / playable -> player links, by playable id to player id
	// - Execute ONLY on server
	void SetPlayablePlayer(RplId playableId, int playerId)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp)
			return;
		RplId previousPlayable = GetPlayableByPlayer(playerId);
		if (playerId < 0)
			RevokePlayableReservation_S(playableId);
		if (!ApplyPlayerPlayableLink(playerId, playableId))
			return;
		RevokePlayableReservation_S(playableId);
		RevokePlayableReservation_S(previousPlayable);
		UUID guid;
		m_mPlayerIdentityGuid.Find(playerId, guid);
		if (!guid.IsNull())
			RevokePlayerReservation_S(guid, true);
		Rpc(RPC_SetPlayablePlayer, playableId, playerId);
	}
	/**
	 * @brief Согласованная мутация связей слот <-> игрок до локальных уведомлений.
	 * @issue BUG-86
	 * @cause Старые контейнеры уведомлялись до завершения maps, а вытесненный игрок не получал события при положительном новом владельце.
	 * @solution Сначала очистить дубли и обе maps, затем уведомить каждого изменённого игрока/контейнер один раз; repeat без изменений — no-op.
	 */
	protected bool ApplyPlayerPlayableLink(int playerId, RplId playableId)
	{
		bool hasSlot = playableId.IsValid();
		if ((!hasSlot && playableId != RplId.Invalid()) || (playerId <= 0 && (!hasSlot || (playerId != -1 && playerId != -2))))
			return false;
		RplId oldPlayable = GetPlayableByPlayer(playerId);
		int oldPlayerId = -1;
		if (hasSlot)
			oldPlayerId = GetPlayerByPlayable(playableId);
		array<RplId> clearedSlots = {};
		array<int> clearedPlayers = {};
		foreach (RplId slotKey, int holder : m_playablePlayers)
		{
			if (!slotKey.IsValid() || slotKey == playableId)
				continue;
			if ((playerId > 0 && holder == playerId) || (oldPlayerId > 0 && oldPlayerId != playerId && holder == oldPlayerId))
			{
				clearedSlots.Insert(slotKey);
				clearedPlayers.Insert(holder);
			}
		}
		bool targetChanged = hasSlot && oldPlayerId != playerId;
		bool playerChanged = playerId > 0 && (oldPlayable != playableId || clearedSlots.Count() > 0 || targetChanged);
		if (!targetChanged && !playerChanged && clearedSlots.IsEmpty())
			return false;

		foreach (RplId slotKey : clearedSlots)
			m_playablePlayers[slotKey] = -1;
		if (hasSlot)
			m_playablePlayers[playableId] = playerId;
		if (playerId > 0)
			m_playersPlayable[playerId] = playableId;
		if (oldPlayerId > 0 && oldPlayerId != playerId)
			m_playersPlayable[oldPlayerId] = RplId.Invalid();
		if (playerId > 0 && hasSlot)
		{
			m_playablePlayersRemembered[playableId] = playerId;
			m_playersPlayableRemembered[playerId] = playableId;
		}

		if (oldPlayerId > 0 && oldPlayerId != playerId)
			m_eOnPlayerPlayableChange.Invoke(oldPlayerId, RplId.Invalid());
		if (playerChanged)
			m_eOnPlayerPlayableChange.Invoke(playerId, playableId);
		for (int i = 0; i < clearedSlots.Count(); i++)
		{
			PS_PlayableContainer oldContainer = m_aPlayables.Get(clearedSlots[i]);
			if (oldContainer)
				oldContainer.InvokeOnPlayerChanged(clearedPlayers[i], -1);
		}
		if (targetChanged)
		{
			PS_PlayableContainer playableContainer = m_aPlayables.Get(playableId);
			if (playableContainer)
				playableContainer.InvokeOnPlayerChanged(oldPlayerId, playerId);
		}
		return true;
	}
	/**
	 * @brief Применение публичного slot-keyed delta и вывод локальных событий из конечных maps.
	 * @rpc Server -> Broadcast (Reliable)
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePlayer(RplId playableId, int playerId)
	{
		ApplyPlayerPlayableLink(playerId, playableId);
	}

	// Playable -> Player link
	// Get playable id by player id or RplId.Invalid() if no playable found
	// - Synced on clients
	RplId GetPlayableByPlayer(int playerId)
	{
		if (!m_playersPlayable.Contains(playerId))
			return RplId.Invalid();
		return m_playersPlayable[playerId];
	}
	/**
	 * @brief Получение последнего занятого слота игрока (восстановление после реконнекта).
	 * @issue BUG-01
	 * @cause Метод обращался к m_playablePlayersRemembered вместо m_playersPlayableRemembered, из-за чего поиск по playerId давал неверные данные.
	 * @solution Обращение к m_playersPlayableRemembered по ключу playerId.
	 */
	RplId GetPlayableByPlayerRemembered(int playerId)
	{
		if (!m_playersPlayableRemembered.Contains(playerId))
			return RplId.Invalid();
		return m_playersPlayableRemembered[playerId];
	}
	// Set playable -> player / player -> playable links, by player id to playable id
	// - Execute ONLY on server
	void SetPlayerPlayable(int playerId, RplId playableId)
	{
		if (!Replication.IsServer() || m_bIsCleanedUp)
			return;
		RplId previousPlayable = GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
			RevokePlayableReservation_S(previousPlayable);
		if (!ApplyPlayerPlayableLink(playerId, playableId))
			return;
		RevokePlayableReservation_S(previousPlayable);
		RevokePlayableReservation_S(playableId);
		UUID guid;
		m_mPlayerIdentityGuid.Find(playerId, guid);
		if (!guid.IsNull())
			RevokePlayerReservation_S(guid, true);
		Rpc(RPC_SetPlayerPlayable, playerId, playableId);

		// INVARIANT: a seated player's faction ALWAYS matches their slot. The faction key is otherwise set by
		// a SEPARATE path (RPC_ChangeFactionKey) whose faction-balance check can reject while the slot is still
		// seated, and a reconnect restores the slot before the faction - either leaves a SEATED player with an
		// empty/stale faction key, which collapses their VoN channel across factions (see PS_VoNRoomsManager)
		// AND gives them the wrong side's map markers (faction affiliation drives both). Seating is already
		// balance/authorization-checked before this server method is reached (PS_PlayableControllerComponent.
		// RPC_SetPlayerPlayable, or an authoritative reconnect restore), so deriving the faction from the slot
		// here cannot bypass balance - it only stops faction and slot from ever disagreeing.
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer slot = GetPlayableById(playableId);
			if (slot && slot.GetFactionKey() != "" && GetPlayerFactionKey(playerId) != slot.GetFactionKey())
				SetPlayerFactionKey(playerId, slot.GetFactionKey());
		}
	}
	/**
	 * @brief Применение публичного player-keyed delta без второго broadcast для вытесненного игрока.
	 * @rpc Server -> Broadcast (Reliable)
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerPlayable(int playerId, RplId playableId)
	{
		ApplyPlayerPlayableLink(playerId, playableId);
	}
	
	// ------------------------------ Current playable controller ----------------------------------
	static PS_PlayableControllerComponent GetPlayableController()
	{
		return s_CurrentPlayableController;
	}

	// ----------------------------- Playable to players group link --------------------------------
	// Get players group by playable id or null if no group found
	// - Synced on clients
	SCR_AIGroup GetPlayerGroupByPlayable(RplId PlayableId)
	{
		if (!m_playablePlayerGroupId.Contains(PlayableId))
			return null;

		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(m_playablePlayerGroupId[PlayableId]);
	}

	/**
	 * @brief Получить группу игрока по его PlayerID
	 * @param playerId Идентификатор игрока
	 * @return SCR_AIGroup или null, если игрок не привязан к группе
	 */
	SCR_AIGroup GetPlayerGroup(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
			return null;
		return GetPlayerGroupByPlayable(playableId);
	}
	// Get players group int callsign by playable id or -1 if no group found
	// - Synced on clients
	int GetGroupCallsignByPlayable(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group)
			return -1;

		return group.GetCallsignNum();
	}

	// Unique per-group VoN room name, keyed by GROUP ID (not the callsign number). Callsign numbers are NOT
	// unique per group - two groups can share a callsign (or it is 0/unassigned during init), so keying group
	// voice channels by callsign collapses different groups onto ONE channel = the cross-group voice leak.
	// Group ids are unique. (Mirrors LiteLobby's group-id channel keying.) The "#PS-VoNRoom_Group" prefix
	// namespaces it and keeps it out of the digit=callsign display path; PS_VoiceRoomHeader maps it back to
	// the group's callsign name for display.
	static string GroupVonRoomName(int groupId)
	{
		if (groupId < 0)
			return "";
		return "#PS-VoNRoom_Group" + groupId.ToString();
	}
	// VoN group room name for a playable (resolves its player group). "" if the playable has no group.
	string GetGroupVonRoomName(RplId PlayableId)
	{
		SCR_AIGroup group = GetPlayerGroupByPlayable(PlayableId);
		if (!group)
			return "";
		return GroupVonRoomName(group.GetGroupID());
	}
	// Authoritative faction for a player derived from their SLOT only. Group faction is deliberately
	// excluded because TvT setups can have groups with overlapping IDs across factions, making
	// FindGroup(groupId) return a group from the wrong side. The slot's faction comes from the prefab
	// and is always reliable. "" if un-slotted (the player is legitimately factionless and belongs in
	// the Global pool). Used by the VoN reconcile safety-net and AssignPhaseVoiceChannel.
	FactionKey GetSlotFactionForPlayer(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer slot = GetPlayableById(playableId);
		if (slot && slot.GetFactionKey() != "")
			return slot.GetFactionKey();
		return "";
	}
	// Set playable players group id
	// - Execute ONLY on server
	void SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		if (!Replication.IsServer())
			return;
		RPC_SetPlayablePlayerGroupId(PlayableId, groupId);
		Rpc(RPC_SetPlayablePlayerGroupId, PlayableId, groupId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayablePlayerGroupId(RplId PlayableId, int groupId)
	{
		m_playablePlayerGroupId[PlayableId] = groupId;
		UpdatePlayablesSortedDelayed(); // Coalesced via callqueue to avoid 128x insertion sort during registration burst
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_eOnPlayableChangeGroup.Invoke(PlayableId, GetPlayableById(PlayableId), groupsManagerComponent.FindGroup(groupId));

		// Voice follows the group: re-key this player's menu VoN to the new group/command channel so the AUDIO
		// matches the (always-recomputed) widget. A group change used to leave voice stranded on the OLD group's
		// channel because nothing re-routed it - the "widget fine, audio wrong" leak. Server + menu phases only
		// (in GAME alive players are parked and the dead/spectator path owns Global).
		if (Replication.IsServer())
		{
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode && gameMode.GetState() != SCR_EGameModeState.GAME)
			{
				int groupChangePlayerId = GetPlayerByPlayable(PlayableId);
				if (groupChangePlayerId > 0)
					gameMode.AssignPhaseVoiceChannel(groupChangePlayerId);
			}
		}
	}

	// --------------------------- Vehicle to players group link ----------------------------------
	// Get players group by vehicle container or null if no group found
	// - Synced on clients
	SCR_AIGroup GetPlayerGroupByVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		SCR_GroupsManagerComponent groupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		return groupsManagerComponent.FindGroup(playableVehicleContainer.m_iGroupId);
	}
	
	// -------------------------------- Vehicle lock state ----------------------------------------
	void SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		RPC_SetPlayableVehicleLocked(vehicleId, lock);
		Rpc(RPC_SetPlayableVehicleLocked, vehicleId, lock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		if (!m_mPlayableVehicles.Contains(vehicleId))
			return;
		m_mPlayableVehicles[vehicleId].SetLock(lock);
	}

	// ---------------------------------- Player pin state ----------------------------------------
	// Get player pinned state
	// - Synced on clients
	bool GetPlayerPin(int playerId)
	{
		if (!m_playersPin.Contains(playerId))
			return false;
		return m_playersPin[playerId];
	}
	// Set player pin state
	// - Execute ONLY on server
	void SetPlayerPin(int playerId, bool pined)
	{
		if (!Replication.IsServer())
			return;
		RPC_SetPlayerPin(playerId, pined);
		Rpc(RPC_SetPlayerPin, playerId, pined);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetPlayerPin(int playerId, bool pined)
	{
		m_playersPin[playerId] = pined;
		m_eOnPlayerPinChange.Invoke(playerId, pined);
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId != RplId.Invalid())
		{
			PS_PlayableContainer playableComponent = m_aPlayables.Get(playableId);
			if (playableComponent)
				playableComponent.GetOnPlayerPinChange().Invoke(pined);
		}
	}
	
	// ------------------------------- Max server players count -----------------------------------
	// Get max players server count
	// - Synced on clients
	int GetMaxPlayers()
	{
		return m_iMaxPlayersCount;
	}
	
	// --------------------------------------------------------------------------------------------
	// ----------------------------------- Client requests ----------------------------------------
	// --------------------------------------------------------------------------------------------
	// Send message to player when he got kicked frop playable slot
	void NotifyKick(int playerId)
	{
		RPC_NotifyKick(playerId);
		Rpc(RPC_NotifyKick, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_NotifyKick(int playerId)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController && playerId == playerController.GetPlayerId())
		{
			SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
			ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("lmsg");
			invoker.Invoke(null, "#PS-Lobby_RoleKick");
		}
	}
	
	// --------------------------------------------------------------------------------------------
	// Force switch player state to game, for proper playable apply
	// Or else player can stack in other menu
	void ForceSwitch(int playerId)
	{
		// Send it ot everyone (TODO: Use owner)
		RPC_ForceSwitch(playerId);
		Rpc(RPC_ForceSwitch, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ForceSwitch(int playerId)
	{
		// Check is it our id
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController || playerController.GetPlayerId() != playerId)
			return;

		// Do full GAME state enter logic
		s_CurrentPlayableController.SwitchToMenu(SCR_EGameModeState.GAME);
	}


	// --------------------------------------------------------------------------------------------
	// ------------------------------------------ Events ------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Already replicated to clients by vanilla, just raise custom event
	protected void OnPlayerConnected(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerConnected().Invoke(playerId);
	}

	// --------------------------------------------------------------------------------------------
	// Manually replicate and invoke player disconnect event on all clients and server
	protected void OnPlayerDisconnected(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		Rpc(RPC_OnPlayerDisconnected, playerId, cause, timeout);
		RPC_OnPlayerDisconnected(playerId, cause, timeout);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerDisconnected().Invoke(playerId);
		m_eOnPlayerDisconnected.Invoke(playerId, cause, timeout);
	}

	// --------------------------------------------------------------------------------------------
	// Player got/lost admin role 
	protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		PS_PlayableContainer playableContainer = GetPlayableById(playableId);
		if (playableContainer)
			playableContainer.GetOnPlayerRoleChange().Invoke(playerId, roleFlags);

		// Leader status drives the Command (HQ) channel: re-key voice on a role change so a new leader is pulled
		// into HQ and a demoted one drops back to their group channel, keeping AUDIO in sync with the widget.
		// Nothing re-routed voice on role change before. Server + menu phases only.
		if (Replication.IsServer())
		{
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode && gameMode.GetState() != SCR_EGameModeState.GAME)
				gameMode.AssignPhaseVoiceChannel(playerId);
		}
	}

	// --------------------------------------------------------------------------------------------
	// Manually replicate and invoke damage state change event on all clients and server
	void OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		Rpc(RPC_OnPlayableDamageStateChanged, playableId, damageState);
		RPC_OnPlayableDamageStateChanged(playableId, damageState);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_OnPlayableDamageStateChanged(RplId playableId, EDamageState damageState)
	{
		if (!m_aPlayables.Contains(playableId))
			return;
		m_aPlayables[playableId].OnDamageStateChanged(damageState);
	}


	// --------------------------------------------------------------------------------------------
	// ------------------------------------ Util global -------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Remove playable entities without link to player
	// Remove playables that should be cleaned up.
	// adminClosedOnly = false (default): delete admin-closed (-2) AND unoccupied units when
	//   m_bRemoveRedundantUnits is enabled (old behavior at freeze time end — now unused).
	// adminClosedOnly = true: delete ONLY admin-closed (-2) characters and locked vehicles,
	//   leaving unassigned-but-not-closed slots alive (called at BRIEFING → GAME transition).
	void RemoveRedundantUnits(bool adminClosedOnly = false)
	{
		for (int i = 0; i < m_aPlayables.Count(); i++)
		{
			PS_PlayableContainer playable = m_aPlayables.GetElement(i);
			int playerForPlayable = GetPlayerByPlayable(playable.GetRplId());
			bool isAdminClosed = playerForPlayable == -2;
			bool isRedundant = !adminClosedOnly && playerForPlayable <= 0 && m_GameModeCoop.GetRemoveRedundantUnits();
			if (isAdminClosed || isRedundant)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetPlayableComponent().GetOwner());
				if (character)
				{
					UnparentVehicleChildren(character);
					// Unregister BEFORE deleting: DeleteEntityAndChildren triggers child component
					// callbacks (OnParentSlotChanged, etc.) that may access m_aPlayables. If the
					// playable is still registered when children are destroyed, gadgets like
					// RHS_GarminForetrexComponent crash with "Entity is already deleted".
					UnRegisterPlayable(playable.GetRplId());
					SCR_EntityHelper.DeleteEntityAndChildren(character);
					m_CallQueue.Call(RemoveRedundantUnits, adminClosedOnly);
					return;
				}
			}
		}
		
		foreach (RplId vehicleId, PS_PlayableVehicleContainer playableVehicleContainer : m_mPlayableVehicles)
		{
			if (playableVehicleContainer.GetLock())
			{
				IEntity entity = IEntity.Cast(Replication.FindItem(playableVehicleContainer.GetRplId()));
				if (entity)
				{
					// Unregister the vehicle from the replicated map before deletion so child
					// component callbacks do not reference a dying entity.
					UnRegisterGroupVehicle(playableVehicleContainer.GetRplId());
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
					m_CallQueue.Call(RemoveRedundantUnits, adminClosedOnly);
					return;
				}
			}
		}
	}
	
	protected void UnparentVehicleChildren(IEntity entity)
	{
		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity nextSibling = child.GetSibling();
			RplId childRplId = Replication.FindItemId(child);
			if (childRplId.IsValid() && m_mPlayableVehicles.Contains(childRplId))
			{
				SCR_AIGroup group = GetPlayerGroupByVehicle(m_mPlayableVehicles[childRplId]);
				entity.RemoveChild(child, true);
				if (group)
					group.AddChild(child, -1, EAddChildFlags.AUTO_TRANSFORM);
			}
			child = nextSibling;
		}
	}
	// --------------------------------------------------------------------------------------------
	// Holster weapon on all playables (TODO: check weapon stuck bug)
	void HolsterWeapons()
	{
		if (!Replication.IsServer())
			return;

		foreach (RplId id, PS_PlayableContainer playable : m_aPlayables)
		{
			playable.GetPlayableComponent().HolsterWeapon();
		}
	}

	// --------------------------------------------------------------------------------------------
	// Sort playables cached list by CallSign -> Rank -> RplId
	protected void UpdatePlayablesSorted()
	{
		array<PS_PlayableContainer> playablesSorted = {};
		map<RplId, ref PS_PlayableContainer> playables = GetPlayables();

		// Rerange playables from global list
		foreach (RplId playableId, PS_PlayableContainer playable : playables)
		{
			if (!playable)
				continue;
			int callSign = GetGroupCallsignByPlayable(playable.GetRplId());
			bool isInserted = false;
			for (int s = 0; s < playablesSorted.Count(); s++)
			{
				PS_PlayableContainer playableS = playablesSorted[s];
				int callSignS = GetGroupCallsignByPlayable(playableS.GetRplId());

				bool rplIdGreater = playableS.GetRplId() > playable.GetRplId();
				bool rankEquival = playable.GetCharacterRank() == playableS.GetCharacterRank();
				bool rankGreater = playable.GetCharacterRank() > playableS.GetCharacterRank();
				bool callSignEquival = callSignS == callSign;
				bool callSignGreater = callSignS > callSign;

				// CallSign -> Rank -> RplId
				if ((((rplIdGreater && rankEquival) || rankGreater) && callSignEquival) || callSignGreater) {
					playablesSorted.InsertAt(playable, s);
					isInserted = true;
					break;
				}
			}
			if (!isInserted) {
				playablesSorted.Insert(playable);
			}
		}

		// Update cached list
		m_aPlayablesSorted = playablesSorted;
	}
	// One frame delay
	protected void UpdatePlayablesSortedDelayed()
	{
		m_CallQueue.Remove(UpdatePlayablesSorted);
		m_CallQueue.Call(UpdatePlayablesSorted);
	}

	// --------------------------------------------------------------------------------------------
	// ---------------------------------------- Util ----------------------------------------------
	// --------------------------------------------------------------------------------------------
	// Check is player control group leader playable
	bool IsPlayerGroupLeader(int thisPlayerId)
	{
		if (thisPlayerId == -1)
			return false;

		RplId thisPlayableId = GetPlayableByPlayer(thisPlayerId);
		if (thisPlayableId == RplId.Invalid())
			return false;

		int thisGroupCallsign = GetGroupCallsignByPlayable(thisPlayableId);

		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			RplId playableId = playable.GetRplId();
			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (playerId == thisPlayerId)
				return true;
			if (GetPlayerFactionKey(playerId) != GetPlayerFactionKey(thisPlayerId))
				continue;
			int groupCallsign = GetGroupCallsignByPlayable(playableId);
			if (thisGroupCallsign != groupCallsign)
				continue;
			return false;
		}

		return true;
	}

	bool IsPlayerFactionCommander(int playerId)
	{
		RplId playableId = GetPlayableByPlayer(playerId);
		if (playableId == RplId.Invalid())
			return false;

		FactionKey factionKey = GetPlayerFactionKey(playerId);
		if (factionKey == "")
			return false;

		array<int> commanderIds = {};
		GetFactionCommanders(factionKey, commanderIds);
		return commanderIds.Contains(playerId);
	}

	// Returns the player in the top (first) slot for the given faction.
	// Only one faction commander per faction — the first playable in the sorted list.
	// Skips disconnected players so the ready button doesn't get stuck on a ghost slot.
	void GetFactionCommanders(FactionKey factionKey, out array<int> commanderIds)
	{
		commanderIds.Clear();

		array<PS_PlayableContainer> playables = GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playables)
		{
			if (playable.GetFactionKey() != factionKey)
				continue;

			int playerId = GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;
			if (m_PlayerManager && !m_PlayerManager.IsPlayerConnected(playerId))
				continue;

			commanderIds.Insert(playerId);
			return;
		}
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------ Replication -------------------------------------------
	// --------------------------------------------------------------------------------------------
	/**
	 * @brief Сериализация снимка лобби для подключающихся клиентов (JIP).
	 * @issue BUG-LockedSlotsJIP, BUG-87
	 * @cause m_iMaxPlayersCount не сериализовался в снимке JIP, из-за чего клиенты получали дефолтный лимит 1.
	 * @solution Сериализация m_iMaxPlayersCount в RplSave и десериализация в RplLoad.
	 */
	override protected bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bSlotsFullyLoaded);
		writer.WriteInt(m_iMaxPlayersCount);

		// Save maps
		// Replicate m_playablePlayers (RplId -> playerId) directly so all locked slots (playerId == -2)
		// and occupied slots (playerId > 0) are preserved without key collision upon JIP.
		PS_ReplicationHelper.WriteMapIntInt(writer, m_playersStates);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayers);
		PS_ReplicationHelper.WriteMapIntBool(writer, m_playersPin);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFaction);
		PS_ReplicationHelper.WriteMapIntFactionKey(writer, m_playersFactionRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayerGroupId);
		PS_ReplicationHelper.WriteMapIntString(writer, m_playersLastName);
		PS_ReplicationHelper.WriteMapIntRplId(writer, m_playersPlayableRemembered);
		PS_ReplicationHelper.WriteMapRplIdInt(writer, m_playablePlayersRemembered);
		PS_ReplicationHelper.WriteMapFactionKeyInt(writer, m_mFactionReady);
		PS_ReplicationHelper.WriteMapRplIdString(writer, m_mPlayablePrefabs);

		// Save per-prefab role info (deduplicated icon/quad/name)
		int roleCount = m_mPrefabRoleName.Count();
		writer.WriteInt(roleCount);
		for (int i = 0; i < roleCount; i++)
		{
			string prefab = m_mPrefabRoleName.GetKey(i);
			writer.WriteString(prefab);
			writer.WriteString(m_mPrefabRoleIcon[prefab]);
			writer.WriteString(m_mPrefabRoleQuad[prefab]);
			writer.WriteString(m_mPrefabRoleName[prefab]);
		}

		// Save containers
		int playablesCount = m_aPlayables.Count();
		writer.WriteInt(playablesCount);
		foreach (RplId id, PS_PlayableContainer container : m_aPlayables)
		{
			container.Save(writer);
		}

		int playableVehiclessCount = m_mPlayableVehicles.Count();
		writer.WriteInt(playableVehiclessCount);
		foreach (RplId id, PS_PlayableVehicleContainer container : m_mPlayableVehicles)
		{
			container.Save(writer);
		}

		// [PS_NetStat] JIP snapshot composition - logged on the SERVER each time a client joins, so it shows
		// the real per-join lobby payload at live player counts (the prime lobby-kick suspect). The whole
		// blob is sent to the joining client in one burst; watch this when lobby-stage kicks happen.
		if (PS_NetStat.s_bEnabled)
			Print(string.Format("[PS_NetStat][SERVER] JIP snapshot: playables=%1 vehicles=%2 states=%3 factions=%4 groups=%5 names=%6 roleDefs=%7",
				playablesCount, playableVehiclessCount, m_playersStates.Count(), m_playersFaction.Count(), m_playablePlayerGroupId.Count(), m_playersLastName.Count(), roleCount), LogLevel.NORMAL);

		return true;
	}

	// --------------------------------------------------------------------------------------------
	/**
	 * @brief Десериализация снимка лобби на клиенте при JIP.
	 * @issue BUG-LockedSlotsJIP, BUG-87
	 * @cause Восстановление m_playablePlayers из m_playersPlayable теряло закрытые слоты; отсутствие чтения m_iMaxPlayersCount приводило к отображению 1 вместо лимита.
	 * @solution Прямое чтение m_playablePlayers, чтение m_iMaxPlayersCount и вызов m_eOnMaxPlayersCountChanged.
	 */
	override protected bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bSlotsFullyLoaded);
		reader.ReadInt(m_iMaxPlayersCount);
		if (m_eOnMaxPlayersCountChanged)
			m_eOnMaxPlayersCountChanged.Invoke(m_iMaxPlayersCount);

		// Load maps (must match RplSave order)
		PS_ReplicationHelper.ReadMapIntInt(reader, m_playersStates);
		m_playablePlayers.Clear();
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayers);
		PS_ReplicationHelper.ReadMapIntBool(reader, m_playersPin);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFaction);
		PS_ReplicationHelper.ReadMapIntFactionKey(reader, m_playersFactionRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayerGroupId);
		PS_ReplicationHelper.ReadMapIntString(reader, m_playersLastName);
		PS_ReplicationHelper.ReadMapIntRplId(reader, m_playersPlayableRemembered);
		PS_ReplicationHelper.ReadMapRplIdInt(reader, m_playablePlayersRemembered);
		PS_ReplicationHelper.ReadMapFactionKeyInt(reader, m_mFactionReady);
		PS_ReplicationHelper.ReadMapRplIdString(reader, m_mPlayablePrefabs);

		// Load per-prefab role info (deduplicated icon/quad/name)
		int roleCount;
		reader.ReadInt(roleCount);
		for (int i = 0; i < roleCount; i++)
		{
			string prefab, icon, quad, name;
			reader.ReadString(prefab);
			reader.ReadString(icon);
			reader.ReadString(quad);
			reader.ReadString(name);
			m_mPrefabRoleIcon[prefab] = icon;
			m_mPrefabRoleQuad[prefab] = quad;
			m_mPrefabRoleName[prefab] = name;
		}

		// Reconstruct m_playersPlayable for real players only (playerId > 0)
		m_playersPlayable.Clear();
		foreach (RplId playableId, int playerId : m_playablePlayers)
		{
			if (playerId > 0)
				m_playersPlayable[playerId] = playableId;
		}

		// Load containers
		int playablesCount;
		reader.ReadInt(playablesCount);
		for (int i = 0; i < playablesCount; i++)
		{
			PS_PlayableContainer container = new PS_PlayableContainer();
			container.Load(reader);
			RPC_RegisterPlayable(container);
		}

		int playableVehiclesCount;
		reader.ReadInt(playableVehiclesCount);
		for (int i = 0; i < playableVehiclesCount; i++)
		{
			PS_PlayableVehicleContainer container = new PS_PlayableVehicleContainer();
			container.Load(reader);
			RPC_RegisterGroupVehicle(container);
		}

		m_bRplLoaded = true;

		return true;
	}
}
