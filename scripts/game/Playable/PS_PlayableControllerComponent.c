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
	
	// ------ MenuState ------
	void SetMenuState(SCR_EGameModeState state)
	{
		m_eMenuState = state;
	}
	
	SCR_EGameModeState GetMenuState()
	{
		return m_eMenuState;
	}
	
	void SwitchToMenu(SCR_EGameModeState state)
	{
		Print("SwitchToMenu: " + state.ToString());
		SetMenuState(state);
		GetGame().GetMenuManager().GetTopMenu().Close();
		switch (state) 
		{
			case SCR_EGameModeState.PREGAME:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
				break;
			case SCR_EGameModeState.SLOTSELECTION:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
				break;
			case SCR_EGameModeState.BRIEFING:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
				break;
			case SCR_EGameModeState.GAME:
				ApplyPlayable();
				break;
			case SCR_EGameModeState.POSTGAME:
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
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
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameMode.FactionLockSwitch();
	}
	
	// Just don't look at it.
	override protected void OnPostInit(IEntity owner)
	{
		SetEventMask(GetOwner(), EntityEvent.POSTFIXEDFRAME);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(PlayerController.Cast(GetOwner()));
		playerController.m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
	}
	
	// We change to VoN boi lets enable camera
	private void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if (!from && !m_bAfterInitialSwitch) return;
		if (!to && !m_bAfterInitialSwitch) return;
		m_bAfterInitialSwitch = true;
		
		PS_LobbyVoNComponent vonFrom;
		if (from) vonFrom = PS_LobbyVoNComponent.Cast(from.FindComponent(PS_LobbyVoNComponent));
		PS_LobbyVoNComponent vonTo;
		if (to) vonTo = PS_LobbyVoNComponent.Cast(to.FindComponent(PS_LobbyVoNComponent));
		if (vonTo && !vonFrom)
		{
			SwitchToObserver(from);
		}
	}
	
	// Yes every frame, just don't look at it.
	override protected void EOnPostFixedFrame(IEntity owner, float timeSlice)
	{
		// Lets fight with phisyc engine
		if (m_eInitialEntity)
		{
			m_eInitialEntity.SetOrigin(VoNPosition);
			Physics physics = m_eInitialEntity.GetPhysics();
			physics.SetVelocity("0 0 0");
			physics.SetAngularVelocity("0 0 0");
			physics.SetMass(0);
			physics.SetDamping(1, 1);
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
		if (thisPlayerController.GetPlayerId() != playerId && playerRole != EPlayerRole.ADMINISTRATOR) return;
		if (playableManager.GetPlayerPin(playerId) && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		playableManager.SetPlayerFactionKey(playerId, factionKey);
	}
	
	// ------------------ VoN controlls ------------------
	void MoveToVoNRoomByKey(int playerId, string roomKey)
	{
		string factionKey = "";
		string groupName = "";
		
		if (roomKey.Contains("|")) {
			array<string> outTokens = new array<string>();
			roomKey.Split("|", outTokens, false);
			factionKey = outTokens[0];
			groupName = outTokens[1];
		}
		Rpc(RPC_MoveVoNToRoom, playerId, factionKey, groupName);
	}
	void MoveToVoNRoom(int playerId, FactionKey factionKey, string roomName)
	{
		Rpc(RPC_MoveVoNToRoom, playerId, factionKey, roomName);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_MoveVoNToRoom(int playerId, FactionKey factionKey, string roomName)
	{
		// If not admin you can change only herself
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		if (thisPlayerController.GetPlayerId() != playerId && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.BRIEFING) { // On briefing also separate to squads
			SetVoNKey("Menu" + factionKey + roomName);
		}
		else SetVoNKey("Menu" + factionKey); // Сhange VoN zone
		
		
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
		return RadioTransceiver.Cast(transiver);
	}
	void LobbyVoNEnable()
	{
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(null);
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(true);
	}
	void LobbyVoNDisable()
	{
		PS_LobbyVoNComponent von = GetVoN();
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	void LobbyVoNRadioEnable()
	{
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	// Separate radio VoNs, CALL IT FROM SERVER
	void SetVoNKey(string VoNKey)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast( entity.FindComponent(SCR_GadgetManagerComponent) );
		IEntity radioEntity = gadgetManager.GetGadgetByType(EGadgetType.RADIO);
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetEncryptionKey(VoNKey);
	}
	
	// ------------------ Observer camera controlls ------------------
	void SwitchToObserver(IEntity from)
	{
		if (m_eCamera) return;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.SpectatorMenu);
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		EntitySpawnParams params = new EntitySpawnParams();
		if (from) from.GetTransform(params.Transform);
        Resource resource = Resource.Load("{127C64F4E93A82BC}Prefabs/Editor/Camera/ManualCameraPhoto.et");
        m_eCamera = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
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
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() == SCR_EGameModeState.PREGAME)
			gameMode.StartGameMode();
	}
	
	// Get controll on selected playable entity
	void ApplyPlayable()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager.GetPlayableByPlayer(thisPlayerController.GetPlayerId()) == RplId.Invalid()) SwitchToObserver(null);
		Rpc(RPC_ApplyPlayable)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ApplyPlayable()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = PlayerController.Cast(GetOwner());
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
		if (playerRole != EPlayerRole.ADMINISTRATOR) return;
		
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
		if (thisPlayerController.GetPlayerId() != playerId && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		playableManager.SetPlayerState(playerId, state);
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
		if (playableManager.GetPlayerPin(playerId) && playerRole != EPlayerRole.ADMINISTRATOR) return;
		
		// don't check other staff if empty playable
		if (playableId == RplId.Invalid()) {
			playableManager.SetPlayerPlayable(playerId, playableId);
			return;
		}
		
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
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
	
	
	
}