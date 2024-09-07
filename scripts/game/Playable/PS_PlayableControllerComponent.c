//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableControllerComponentClass: ScriptComponentClass
{
}

// We can send rpc only from authority
// And here we are, modifying player controller since it's only what we have on client.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableControllerComponent : ScriptComponent
{
	IEntity m_eCamera;
	IEntity m_eInitialEntity;
	vector VoNPosition = PS_VoNRoomsManager.roomInitialPosition;
	SCR_EGameModeState m_eMenuState = SCR_EGameModeState.PREGAME;
	bool m_bAfterInitialSwitch = false;
	vector m_vObserverPosition = "0 0 0";
	vector lastCameraTransform[4];
	
	// Event
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_eOnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> GetOnPlayerRoleChange()
		return m_eOnPlayerRoleChange;
	
	// ------ FactionReady ------
	void SetFactionReady(FactionKey factionKey, int readyValue)
	{
		Rpc(RPC_SetFactionReady, factionKey, readyValue);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SetFactionReady(FactionKey factionKey, int readyValue)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetFactionReady(factionKey, readyValue);
	}
	
	// ------ MenuState ------
	void SetMenuState(SCR_EGameModeState state)
	{
		m_eMenuState = state;
	}
	
	SCR_EGameModeState GetMenuState()
	{
		return m_eMenuState;
	}
	
	void SwitchToMenuServer(SCR_EGameModeState state)
	{
		Rpc(RPC_SwitchToMenuServer, state);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_SwitchToMenuServer(SCR_EGameModeState state)
	{
		SwitchToMenu(state);
	}
	
	void SwitchToMenu(SCR_EGameModeState state)
	{
		SetMenuState(state);
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.PreviewMapMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CoopLobby);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CutsceneMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.BriefingMapMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.FadeToGame);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.DebriefingMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.PlayableRespawnMenu);
		switch (state) 
		{
			case SCR_EGameModeState.PREGAME:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.PreviewMapMenu);
				break;
			case SCR_EGameModeState.SLOTSELECTION:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
				break;
			case SCR_EGameModeState.CUTSCENE:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CutsceneMenu);
				break;
			case SCR_EGameModeState.BRIEFING:
				PS_CutsceneManager.GetInstance().PreloadWorld();
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
				break;
			case SCR_EGameModeState.GAME:
				GetGame().GetCallqueue().Call(ApplyPlayable);
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.FadeToGame);
				break;
			case SCR_EGameModeState.DEBRIEFING:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.DebriefingMenu);
				break;
			case SCR_EGameModeState.POSTGAME:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.DebriefingMenu);
				break;
		}
	}
	
	void AdvanceGameState(SCR_EGameModeState state)
	{
		Rpc(RPC_AdvanceGameState, state);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_AdvanceGameState(SCR_EGameModeState state)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameMode.AdvanceGameState(state);
	}
	
	void LoadMission(string missionName)
	{
		Rpc(RPC_LoadMission, missionName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_LoadMission(string missionName)
	{
		SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
		// It's litteraly broken on dedicated.
		saveManager.RestartAndLoad(missionName);
	}
	
	// ------ FactionLock ------
	void FactionLockSwitch()
	{
		Rpc(RPC_FactionLockSwitch);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_FactionLockSwitch()
	{
		// only admins can change faction lock
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playerRole == EPlayerRole.NONE) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameMode.FactionLockSwitch();
	}
	
	// Just don't look at it.
	override protected void OnPostInit(IEntity owner)
	{
		SetEventMask(GetOwner(), EntityEvent.POSTFIXEDFRAME);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(PlayerController.Cast(GetOwner()));
		playerController.m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameModeCoop)
			return;
		
		ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> onPlayerRoleChanged = gameModeCoop.GetOnPlayerRoleChange();
		if (!onPlayerRoleChanged)
			return;
		
		onPlayerRoleChanged.Insert(OnPlayerRoleChange);
	}
	
	void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		m_eOnPlayerRoleChange.Invoke(playerId, roleFlags);
	}
	
	// We change to VoN boi lets enable camera
	private void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		// Write entity change to replay
		if (Replication.IsServer()) {
			RplId toRplId = RplId.Invalid();
			if (to) {
				RplComponent rplTo = RplComponent.Cast(to.FindComponent(RplComponent));
				toRplId = rplTo.Id();
			}
			PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		}
		
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!rpl.IsOwner()) return;
		if (!from && !m_bAfterInitialSwitch) return;
		if (!to && !m_bAfterInitialSwitch) return;
		if (!to) {
			m_vObserverPosition = from.GetOrigin();
		}		
		m_bAfterInitialSwitch = true;
		
		PS_LobbyVoNComponent vonFrom;
		if (from) vonFrom = PS_LobbyVoNComponent.Cast(from.FindComponent(PS_LobbyVoNComponent));
		PS_LobbyVoNComponent vonTo;
		if (to) vonTo = PS_LobbyVoNComponent.Cast(to.FindComponent(PS_LobbyVoNComponent));
		if (vonTo && !vonFrom)
		{
			if (from) m_vObserverPosition = from.GetOrigin();
			SwitchToObserver(from);
		}
		if (!vonTo) SwitchFromObserver();
	}
	
	// Yes every frame, just don't look at it.
	override protected void EOnPostFixedFrame(IEntity owner, float timeSlice)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl.IsOwner())
			return;
		
		// Lets fight with phisyc engine
		if (m_eInitialEntity)
		{
			//vector currentOrigin = m_eInitialEntity.GetOrigin();
			//if (currentOrigin == VoNPosition) return;
			//Print("Move to: " + currentOrigin.ToString());
			
			m_eInitialEntity.SetOrigin(VoNPosition);
			
			// Who broke camera on map?
			CameraBase cameraBase = GetGame().GetCameraManager().CurrentCamera();
			if (cameraBase)
				cameraBase.ApplyTransform(timeSlice);
			
			Physics physics = m_eInitialEntity.GetPhysics();
			if (physics)
			{
				physics.SetVelocity("0 0 0");
				physics.SetAngularVelocity("0 0 0");
				physics.SetMass(0);
				physics.SetDamping(1, 1);
				//physics.ChangeSimulationState(SimulationState.NONE);
				physics.SetActive(ActiveState.INACTIVE);
			}
		} else {
			PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
			IEntity entity = thisPlayerController.GetControlledEntity();
			if (entity)
			{
				PS_LobbyVoNComponent von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
				if (von) m_eInitialEntity = entity;
			}
		}
	}
	
	// Save VoN boi for reuse
	IEntity GetInitialEntity()
	{
		return m_eInitialEntity;
	}
	void SetInitialEntity(IEntity initialEntity)
	{
		m_eInitialEntity = initialEntity;
	}
	
	void ChangeFactionKey(int playerId, FactionKey factionKey)
	{
		Rpc(RPC_ChangeFactionKey, playerId, factionKey)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ChangeFactionKey(int playerId, FactionKey factionKey)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		
		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()) && !gameModeCoop.CanJoinFaction(factionKey, playableManager.GetPlayerFactionKey(playerId)))
			return;
		
		if (thisPlayerController.GetPlayerId() != playerId && playerRole == EPlayerRole.NONE) return;
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE) return;
		
		playableManager.SetPlayerFactionKey(playerId, factionKey);
	}
	
	// ------------------ VoN controlls ------------------
	void MoveToVoNRoomByKey(int playerId, string roomKey)
	{
		string factionKey = "";
		string roomName = "#PS-VoNRoom_Global";
		
		if (roomKey.Contains("|")) {
			array<string> outTokens = new array<string>();
			roomKey.Split("|", outTokens, false);
			factionKey = outTokens[0];
			roomName = outTokens[1];
		}
		
		Rpc(RPC_MoveVoNToRoom, playerId, factionKey, roomName);
	}
	void MoveToVoNRoom(int playerId, FactionKey factionKey, string roomName)
	{
		Rpc(RPC_MoveVoNToRoom, playerId, factionKey, roomName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_MoveVoNToRoom(int playerId, FactionKey factionKey, string roomName)
	{		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		VoNRoomsManager.MoveToRoom(playerId, factionKey, roomName);
	}
	
	
	PS_LobbyVoNComponent GetVoN()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		PS_LobbyVoNComponent von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		return von;
	}
	RadioTransceiver GetVoNTransiver()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast( entity.FindComponent(SCR_GadgetManagerComponent) );
		IEntity radioEntity = gadgetManager.GetGadgetByType(EGadgetType.RADIO);
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		transiver.SetFrequency(1);
		return transiver;
	}
	void LobbyVoNEnable()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(null);
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(true);
	}
	void LobbyVoNRadioEnable()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	void LobbyVoNDisable()
	{
		// Delay VoN disable
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, PS_LobbyVoNComponent.PS_TRANSMISSION_TIMEOUT_MS);
	}
	void LobbyVoNDisableDelayed()
	{
		PS_LobbyVoNComponent von = GetVoN();
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	// Separate radio VoNs, CALL IT FROM SERVER
	void SetVoNKey(string VoNKey)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		if (!entity) return;
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast( entity.FindComponent(SCR_GadgetManagerComponent) );
		IEntity radioEntity = gadgetManager.GetGadgetByType(EGadgetType.RADIO);
		if (radioEntity)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
			radio.SetEncryptionKey(VoNKey);
		}
	}
	bool isVonInit()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast( entity.FindComponent(SCR_GadgetManagerComponent) );
		IEntity radioEntity = gadgetManager.GetGadgetByType(EGadgetType.RADIO);
		return radioEntity;
	}
	
	// ------------------ Observer camera controlls ------------------
	void SaveCameraTransform()
	{
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_CameraEditorComponent, false));
		cameraManager.GetLastCameraTransform(lastCameraTransform);
	}
	
	void SwitchToObserver(IEntity from)
	{
		if (m_eCamera) return;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.SpectatorMenu);
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		EntitySpawnParams params = new EntitySpawnParams();
		if (from) from.GetTransform(params.Transform);
		MoveToVoNRoom(thisPlayerController.GetPlayerId(), "", "");
      Resource resource = Resource.Load("{6EAA30EF620F4A2E}Prefabs/Editor/Camera/ManualCameraSpectator.et");
      m_eCamera = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		
		if (lastCameraTransform[3][1] < 10000 && lastCameraTransform[3][1] > 0)
		{
			m_eCamera.SetTransform(lastCameraTransform);
			lastCameraTransform[3][1] = 10000;
		} else if (m_vObserverPosition != "0 0 0") { 
			m_eCamera.SetOrigin(m_vObserverPosition);
			m_vObserverPosition = "0 0 0";
		} else {
			SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
			m_eCamera.SetOrigin(mapEntity.Size() / 2.0 + vector.Up * 100);
		}
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	}
	
	void SwitchFromObserver()
	{
		if (!m_eCamera) return;
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.SpectatorMenu);
		SCR_EntityHelper.DeleteEntityAndChildren(m_eCamera);
		m_eCamera = null;
	}
	
	
	// Force change game state
	void ForceGameStart()
	{
		Rpc(RPC_ForceGameStart)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ForceGameStart()
	{
		// only admins can force start
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (SCR_Global.IsAdmin(thisPlayerController.GetPlayerId())) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME)
			gameMode.StartGameMode();
	}
	
	void ForceSwitch(int playerId)
	{
		Rpc(RPC_ForceSwitch, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ForceSwitch(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.ForceSwitch(playerId)
	}
	
	// Get controll on selected playable entity
	void ApplyPlayable()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		if (playableManager.GetPlayableByPlayer(thisPlayerController.GetPlayerId()) == RplId.Invalid()) SwitchToObserver(null);
		Rpc(RPC_ApplyPlayable);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ApplyPlayable()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = PlayerController.Cast(GetOwner());
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		playableManager.ApplyPlayable(playerController.GetPlayerId());
	}
	
	void UnpinPlayer(int playerId)
	{
		Rpc(RPC_UnpinPlayer, playerId)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_UnpinPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		
		playableManager.SetPlayerPin(playerId, false);
	}
	
	void KickPlayer(int playerId)
	{
		Rpc(RPC_KickPlayer, playerId)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_KickPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playerRole == EPlayerRole.NONE) return;
		
		playerManager.KickPlayer(playerId, PlayerManagerKickReason.KICK, 0);
	}
		
	// -------------------- Set ---------------------
	void SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		Rpc(RPC_SetPlayerState, playerId, state)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerState(int playerId, PS_EPlayableControllerState state)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// If not admin you can change only herself
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (thisPlayerController.GetPlayerId() != playerId && playerRole == EPlayerRole.NONE) return;
		
		playableManager.SetPlayerState(playerId, state);
	}
	
	void SetPlayablePlayer(RplId playableId, int playerId)
	{
		Rpc(RPC_SetPlayablePlayer, playableId, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayablePlayer(RplId playableId, int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// You can't change playable if pinned and not admin
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE) return;
		
		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
		if (playableComponent)
		{
			FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			if (playerId >= 0 && !SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()) && !gameModeCoop.CanJoinFaction(factionKey, playableManager.GetPlayerFactionKey(playerId)))
				return;
		}
		
		playableManager.SetPlayablePlayer(playableId, playerId);
	}
	
	void SetPlayerPlayable(int playerId, RplId playableId) 
	{
		Rpc(RPC_SetPlayerPlayable, playerId, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayerPlayable(int playerId, RplId playableId) 
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// You can't change playable if pinned and not admin
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE) return;
		
		// don't check other staff if empty playable
		if (playableId == RplId.Invalid()) {
			if (playerId != thisPlayerController.GetPlayerId())
				playableManager.NotifyKick(playerId);
			playableManager.SetPlayerPlayable(playerId, playableId);
			return;
		}
		
		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
		if (playableComponent)
		{
			FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			if (playerId >= 0 && !SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()) && !gameModeCoop.CanJoinFaction(factionKey, playableManager.GetPlayerFactionKey(playerId)))
				return;
		}
		
		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
		
		// Check is playable already selected or dead
		int curretPlayerId = playableManager.GetPlayerByPlayable(playableId);
		if (playableCharacter.GetDamageManager().IsDestroyed() || (curretPlayerId != -1 && curretPlayerId != playerId)) {
			return;
		}
		
		playableManager.SetPlayerPlayable(playerId, playableId);
		
		// Pin player if setted by admin
		if (playerId != thisPlayerController.GetPlayerId()) playableManager.SetPlayerPin(playerId, true);	
	}
	
	void SetObjectiveCompleteState(PS_Objective objective, bool complete)
	{
		RplId objectiveId = objective.GetRplId();
		Rpc(RPC_SetObjectiveCompleteState, objectiveId, complete);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SetObjectiveCompleteState(RplId objectiveId, bool complete)
	{
		PS_Objective objective = PS_Objective.Cast(Replication.FindItem(objectiveId));
		if (objective)
			objective.SetCompleted(complete);
	}
}