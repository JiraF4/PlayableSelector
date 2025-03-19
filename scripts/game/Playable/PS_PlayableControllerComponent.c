[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableControllerComponentClass : ScriptComponentClass
{
}

// We can send rpc only from authority
// And here we are, modifying player controller since it's only what we have on client.
class PS_PlayableControllerComponent : ScriptComponent
{
	protected IEntity m_Camera;
	protected IEntity m_InitialEntity;
	protected vector m_vVoNPosition = PS_VoNRoomsManager.roomInitialPosition;
	protected SCR_EGameModeState m_eMenuState = SCR_EGameModeState.PREGAME;
	protected bool m_bAfterInitialSwitch = false;
	protected vector m_vObserverPosition = "0 0 0";
	protected vector lastCameraTransform[4];

	void SetVoNPosition(vector VoNPosition)
	{
		m_vVoNPosition = VoNPosition;
	}
	
	[RplProp()]
	bool m_bOutFreezeTime;
	
	void SetOutFreezeTime(bool outFreezeTime)
	{
		Rpc(RPC_SetOutFreezeTime, outFreezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_SetOutFreezeTime(bool outFreezeTime)
	{
		m_bOutFreezeTime = outFreezeTime;
	}
	
	// Event
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_eOnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> GetOnPlayerRoleChange()
	{
		return m_eOnPlayerRoleChange;
	}

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
		if (playerRole == EPlayerRole.NONE)
			return;

		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		gameMode.FactionLockSwitch();
	}

	// ------ FreezeTimer ------
	void FreezeTimerAdvance(int time)
	{
		Rpc(RPC_FreezeTimerAdvance, time);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_FreezeTimerAdvance(int time)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.FreezeTimerAdvance(time);
	}
	void FreezeTimerEnd()
	{
		Rpc(RPC_FreezeTimerEnd);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_FreezeTimerEnd()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.FreezeTimerEnd();
	}
	
	// ------ SpawnPrefab ------
	void SpawnPrefab(string GUID, vector position)
	{
		IEntity camera = GetGame().GetCameraManager().CurrentCamera();
		if (position == "0 0 0")
			position = camera.GetOrigin();
		Rpc(RPC_SpawnPrefab, position, GUID);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SpawnPrefab(vector position, string GUID)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

		Resource resource = Resource.Load(GUID);
		EntitySpawnParams entitySpawnParams = new EntitySpawnParams();
		Math3D.MatrixIdentity4(entitySpawnParams.Transform);
		entitySpawnParams.Transform[3] = position;

		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), entitySpawnParams);
		if (!entity)
			return;
		Physics physics = entity.GetPhysics();
		if (physics)
			physics.SetActive(ActiveState.ACTIVE);
	}
	
	// ------ SpawnAdministrator ------
	void SpawnAdministrator(vector position)
	{
		Rpc(RPC_SpawnAdministrator, position);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SpawnAdministrator(vector position)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

		Resource resource = Resource.Load("{3C87CA398115BBD4}Prefabs/Characters/Core/Character_Administrator.et");
		EntitySpawnParams entitySpawnParams = new EntitySpawnParams();
		Math3D.MatrixIdentity4(entitySpawnParams.Transform);
		entitySpawnParams.Transform[3] = position;

		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), entitySpawnParams);
		if (!entity)
			return;
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (rpl)
			rpl.GiveExt(thisPlayerController.GetRplIdentity(), false);
		thisPlayerController.SetControlledEntity(entity);
	}
	
	// ------ RespawnPlayable ------
	void RespawnPlayable(RplId playableId, bool useInitPosition)
	{
		if (Replication.IsServer())
			RPC_RespawnPlayable(playableId, useInitPosition);
		else
			Rpc(RPC_RespawnPlayable, playableId, useInitPosition);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_RespawnPlayable(RplId playableId, bool useInitPosition)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;
		
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(playableId));
		if (!rplComponent)
			return;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(rplComponent.GetEntity());
		if (!character)
			return;

		EntityPrefabData prefab = character.GetPrefabData();
		if (!prefab)
			return;
		
		PS_PlayableComponent oldPlayableComponent = character.PS_GetPlayable();
		EntitySpawnParams params = new EntitySpawnParams();
		if (useInitPosition)
			oldPlayableComponent.GetSpawnTransform(params.Transform);
		else
			character.GetWorldTransform(params.Transform);
		
		SCR_ChimeraCharacter newCharacter = SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load(prefab.GetPrefabName()), GetGame().GetWorld(), params));
		PS_PlayableComponent playableContainer = newCharacter.PS_GetPlayable();

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_AIGroup aiGroup = playableManager.GetPlayerGroupByPlayable(oldPlayableComponent.GetRplId());
		SCR_AIGroup playabelGroup = aiGroup.m_BotsGroup;
		playabelGroup.AddAIEntityToGroup(newCharacter);
		playableManager.SetPlayablePlayerGroupId(playableContainer.GetRplId(), aiGroup.GetGroupID());

		playableContainer.SetPlayable(true);
		oldPlayableComponent.SetPlayable(false);

		character.GetDamageManager().Kill(Instigator.CreateInstigator(newCharacter));
		character.GetDamageManager().SetHealthScaled(0);
		GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableComponent, newCharacter, playableContainer);
	}

	// ------ ForceRespawnPlayer ------
	void ForceRespawnPlayer(bool initPosition = false)
	{
		IEntity camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
			return;
		PS_ManualCameraSpectator cameraSpectator = PS_ManualCameraSpectator.Cast(camera);

		SCR_ChimeraCharacter character;
		if (cameraSpectator)
			character = SCR_ChimeraCharacter.Cast(cameraSpectator.GetCharacterEntity());
		if (!character)
		{
			SCR_AttachEntity attachEntity = SCR_AttachEntity.Cast(camera.GetParent());
			if (!attachEntity)
				return;

			character = SCR_ChimeraCharacter.Cast(attachEntity.GetTarget());
		}
		if (!character)
			return;

		Rpc(RPC_ForceRespawnPlayer, Replication.FindId(character), initPosition);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ForceRespawnPlayer(RplId respawnEntityRplId, bool initPosition)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(Replication.FindItem(respawnEntityRplId));
		if (!character)
			return;

		EntityPrefabData prefab = character.GetPrefabData();
		if (!prefab)
			return;

		PS_PlayableComponent oldPlayableComponent = character.PS_GetPlayable();
		EntitySpawnParams params = new EntitySpawnParams();
		if (initPosition)
			oldPlayableComponent.GetSpawnTransform(params.Transform);
		else
			character.GetWorldTransform(params.Transform);

		SCR_ChimeraCharacter newCharacter = SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load(prefab.GetPrefabName()), GetGame().GetWorld(), params));
		PS_PlayableComponent playableContainer = newCharacter.PS_GetPlayable();

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_AIGroup aiGroup = playableManager.GetPlayerGroupByPlayable(oldPlayableComponent.GetRplId());
		SCR_AIGroup playabelGroup = aiGroup.m_BotsGroup;
		playabelGroup.AddAIEntityToGroup(newCharacter);
		playableManager.SetPlayablePlayerGroupId(playableContainer.GetRplId(), aiGroup.GetGroupID());

		playableContainer.SetPlayable(true);
		oldPlayableComponent.SetPlayable(false);

		character.GetDamageManager().Kill(Instigator.CreateInstigator(newCharacter));
		character.GetDamageManager().SetHealthScaled(0);
		GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableComponent, newCharacter, playableContainer);
	}

	void RPC_ForceRespawnPlayerLate(SCR_ChimeraCharacter character, PS_PlayableComponent oldPlayableComponent, SCR_ChimeraCharacter newCharacter, PS_PlayableComponent playableContainer)
	{
		character.GetDamageManager().Kill(Instigator.CreateInstigator(newCharacter));
		character.GetDamageManager().SetHealthScaled(0);
		if (!character.GetDamageManager().IsDestroyed())
		{
			GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableComponent, newCharacter, playableContainer);
			return;
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		SCR_AIGroup aiGroup = playableManager.GetPlayerGroupByPlayable(oldPlayableComponent.GetRplId());
		SCR_AIGroup playabelGroup = aiGroup.GetSlave();
		playabelGroup.AddAIEntityToGroup(character);
		playableManager.SetPlayablePlayerGroupId(playableContainer.GetRplId(), aiGroup.GetGroupID());
		int playerId = playableManager.GetPlayerByPlayableRemembered(oldPlayableComponent.GetRplId());
		VoNRoomsManager.MoveToRoom(playerId, "", "");
		if (playerId > -1)
		{
			GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate2, 500, false, playerId, playableContainer);
		}
	}
	void RPC_ForceRespawnPlayerLate2(int playerId, PS_PlayableComponent playable)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPlayable(playerId, playable.GetRplId());
		ForceSwitch(playerId);
	}

	// Just don't look at it.
	override protected void OnPostInit(IEntity owner)
	{
		SetEventMask(GetOwner(), EntityEvent.POSTFIXEDFRAME);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
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
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		
		// Write entity change to replay
		if (Replication.IsServer()) {
			RplId toRplId = RplId.Invalid();
			if (to) {
				RplComponent rplTo = RplComponent.Cast(to.FindComponent(RplComponent));
				toRplId = rplTo.Id();
			}
		}

		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!rpl.IsOwner())
			return;
		if (!from && !m_bAfterInitialSwitch)
			return;
		if (!to && !m_bAfterInitialSwitch)
			return;
		if (!to) {
			m_vObserverPosition = from.GetOrigin();
		}
		m_bAfterInitialSwitch = true;
		
		PS_LobbyVoNComponent vonFrom;
		if (from)
			vonFrom = PS_LobbyVoNComponent.Cast(from.FindComponent(PS_LobbyVoNComponent));
		PS_LobbyVoNComponent vonTo;
		if (to)
			vonTo = PS_LobbyVoNComponent.Cast(to.FindComponent(PS_LobbyVoNComponent));
		if (!vonTo)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameModeCoop.GetState() == SCR_EGameModeState.GAME)
				GetGame().GetCallqueue().Call(TellFuckingEditorCoreThanWeAlive, thisPlayerController.GetPlayerId(), to);
		}
		if (vonTo && !vonFrom)
		{
			if (from)
				m_vObserverPosition = from.GetOrigin();
			SwitchToObserver(from);
		}
		if (!vonTo)
		{
			SwitchFromObserver();
		}
	}
	
	// There is sure no ебанорго game modes without spawns, yeah sure блять
	void TellFuckingEditorCoreThanWeAlive(int playerId, IEntity entity)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.IsFreezeTimeEnd() && gameModeCoop.GetDisableBuildingModeAfterFreezeTime())
			 return;
		SCR_BaseGameMode.Cast(GetGame().GetGameMode()).GetOnPlayerSpawned().Invoke(playerId, entity);
		Rpc(AndFuckingServerTo, playerId, Replication.FindId(entity))
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void AndFuckingServerTo(int playerId, RplId entityId)
	{
		IEntity entity = IEntity.Cast(Replication.FindItem(entityId));
		SCR_BaseGameMode.Cast(GetGame().GetGameMode()).GetOnPlayerSpawned().Invoke(playerId, entity);
	}
	
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if ((gameMode.GetState() == SCR_EGameModeState.GAME && gameMode.IsFreezeTimeEnd()) || !gameMode.IsFreezeTimeShootingForbiden())
		{
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
			return;
		}
		
		if (PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = PlayerController.Cast(owner);
		ActionManager actionManager = playerController.GetActionManager();
		if (!actionManager)
			return;
		
		actionManager.SetActionValue("CharacterFire", 0);
		actionManager.SetActionValue("CharacterThrowGrenade", 0);
		actionManager.SetActionValue("CharacterMelee", 0);
		actionManager.SetActionValue("CharacterFireStatic", 0);
		actionManager.SetActionValue("TurretFire", 0);
		actionManager.SetActionValue("VehicleFire", 0);
		actionManager.SetActionValue("VehicleHorn", 0);
		
		IEntity character = playerController.GetControlledEntity();
		if (character)
		{
			Vehicle vehicle = Vehicle.Cast(character.GetRootParent());
			if (vehicle)
			{
				if (!vehicle.IsEnableMoveOnFreeze())
				{
					DisableVehicleMove(actionManager);
					VehicleWheeledSimulation vehicleWheeledSimulation = VehicleWheeledSimulation.Cast(vehicle.FindComponent(VehicleWheeledSimulation));
					if (vehicleWheeledSimulation)
					{
						if (vehicleWheeledSimulation.EngineIsOn())
							vehicleWheeledSimulation.EngineStop();
					}
				}
			}
		}
		
		if (m_bOutFreezeTime)
		{
			actionManager.SetActionValue("CharacterForward", 0);
			actionManager.SetActionValue("CharacterRight", 0);
			actionManager.SetActionValue("CharacterTurnUp", 0);
			actionManager.SetActionValue("CharacterTurnRight", 0);
			actionManager.SetActionValue("CharacterTurnUp", 0);
			actionManager.SetActionValue("CharacterTurnRight", 0);
			actionManager.SetActionValue("GetOut", 0);
			actionManager.SetActionValue("JumpOut", 0);
			actionManager.SetActionValue("CharacterStand", 0);
			actionManager.SetActionValue("CharacterCrouch", 0);
			actionManager.SetActionValue("CharacterProne", 0);
			actionManager.SetActionValue("CharacterStandCrouchToggle", 0);
			actionManager.SetActionValue("CharacterStandProneToggle", 0);
			actionManager.SetActionValue("CharacterRoll", 0);
			actionManager.SetActionValue("CharacterJump", 0);
			
			DisableVehicleMove(actionManager);
			
			if (character)
			{
				Vehicle vehicle = Vehicle.Cast(character.GetRootParent());
				if (vehicle)
				{
					BaseVehicleNodeComponent vehicleNodeComponent = BaseVehicleNodeComponent.Cast(vehicle.FindComponent(BaseVehicleNodeComponent));
					if (vehicleNodeComponent)
					{
						SCR_HelicopterControllerComponent helicopterControllerComponent = SCR_HelicopterControllerComponent.Cast(vehicleNodeComponent.FindComponent(SCR_HelicopterControllerComponent));
						if (!helicopterControllerComponent.GetAutohoverEnabled())
						{
							actionManager.SetActionValue("AutohoverToggle", 1);
						}
					}
				}
			}
		}
	}
	
	void DisableVehicleMove(ActionManager actionManager)
	{
		actionManager.SetActionValue("VehicleEngineStop", 1);
		actionManager.SetActionValue("VehicleEngineStart", 0);
		actionManager.SetActionValue("AutohoverToggle", 0);
		actionManager.SetActionValue("WheelBrake", 0);
		actionManager.SetActionValue("WheelBrakePersistent", 1);
		actionManager.SetActionValue("CyclicForward", 0);
		actionManager.SetActionValue("CyclicBack", 0);
		actionManager.SetActionValue("CyclicLeft", 0);
		actionManager.SetActionValue("CyclicRight", 0);
		actionManager.SetActionValue("AntiTorqueLeft", 0);
		actionManager.SetActionValue("AntiTorqueRight", 0);
		actionManager.SetActionValue("CollectiveIncrease", 0);
		actionManager.SetActionValue("CollectiveDecrease", 0);
		actionManager.SetActionValue("HelicopterEngineStop", 0);
		actionManager.SetActionValue("HelicopterEngineStart", 0);
		
		actionManager.SetActionValue("CarThrust", 0);
		actionManager.SetActionValue("CarBrake", 0);
		actionManager.SetActionValue("CarSteering", 0);
		actionManager.SetActionValue("CarTurbo", 0);
		actionManager.SetActionValue("CarTurboToggle", 0);
		actionManager.SetActionValue("CarShift", 0);
		actionManager.SetActionValue("CarShiftReverse", 0);
		actionManager.SetActionValue("CarHandBrake", 1);
		actionManager.SetActionValue("CarHandBrakePersistent", 0);
		actionManager.SetActionValue("CarLightsHiBeamToggle", 0);
		actionManager.SetActionValue("CarHazardLights", 0);
	}

	// Yes every frame, just don't look at it.
	override protected void EOnPostFixedFrame(IEntity owner, float timeSlice)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl.IsOwner())
			return;
		
		// Lets fight with phisyc engine
		if (m_InitialEntity)
		{
			PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
			int playerId = thisPlayerController.GetPlayerId();
			m_vVoNPosition = Vector(0, 100000, 0) + Vector(1000 * Math.Mod(playerId, 10), 5000 * Math.Floor(Math.Mod(playerId, 100) / 10), 5000 * Math.Floor(playerId / 100));
			vector currentOrigin = m_InitialEntity.GetOrigin();
			//if (currentOrigin == m_vVoNPosition) return;
			//Print("Move to: " + m_vVoNPosition.ToString());
			
			GameEntity gameEntity = GameEntity.Cast(m_InitialEntity);
			vector mat[4];
			Math3D.MatrixIdentity4(mat);
			mat[3] = m_vVoNPosition;
			gameEntity.Teleport(mat);
			
			MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
			if (menu && (menu.IsInherited(PS_PreviewMapMenu) || menu.IsInherited(PS_CoopLobby) || menu.IsInherited(PS_BriefingMapMenu)))
			{
				GetGame().GetCameraManager().CurrentCamera().SetWorldTransform(mat);
			}

			// Who broke camera on map?
			CameraBase cameraBase = GetGame().GetCameraManager().CurrentCamera();
			if (cameraBase)
				cameraBase.ApplyTransform(timeSlice);

			Physics physics = m_InitialEntity.GetPhysics();
			if (physics)
			{
				//physics.SetVelocity("0 0 0");
				//physics.SetAngularVelocity("0 0 0");
				//physics.SetMass(0);
				//physics.SetDamping(1, 1);
				//physics.ChangeSimulationState(SimulationState.NONE);
				physics.SetActive(ActiveState.INACTIVE);
			}
		} else {
			PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
			IEntity entity = thisPlayerController.GetControlledEntity();
			if (entity)
			{
				PS_LobbyVoNComponent von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
				if (von)
					m_InitialEntity = entity;
			}
		}
	}

	// Save VoN boi for reuse
	IEntity GetInitialEntity()
	{
		return m_InitialEntity;
	}
	void SetInitialEntity(IEntity initialEntity)
	{
		m_InitialEntity = initialEntity;
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

		if (thisPlayerController.GetPlayerId() != playerId && playerRole == EPlayerRole.NONE)
			return;
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE)
			return;

		playableManager.SetPlayerFactionKey(playerId, factionKey);
	}

	// ------------------ VoN controlls ------------------
	void MoveToVoNRoomByKey(int playerId, string roomKey)
	{
		string factionKey = "";
		string roomName = "#PS-VoNRoom_Global";

		if (roomKey.Contains("|")) {
			array<string> outTokens = {};
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
	RadioTransceiver GetVoNTransiver(int radioId)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast(entity.FindComponent(SCR_GadgetManagerComponent));
		array<SCR_GadgetComponent> radios = gadgetManager.GetGadgetsByType(EGadgetType.RADIO);
		IEntity radioEntity = radios[radioId].GetOwner();
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		transiver.SetFrequency(radioId + 1);
		return transiver;
	}
	void LobbyVoNEnable()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(GetVoNTransiver(1));
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	void LobbyVoNRadioEnable()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		PS_LobbyVoNComponent von = GetVoN();
		von.SetTransmitRadio(GetVoNTransiver(0));
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
		if (!von)
			return;
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	// Separate radio VoNs, CALL IT FROM SERVER
	void SetVoNKey(string VoNKey, string VoNKeyLocal)
	{
		if (!GetVoN())
			return;
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		if (!entity)
			return;
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast(entity.FindComponent(SCR_GadgetManagerComponent));
		array<SCR_GadgetComponent> radios = gadgetManager.GetGadgetsByType(EGadgetType.RADIO);
		if (radios.Count() > 0)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(radios[0].GetOwner().FindComponent(BaseRadioComponent));
			radio.SetEncryptionKey(VoNKey);
			radio = BaseRadioComponent.Cast(radios[1].GetOwner().FindComponent(BaseRadioComponent));
			radio.SetEncryptionKey(VoNKeyLocal);
		}
	}
	bool isVonInit()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast(entity.FindComponent(SCR_GadgetManagerComponent));
		IEntity radioEntity = gadgetManager.GetGadgetByType(EGadgetType.RADIO);
		return radioEntity;
	}
	
	void GetArmaIdFromServer(int playerId)
	{
		Rpc(RPC_GetArmaIdFromServer_Server, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_GetArmaIdFromServer_Server(int playerId)
	{
		string playerUUID = GetGame().GetBackendApi().GetPlayerUID(playerId);
		Rpc(RPC_GetArmaIdFromServer_Owner, playerUUID);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_GetArmaIdFromServer_Owner(string playerUUID)
	{
		System.ExportToClipboard(playerUUID);
	}

	// ------------------ Observer camera controlls ------------------
	void SaveCameraTransform()
	{
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_CameraEditorComponent, false));
		cameraManager.GetLastCameraTransform(lastCameraTransform);
	}

	void SwitchToObserver(IEntity from)
	{
		SCR_EditorManagerEntity editorManagerEntity = SCR_EditorManagerEntity.GetInstance();
		if (editorManagerEntity.IsOpened())
			return;
		
		if (m_Camera)
			return;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.SpectatorMenu);
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		EntitySpawnParams params = new EntitySpawnParams();
		if (from)
			from.GetTransform(params.Transform);
		MoveToVoNRoom(thisPlayerController.GetPlayerId(), "", "");
		Resource resource = Resource.Load("{6EAA30EF620F4A2E}Prefabs/Editor/Camera/ManualCameraSpectator.et");
		m_Camera = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);

		if (lastCameraTransform[3][1] < 10000 && lastCameraTransform[3][1] > 0)
		{
			m_Camera.SetTransform(lastCameraTransform);
			lastCameraTransform[3][1] = 10000;
		} else if (m_vObserverPosition != "0 0 0") {
			m_Camera.SetOrigin(m_vObserverPosition);
			m_vObserverPosition = "0 0 0";
		} else {
			SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
			m_Camera.SetOrigin(mapEntity.Size() / 2.0 + vector.Up * 100);
		}
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_Camera));

		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.GetFriendliesSpectatorOnly())
			PS_ManualCameraSpectator.Cast(m_Camera).SetCharacterEntity(from);
	}

	void SwitchFromObserver()
	{
		if (!m_Camera)
			return;
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.SpectatorMenu);
		SCR_EntityHelper.DeleteEntityAndChildren(m_Camera);
		m_Camera = null;
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
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

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
		if (playableManager.GetPlayableByPlayer(thisPlayerController.GetPlayerId()) == RplId.Invalid())
			SwitchToObserver(null);
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
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPin(playerId, false);
	}

	void PinPlayer(int playerId)
	{
		Rpc(RPC_PinPlayer, playerId)
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_PinPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPin(playerId, true);
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
		if (playerRole == EPlayerRole.NONE)
			return;

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
		if (thisPlayerController.GetPlayerId() != playerId && playerRole == EPlayerRole.NONE)
			return;

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
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE)
			return;

		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PS_PlayableContainer playableContainer = playableManager.GetPlayableById(playableId);
		if (playableContainer)
		{
			FactionKey factionKey = playableContainer.GetFactionKey();
			if (playerId >= 0 && !SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()) && !gameModeCoop.CanJoinFaction(factionKey, playableManager.GetPlayerFactionKey(playerId)))
				return;
		}

		playableManager.SetPlayablePlayer(playableId, playerId);
	}

	void SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		Rpc(RPC_SetPlayableVehicleLocked, vehicleId, lock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SetPlayableVehicleLocked(RplId vehicleId, bool lock)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
			return;
		
		playableManager.SetPlayableVehicleLocked(vehicleId, lock);
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
		if (playableManager.GetPlayerPin(playerId) && playerRole == EPlayerRole.NONE)
			return;

		// don't check other staff if empty playable
		if (playableId == RplId.Invalid()) {
			if (playerId != thisPlayerController.GetPlayerId())
				playableManager.NotifyKick(playerId);
			playableManager.SetPlayerPlayable(playerId, playableId);
			return;
		}

		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PS_PlayableContainer playableContainer = playableManager.GetPlayableById(playableId);
		if (playableContainer)
		{
			FactionKey factionKey = playableContainer.GetFactionKey();
			if (playerId >= 0 && !SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()) && !gameModeCoop.CanJoinFaction(factionKey, playableManager.GetPlayerFactionKey(playerId)))
				return;
		}

		SCR_ChimeraCharacter playableCharacter = SCR_ChimeraCharacter.Cast(playableContainer.GetPlayableComponent().GetOwner());

		// Check is playable already selected or dead
		int curretPlayerId = playableManager.GetPlayerByPlayable(playableId);
		if (playableCharacter.GetDamageManager().IsDestroyed() || (curretPlayerId != -1 && curretPlayerId != playerId)) {
			return;
		}

		playableManager.SetPlayerPlayable(playerId, playableId);

		// Pin player if setted by admin
		if (playerId != thisPlayerController.GetPlayerId())
			playableManager.SetPlayerPin(playerId, true);
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
