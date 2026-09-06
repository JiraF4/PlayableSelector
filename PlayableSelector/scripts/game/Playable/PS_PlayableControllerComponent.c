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
	protected PS_GameModeCoop m_GameModeCoop; // lazy-cached world singleton (stable for this component's life)

	// Diagnostic: one-shot flags to avoid spamming [PS_SpecDiag] logs on the 500ms watchdog tick.
	protected bool m_bSpecDiagSuppressLogged;		// suppress widget detail logged once, cleared on teardown
	protected string m_sSpecDiagLastWatchdogSkip;	// last watchdog skip reason, only log on change

	// Lazy-cached game mode. It is a world singleton created once per mission and never changes, so this avoids
	// the repeated PS_GameModeCoop.Cast(GetGame().GetGameMode()) - notably in the per-frame EOnFrame freeze
	// blocker. Reconnect-safe: this component is recreated per connection (fresh cache); lazy set-on-first-non-null
	// avoids caching null if component init races game-mode creation.
	PS_GameModeCoop GetGameModeCoop()
	{
		if (!m_GameModeCoop)
			m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		return m_GameModeCoop;
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

	// ------ GroupReady (squad ready during freeze time) ------
	void SetGroupReady(int groupId, int readyValue)
	{
		Rpc(RPC_SetGroupReady, groupId, readyValue);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SetGroupReady(int groupId, int readyValue)
	{
		// Only group leaders may vote
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!thisPlayerController)
			return;
		if (!playableManager.IsPlayerGroupLeader(thisPlayerController.GetPlayerId()))
			return;

		// Only during freeze time
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameMode || gameMode.GetState() != SCR_EGameModeState.GAME || gameMode.IsFreezeTimeEnd())
			return;

		playableManager.SetGroupReady(groupId, readyValue);
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

		// Body-less: tear down the spectator camera/menu on any state change (the GAME case re-opens it
		// below via ApplyPlayable when the player has no slot). No-op when not spectating.
		SwitchFromObserver();

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

		// Body-less voice: re-evaluate the menu talking device on every menu/state change.
		PS_MenuVoN.Refresh();
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
		// SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
		// It's litteraly broken on dedicated.
		// saveManager.RestartAndLoad(missionName);
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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=FactionLockSwitch",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()));
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=SpawnPrefab guid='%2' pos=(%3,%4,%5)",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				GUID, position[0], position[1], position[2]);
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=SpawnAdministrator pos=(%2,%3,%4)",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				position[0], position[1], position[2]);
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=RespawnPlayable playableId=%2 useInitPos=%3",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				playableId, useInitPosition);
			return;
		}
		
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
		GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableComponent.GetRplId(), newCharacter, playableContainer);
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

		Rpc(RPC_ForceRespawnPlayer, Replication.FindItemId(character), initPosition);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ForceRespawnPlayer(RplId respawnEntityRplId, bool initPosition)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!SCR_Global.IsAdmin(thisPlayerController.GetPlayerId()))
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=ForceRespawnPlayer entityRplId=%2 initPos=%3",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				respawnEntityRplId, initPosition);
			return;
		}

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
		GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableComponent.GetRplId(), newCharacter, playableContainer);
	}

	// oldPlayableId is the OLD playable's RplId, captured at schedule time. The old body (and its
	// PS_PlayableComponent, which lives ON that body) can be deleted in the 300ms before this fires, so we
	// must NOT depend on a live oldPlayableComponent reference here - the original did and threw a VME
	// (null 'character') from OnUpdate every frame. The player/group lookups below are RplId-keyed maps in
	// PS_PlayableManager that outlive the entity, so the re-home still completes correctly when the body
	// is already gone; we only skip the parts that genuinely need the (now-deleted) body.
	void RPC_ForceRespawnPlayerLate(SCR_ChimeraCharacter character, RplId oldPlayableId, SCR_ChimeraCharacter newCharacter, PS_PlayableComponent playableContainer)
	{
		// While the old body still exists, keep killing it until the engine reports it fully destroyed.
		// A deleted body (null) is already "destroyed", so fall through to the re-home in that case.
		if (character && character.GetDamageManager())
		{
			character.GetDamageManager().Kill(Instigator.CreateInstigator(newCharacter));
			character.GetDamageManager().SetHealthScaled(0);
			if (!character.GetDamageManager().IsDestroyed())
			{
				GetGame().GetCallqueue().CallLater(RPC_ForceRespawnPlayerLate, 300, false, character, oldPlayableId, newCharacter, playableContainer);
				return;
			}
		}

		if (!playableContainer)
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		SCR_AIGroup aiGroup = playableManager.GetPlayerGroupByPlayable(oldPlayableId);
		if (!aiGroup)
			return;
		SCR_AIGroup playabelGroup = aiGroup.GetSlave();
		// Only re-home the old body into the bot group if it still exists.
		if (playabelGroup && character)
			playabelGroup.AddAIEntityToGroup(character);
		playableManager.SetPlayablePlayerGroupId(playableContainer.GetRplId(), aiGroup.GetGroupID());
		int playerId = playableManager.GetPlayerByPlayableRemembered(oldPlayableId);
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
		/*
		EntitySpawnParams params = new EntitySpawnParams(); 
		Resource resource = Resource.Load("{6EAA30EF620F4A2E}Prefabs/Editor/Camera/ManualCameraSpectator.et");
		m_Camera = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		*/
		
		// (Removed the per-frame UpdatePosition CallLater - it parked the old controlled BODY every frame;
		// body-less there is no body, so it just burned a FindComponent per frame on the owner client doing
		// nothing. SetEventMask FRAME stays for EOnFrame, the freeze-time fire blocker.)
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

		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!rpl.IsOwner())
			return;

		// Body-less voice: control changes flip menu-speaker state (took a playable / died) -
		// re-evaluate the local menu talking device.
		PS_MenuVoN.Refresh();

		// Remember where control was lost - the spectator camera starts there.
		if (!to && from)
			m_vObserverPosition = from.GetOrigin();

		// Body-less: the old design keyed the observer transitions off a PS_LobbyVoNComponent on the
		// controlled body. There is no body now (vonTo would ALWAYS be null -> SwitchFromObserver fired on
		// every change, tearing the spectator down). Decide off the new entity's life state instead:
		// returning to a LIVING character means the player is back in the game (leave spectator + tell the
		// editor core we are alive so it releases the camera). Control going to NULL or to a DEAD corpse
		// must NOT tear the spectator down - that path is owned by SendPlayerToSpectator_S /
		// RPC_EnterSpectator, which also DELETES the corpse so control drops to null and frees VONDirect.
		bool toIsLivingCharacter = false;
		ChimeraCharacter toCharacter = ChimeraCharacter.Cast(to);
		if (toCharacter)
		{
			SCR_DamageManagerComponent toDmg = SCR_DamageManagerComponent.Cast(toCharacter.FindComponent(SCR_DamageManagerComponent));
			toIsLivingCharacter = !toDmg || toDmg.GetState() != EDamageState.DESTROYED;
		}

		// Diagnostic: log every control transition (death, respawn, reconnect).
		string fromDesc = "null";
		if (from) fromDesc = from.ClassName();
		string toDesc = "null";
		if (to) toDesc = to.ClassName();
		string toDmgStr = "nullEntity";
		if (toCharacter)
		{
			SCR_DamageManagerComponent toDmg2 = SCR_DamageManagerComponent.Cast(toCharacter.FindComponent(SCR_DamageManagerComponent));
			if (toDmg2) toDmgStr = toDmg2.GetState().ToString();
			else toDmgStr = "noDmg";
		}
		PrintFormat("[PS_SpecDiag] OnControlledEntityChanged: from=%1 to=%2 dmgState=%3 isLiving=%4 isMenuSpeaker=%5",
			fromDesc, toDesc, toDmgStr, toIsLivingCharacter,
			SCR_VoNComponent.PS_IsMenuSpeaker(thisPlayerController.GetPlayerId()));

		if (toIsLivingCharacter)
		{
			SwitchFromObserver();
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameModeCoop.GetState() == SCR_EGameModeState.GAME)
				GetGame().GetCallqueue().Call(TellFuckingEditorCoreThanWeAlive, thisPlayerController.GetPlayerId(), to);
		}
	}
	
	// There is sure no ебанорго game modes without spawns, yeah sure блять
	void TellFuckingEditorCoreThanWeAlive(int playerId, IEntity entity)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.IsFreezeTimeEnd() && gameModeCoop.GetDisableBuildingModeAfterFreezeTime())
			 return;
		SCR_BaseGameMode.Cast(GetGame().GetGameMode()).GetOnPlayerSpawned().Invoke(playerId, entity);
		Rpc(AndFuckingServerTo, playerId, Replication.FindItemId(entity))
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void AndFuckingServerTo(int playerId, RplId entityId)
	{
		IEntity entity = IEntity.Cast(Replication.FindItem(entityId));
		SCR_BaseGameMode.Cast(GetGame().GetGameMode()).GetOnPlayerSpawned().Invoke(playerId, entity);
	}
	
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		PS_GameModeCoop gameMode = GetGameModeCoop();
		if (!gameMode)
			return; // game mode not resolved yet (early frame) - retry next frame
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

	// EOnFixedFrame removed - FIXEDFRAME was never masked (the POSTFIXEDFRAME SetEventMask is commented out),
	// so it never fired, and it only called the now-unused body-parking UpdatePosition.
	void UpdatePosition(bool force)
	{
		// Repeating call may still fire while the player controller is being torn down on disconnect
		IEntity owner = GetOwner();
		if (!owner)
			return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl || !rpl.IsOwner())
			return;
		PlayerController ownerPlayerController = PlayerController.Cast(owner);
		if (!ownerPlayerController)
			return;

		// Lets fight with phisyc engine
		if (m_InitialEntity)
		{
			// While spectating, the server parks the body above the player's own corpse so the
			// battlefield stays streamed around it (NDS streams around the controlled entity).
			// The no-physics component keeps it there - don't drag it to the lobby grid.
			if (m_Camera)
			{
				// Who broke camera on map?
				CameraBase specCameraBase = GetGame().GetCameraManager().CurrentCamera();
				if (specCameraBase)
					specCameraBase.ApplyTransform(GetGame().GetWorld().GetTimeSlice());
			}
			else
			{
				// In menus (preview/lobby/briefing) pin the body to a deterministic per-player spot.
				// VoN "rooms" rely on each player's body being spatially separated so proximity voice
				// never bleeds between players; forcing the position guarantees uniqueness regardless
				// of replication timing.
				int playerId = ownerPlayerController.GetPlayerId();
				m_vVoNPosition = PS_GameModeCoop.GetInitialEntityPosition(playerId);
				vector currentOrigin = m_InitialEntity.GetOrigin();

				vector mat[4];
				Math3D.MatrixIdentity4(mat);
				mat[3] = m_vVoNPosition;

				// Touch the transform only when it actually drifted: this entity is owned by the local
				// client, every SetTransform dirties its replication state, so per-frame would flood.
				if (force || vector.DistanceSq(currentOrigin, m_vVoNPosition) > 0.01)
				{
					GameEntity gameEntity = GameEntity.Cast(m_InitialEntity);
					if (force)
						gameEntity.Teleport(mat);
					gameEntity.SetTransform(mat);

					Physics physics = m_InitialEntity.GetPhysics();
					if (physics)
						physics.SetActive(ActiveState.INACTIVE);
				}

				MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
				if (menu && (menu.IsInherited(PS_PreviewMapMenu) || menu.IsInherited(PS_CoopLobby) || menu.IsInherited(PS_BriefingMapMenu)))
				{
					GetGame().GetCameraManager().CurrentCamera().SetWorldTransform(mat);
				}

				// Who broke camera on map?
				CameraBase cameraBase = GetGame().GetCameraManager().CurrentCamera();
				if (cameraBase)
					cameraBase.ApplyTransform(GetGame().GetWorld().GetTimeSlice());
			}
		} else {
			IEntity entity = ownerPlayerController.GetControlledEntity();
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

	// Body-less: dead path (lobby voice is on the VoN proxy now, see PS_MenuVoN). Kept for the
	// in-game character VoN callers; guarded so a body-less (null) controlled entity never derefs.
	PS_LobbyVoNComponent GetVoN()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!thisPlayerController)
			return null;
		IEntity entity = thisPlayerController.GetControlledEntity();
		if (!entity)
			return null;
		return PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
	}
	// Collect the lobby VoN radios from the controlled entity, in order.
	// Works whether the radios are carried as inventory gadgets (full character carrier)
	// or attached as direct child entities (stripped carrier without the inventory system).
	// The carrier prefab must keep the two radios in a stable order (radio 0 first, radio 1 second).
	protected void GetVoNRadios(out array<BaseRadioComponent> radios)
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		if (!thisPlayerController)
			return;
		IEntity entity = thisPlayerController.GetControlledEntity();
		if (!entity)
			return;

		// Inventory gadget path (radios carried as items)
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.Cast(entity.FindComponent(SCR_GadgetManagerComponent));
		if (gadgetManager)
		{
			array<SCR_GadgetComponent> gadgets = gadgetManager.GetGadgetsByType(EGadgetType.RADIO);
			foreach (SCR_GadgetComponent gadget : gadgets)
			{
				if (!gadget)
					continue;
				BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
				if (radio)
					radios.Insert(radio);
			}
			if (radios.Count() >= 2)
				return;
			radios.Clear();
		}

		// Direct child entity path (radios attached without an inventory system)
		IEntity child = entity.GetChildren();
		while (child)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(child.FindComponent(BaseRadioComponent));
			if (radio)
				radios.Insert(radio);
			child = child.GetSibling();
		}
	}

	RadioTransceiver GetVoNTransiver(int radioId)
	{
		array<BaseRadioComponent> radios = {};
		GetVoNRadios(radios);
		if (radioId < 0 || radioId >= radios.Count())
			return null;
		BaseRadioComponent radio = radios[radioId];
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		transiver.SetFrequency(radioId + 1);
		return transiver;
	}
	// Body-less: lobby push-to-talk is now owned by PS_MenuVoN (it binds VONDirect while the local
	// player is a menu speaker). These three remain only because some menus still bind them as input
	// actions; they are deliberate no-ops now (there is no controlled body VoN to drive).
	void LobbyVoNEnable()
	{
	}
	void LobbyVoNRadioEnable()
	{
	}
	void LobbyVoNDisable()
	{
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
		array<BaseRadioComponent> radios = {};
		GetVoNRadios(radios);
		if (radios.Count() >= 2)
		{
			radios[0].SetEncryptionKey(VoNKey);
			radios[1].SetEncryptionKey(VoNKeyLocal);
		}
	}
	bool isVonInit()
	{
		array<BaseRadioComponent> radios = {};
		GetVoNRadios(radios);
		return radios.Count() >= 2;
	}

	// Spectator voice fix (issue 3): a dead player keeps CONTROLLING their corpse, whose carried radios
	// stay powered on the in-game faction net - so the spectator still HEARS living teammates' radio
	// chatter. Menu/spectator voice runs on the separate VoN proxy entity, so powering the corpse's
	// radios down kills only that unwanted in-game reception. Server-authoritative: the off state
	// replicates to the owner, so their client stops decoding the faction net. No respawns in this mode,
	// so there is nothing to restore.
	void DisableBodyVoNRadios()
	{
		array<BaseRadioComponent> radios = {};
		GetVoNRadios(radios);
		foreach (BaseRadioComponent radio : radios)
		{
			if (radio)
				radio.SetPower(false);
		}
	}
	
	void GetArmaIdFromServer(int playerId)
	{
		Rpc(RPC_GetArmaIdFromServer_Server, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_GetArmaIdFromServer_Server(int playerId)
	{
		string playerUUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		Rpc(RPC_GetArmaIdFromServer_Owner, playerUUID);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_GetArmaIdFromServer_Owner(string playerUUID)
	{
		System.ExportToClipboard(playerUUID);
	}

	protected ref ScriptInvokerString m_eOnPlayerGuidReceived = new ScriptInvokerString();
	ScriptInvokerString GetOnPlayerGuidReceived()
	{
		return m_eOnPlayerGuidReceived;
	}

	void RequestPlayerGuid(int playerId)
	{
		Rpc(RPC_RequestPlayerGuid_Server, playerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_RequestPlayerGuid_Server(int playerId)
	{
		string playerGuid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		Rpc(RPC_RequestPlayerGuid_Owner, playerGuid);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_RequestPlayerGuid_Owner(string playerGuid)
	{
		m_eOnPlayerGuidReceived.Invoke(playerGuid);
	}

	// ------------------ Observer camera controlls ------------------
	void SaveCameraTransform()
	{
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_CameraEditorComponent, false));
		// The camera editor component isn't always present when the editor closes (EditorClosed path);
		// guard so we don't VME trying to read the last transform off a null manager.
		if (!cameraManager)
			return;
		cameraManager.GetLastCameraTransform(lastCameraTransform);
	}

	// ------------------ Spectator streaming observer: REMOVED ------------------
	// An MPObserver (RplComponent.InsertMPObserver) used to follow the spectator camera and stream the
	// battlefield around the view. It was the ONLY spectator-streaming mechanism among the reference
	// lobbies (Echo and LiteLobby use none) and the main remaining "Replication Flooded/Stalled" lever, so
	// it is removed entirely. Spectators now see only what default NDS streams around their parked corpse,
	// exactly like Echo/LiteLobby. The GetSpectatorStreamingObserver() gamemode flag is now inert.

	// ------------------ Spectate a player outside this client's replication pool ------------------
	// With default NDS culling (force-streaming disabled) a distant playable is not replicated here, so
	// PS_SpectatorMenu.SetCameraCharacter cannot resolve a local entity to follow. Instead ask the server
	// for that playable's world position, fly the free spectator camera there, and (when the streaming
	// observer is enabled) push the observer to that spot so the area - and the player - streams in. Once
	// the player is streamed, clicking them again takes the normal first-person follow path.
	void RequestSpectatePosition(RplId playableId)
	{
		if (playableId == RplId.Invalid() || !m_Camera)
			return;
		Rpc(RPC_RequestSpectatePosition, playableId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_RequestSpectatePosition(RplId playableId)
	{
		// Server has every entity - resolve the playable and read its current position + forward.
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(playableId));
		if (!rpl)
			return;
		IEntity entity = rpl.GetEntity();
		if (!entity)
			return;
		vector pos = entity.GetOrigin();
		vector transform[4];
		entity.GetTransform(transform);
		vector fwd = transform[2]; // forward direction of the target
		if (GetGame().GetPlayerController() == GetOwner())
			RPC_ReceiveSpectatePosition(playableId, pos, fwd);
		else
			Rpc(RPC_ReceiveSpectatePosition, playableId, pos, fwd);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_ReceiveSpectatePosition(RplId playableId, vector pos, vector fwd)
	{
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(m_Camera);
		if (!camera)
			return; // no longer spectating

		// A previous AttachTo would fight the teleport - drop it first.
		if (PS_AttachManualCameraObserverComponent.s_Instance && PS_AttachManualCameraObserverComponent.s_Instance.GetTarget())
			PS_AttachManualCameraObserverComponent.s_Instance.Detach();

		// Place camera 5m behind the player looking at them (instead of at their exact position).
		camera.SetCameraBehindPosition(pos, fwd);
	}

	void SwitchToObserver(IEntity from)
	{
		SCR_EditorManagerEntity editorManagerEntity = SCR_EditorManagerEntity.GetInstance();
		if (editorManagerEntity.IsOpened())
		{
			PrintFormat("[PS_SpecDiag] SwitchToObserver: BLOCKED - editor is open");
			return;
		}
		
		if (m_Camera)
		{
			PrintFormat("[PS_SpecDiag] SwitchToObserver: BLOCKED - camera already exists");
			return;
		}
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.SpectatorMenu);
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		IEntity entity = thisPlayerController.GetControlledEntity();
		EntitySpawnParams params = new EntitySpawnParams();
		if (from)
			from.GetTransform(params.Transform);
		// Spectators talk on the global VoN room (matches SendPlayerToSpectator_S and RoomSwitchToGlobal).
		// This was "", "" (the empty-room channel), which briefly routed the spectator to a different
		// channel than everyone else's global room.
		MoveToVoNRoom(thisPlayerController.GetPlayerId(), "", "#PS-VoNRoom_Global");
		Resource resource = Resource.Load("{6EAA30EF620F4A2E}Prefabs/Editor/Camera/ManualCameraSpectator.et");
		m_Camera = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);

		// Priority: corpse position (death) > saved editor position > last observer position > map center.
		if (from)
		{
			// Death / spectator-with-entity: place camera 5m behind the corpse, looking at it.
			vector corpseTransform[4];
			from.GetTransform(corpseTransform);
			PS_ManualCameraSpectator.Cast(m_Camera).SetCameraBehindPosition(corpseTransform[3], corpseTransform[2]);
		}
		else if (lastCameraTransform[3][1] < 10000 && lastCameraTransform[3][1] > 0)
		{
			m_Camera.SetTransform(lastCameraTransform);
			lastCameraTransform[3][1] = 10000;
		}
		else if (m_vObserverPosition != "0 0 0")
		{
			m_Camera.SetOrigin(m_vObserverPosition);
			m_vObserverPosition = "0 0 0";
		}
		else
		{
			SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
			vector mapCenter = mapEntity.Size() / 2.0;
			// Use terrain height so the camera doesn't spawn underground on elevated maps.
			BaseWorld world = GetGame().GetWorld();
			float surfaceY = 100;
			if (world)
				surfaceY = world.GetSurfaceY(mapCenter[0], mapCenter[2]) + 100;
			m_Camera.SetOrigin(Vector(mapCenter[0], surfaceY, mapCenter[2]));
		}
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_Camera));

		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());

		// Diagnostic: log the camera position chosen and game state
		vector camPos = m_Camera.GetOrigin();
		PlayerController ownerPc = PlayerController.Cast(GetOwner());
		int ownerPid = 0;
		if (ownerPc) ownerPid = ownerPc.GetPlayerId();
		string source = "unknown";
		if (from) source = "corpse";
		else if (lastCameraTransform[3][1] >= 10000 || lastCameraTransform[3][1] <= 0) source = "mapCenter";
		else source = "lastEditor";
		string stateStr = "null";
		if (gameMode) stateStr = gameMode.GetState().ToString();
		PrintFormat("[PS_SpecDiag] SwitchToObserver: cameraCreated player=%1 from=%2 pos=(%3,%4,%5) gameState=%6 isMenuSpeaker=%7",
			ownerPid, source, camPos[0], camPos[1], camPos[2],
			stateStr,
			SCR_VoNComponent.PS_IsMenuSpeaker(ownerPid));

		if (gameMode.GetFriendliesSpectatorOnly())
			PS_ManualCameraSpectator.Cast(m_Camera).SetCharacterEntityMove(from);

		// Suppress third-party screen effects (e.g. LMSuppression blur/vignette) on the spectator
		// camera. The player controls their dead corpse, so these effects persist from the living state.
		SuppressSpectatorScreenEffects();

		// Body-less: keep the spectator camera from being stolen by the corpse death-cam / editor / map.
		StartSpectatorCameraWatchdog();
	}

	void SwitchFromObserver()
	{
		if (!m_Camera)
			return;
		PlayerController pc = PlayerController.Cast(GetOwner());
		int pid = 0;
		if (pc) pid = pc.GetPlayerId();
		PrintFormat("[PS_SpecDiag] SwitchFromObserver: tearing down spectator player=%1 isMenuSpeaker=%2",
			pid, SCR_VoNComponent.PS_IsMenuSpeaker(pid));
		m_bSpecDiagSuppressLogged = false;
		m_sSpecDiagLastWatchdogSkip = "";
		GetGame().GetCallqueue().Remove(EnforceSpectatorCamera);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.SpectatorMenu);
		RestoreSpectatorScreenEffects();
		SCR_EntityHelper.DeleteEntityAndChildren(m_Camera);
		m_Camera = null;
	}

	// Body-less spectator camera watchdog: the player keeps their dead CORPSE as the controlled entity,
	// whose death-cam (and the editor/world/preview cameras) try to grab the view. While spectating,
	// re-assert the free spectator camera as the active one. Ported from LiteLobby (EnforceSpectatorCamera).
	protected void StartSpectatorCameraWatchdog()
	{
		GetGame().GetCallqueue().Remove(EnforceSpectatorCamera);
		GetGame().GetCallqueue().CallLater(EnforceSpectatorCamera, 500, true);
		GetGame().GetCallqueue().CallLater(EnforceSpectatorCamera, 0, false);
	}
	protected	void EnforceSpectatorCamera()
	{
		if (!m_Camera)
		{
			GetGame().GetCallqueue().Remove(EnforceSpectatorCamera);
			return;
		}
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		MenuBase topMenu = menuManager.GetTopMenu();
		// No menu at all (a stage preview closed over us): the spectator menu IS this player's GAME
		// view - restore it, then re-take the camera below.
		if (!topMenu && !menuManager.IsAnyDialogOpen())
		{
			if (!menuManager.FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu))
			{
				PrintFormat("[PS_SpecDiag] Watchdog: no top menu, SpectatorMenu missing - reopening");
				menuManager.OpenMenu(ChimeraMenuPreset.SpectatorMenu);
			}
		}
		// A fullscreen menu other than the spectator or fade-to-game screen is on top (lobby/briefing/map
		// opened over us): ownership is irrelevant while it covers the screen - decide again next tick.
		// FadeToGame is excluded: on reconnect SwitchToMenu(GAME) opens it on top of SpectatorMenu, and
		// if we skip suppression during the 1s fade, vanilla's death/bleeding effects re-assert and the
		// player sees a persistent black screen.
		else if (topMenu && !topMenu.IsInherited(PS_SpectatorMenu) && !topMenu.IsInherited(PS_FadeToGame))
		{
			string skipReason = topMenu.ClassName();
			if (m_sSpecDiagLastWatchdogSkip != skipReason)
			{
				m_sSpecDiagLastWatchdogSkip = skipReason;
				PrintFormat("[PS_SpecDiag] Watchdog: SKIP - topMenu=%1 blocks spectator (neither SpectatorMenu nor FadeToGame)", skipReason);
			}
			return;
		}

		// An opened editor grabs the camera every frame - close it (reopening GM stays one key away).
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && editorManager.IsOpened())
		{
			if (editorManager.IsInTransition())
				return;
			PrintFormat("[PS_SpecDiag] Watchdog: closing editor to reclaim camera");
			editorManager.Close(false);
			return;
		}

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;
		CameraBase currentCam = cameraManager.CurrentCamera();
		if (currentCam != m_Camera)
		{
			string camName = "null";
			if (currentCam) camName = currentCam.ClassName();
			PrintFormat("[PS_SpecDiag] Watchdog: camera stolen by %1 - reclaiming", camName);
			cameraManager.SetCamera(CameraBase.Cast(m_Camera));
		}

		// Keep third-party screen effects suppressed while the spectator camera is active.
		// Effects like LMSuppression can re-register on camera changes; this catches them.
		SuppressSpectatorScreenEffects();

		// Keep the corpse's carried radios powered off. The vanilla VoN system
		// (SCR_VONEntryRadio) periodically re-powers radios via SetPower(IsUsable()),
		// so a one-shot disable is not enough — the spectator would re-hear in-game
		// radio chatter from alive teammates on the same faction net.
		DisableBodyVoNRadios();
	}

	// ---- Screen effect suppression for spectator camera ----
	// When spectating, the player controls their dead corpse. Both vanilla and third-party screen
	// effects persist because the corpse's damage state (burning, bleeding, dead) is still active.
	// We clear ALL known post-process effect priorities and hide ALL known screen effect HUD widgets.
	// All lookups are null-checked, so this is a no-op for effects that aren't present.
	//
	// Vanilla post-process priorities (from Arma Reforger API 1.7.0.54):
	//   5 = Colors (SCR_DesaturationEffect — blood loss desaturation)
	//   6 = RadialBlur (SCR_StaminaBlurEffect — stamina blur)
	//   7 = GaussFilter (SCR_DamageBlurEffect — damage blur)
	//   9 = ChromAber (SCR_RegenerationScreenEffect — chromatic aberration)
	// LMSuppression post-process priorities:
	//   18 = RadialBlur (LM_SuppressionScreenEffect)
	//   19 = Colors (LM_SuppressionScreenEffect)
	protected void SuppressSpectatorScreenEffects()
	{
		// Clear ALL known camera post-process effects via the world-level API
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			int camId = world.GetCurrentCameraId();
			// Vanilla effects
			world.SetCameraPostProcessEffect(camId, 5, PostProcessEffectType.None, "");	// Desaturation
			world.SetCameraPostProcessEffect(camId, 6, PostProcessEffectType.None, "");	// Stamina blur
			world.SetCameraPostProcessEffect(camId, 7, PostProcessEffectType.None, "");	// Damage blur
			world.SetCameraPostProcessEffect(camId, 9, PostProcessEffectType.None, "");	// Chromatic aberration
			// LMSuppression effects
			world.SetCameraPostProcessEffect(camId, 18, PostProcessEffectType.None, "");	// Suppression radial blur
			world.SetCameraPostProcessEffect(camId, 19, PostProcessEffectType.None, "");	// Suppression color
			if (!m_bSpecDiagSuppressLogged)
				PrintFormat("[PS_SpecDiag] Suppress: cleared post-process camId=%1 (prio 5,6,7,9,18,19)", camId);
		}

		// Hide ALL known screen effect HUD widgets
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		// Diag mode on first call only — logs widget visibility/opacity before hiding.
		// Subsequent watchdog ticks (every 500ms) call KillAndHideWidget silently.
		bool diag = !m_bSpecDiagSuppressLogged;

		// Vanilla widgets — StopAllAnimations first so AnimateWidget (which runs every frame)
		// can't re-drive opacity after we hide. SCR_DeathScreenEffect and SCR_BleedingScreenEffect
		// mods prevent new animations from starting, but animations already in-flight must be killed.
		KillAndHideWidget(workspace, "DeathOverlay", diag);
		KillAndHideWidget(workspace, "DeathBlackOut", diag);
		KillAndHideWidget(workspace, "BloodVignette1", diag);
		KillAndHideWidget(workspace, "BloodVignette2", diag);
		KillAndHideWidget(workspace, "BleedingBlackOut", diag);
		KillAndHideWidget(workspace, "UnconOverlay", diag);
		KillAndHideWidget(workspace, "SuppressionVignette", diag);
		KillAndHideWidget(workspace, "DrowningVignette", diag);
		KillAndHideWidget(workspace, "DrowningBlackOut", diag);

		// LMSuppression widgets
		KillAndHideWidget(workspace, "LM_SuppressionVignette", diag);
		KillAndHideWidget(workspace, "LM_SuppressionFlinch", diag);

		if (diag)
			m_bSpecDiagSuppressLogged = true;
	}

	protected void RestoreSpectatorScreenEffects()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		PrintFormat("[PS_SpecDiag] Restore: showing all screen effect widgets");

		// Vanilla widgets
		ShowWidget(workspace, "DeathOverlay");
		ShowWidget(workspace, "DeathBlackOut");
		ShowWidget(workspace, "BloodVignette1");
		ShowWidget(workspace, "BloodVignette2");
		ShowWidget(workspace, "BleedingBlackOut");
		ShowWidget(workspace, "UnconOverlay");
		ShowWidget(workspace, "SuppressionVignette");
		ShowWidget(workspace, "DrowningVignette");
		ShowWidget(workspace, "DrowningBlackOut");

		// LMSuppression widgets
		ShowWidget(workspace, "LM_SuppressionVignette");
		ShowWidget(workspace, "LM_SuppressionFlinch");
	}

	protected void KillAndHideWidget(WorkspaceWidget workspace, string name, bool diag = false)
	{
		Widget w = workspace.FindAnyWidget(name);
		if (!w)
			return;
		if (diag)
		{
			bool wasVisible = w.IsVisible();
			float opacity = w.GetOpacity();
			AnimateWidget.StopAllAnimations(w);
			w.SetVisible(false);
			if (wasVisible || opacity > 0.01)
				PrintFormat("[PS_SpecDiag] Suppress widget: %1 wasVisible=%2 opacity=%3 -> hidden", name, wasVisible, opacity);
		}
		else
		{
			AnimateWidget.StopAllAnimations(w);
			w.SetVisible(false);
		}
	}

	protected void ShowWidget(WorkspaceWidget workspace, string name)
	{
		Widget w = workspace.FindAnyWidget(name);
		if (w)
			w.SetVisible(true);
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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=ForceGameStart",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()));
			return;
		}

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

	// Server: ask the owning client to enter the spectator camera/menu. Used on death - the player
	// keeps their corpse as the controlled entity, so no control change fires the client trigger.
	void EnterSpectatorOwner()
	{
		// Listen host: Rpc() never executes on the sending machine, so call directly there.
		if (GetGame().GetPlayerController() == GetOwner())
			RPC_EnterSpectator();
		else
			Rpc(RPC_EnterSpectator);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_EnterSpectator()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		IEntity corpse;
		if (pc)
			corpse = pc.GetControlledEntity();
		int pid = 0;
		if (pc) pid = pc.GetPlayerId();
		bool isMenuSpeaker = SCR_VoNComponent.PS_IsMenuSpeaker(pid);
		string corpseState = "null";
		if (corpse)
		{
			SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(corpse.FindComponent(SCR_DamageManagerComponent));
			string dmgStateStr = "noDmg";
			if (dmg) dmgStateStr = dmg.GetState().ToString();
			corpseState = "entity=" + corpse.GetPrefabData().GetPrefabName() + " dmgState=" + dmgStateStr;
		}
		PrintFormat("[PS_SpecDiag] RPC_EnterSpectator: player=%1 isMenuSpeaker=%2 corpse=(%3)", pid, isMenuSpeaker, corpseState);

		// Dead -> menu speaker: refresh the menu talking device (death does not change the controlled
		// entity, so OnControlledEntityChanged would not fire this).
		PS_MenuVoN.Refresh();

		// Open the spectator camera/menu, starting at the corpse if we still control it.
		SwitchToObserver(corpse);

		// The corpse's dead life-state and the released playable slot replicate on a path that is NOT
		// ordered with this RPC, so PS_IsMenuSpeaker can still read FALSE for a frame or two right here.
		// Refresh() would then deactivate the menu device and never re-trigger (control does not change
		// while spectating) - the spectator could neither speak nor hear. Re-run it after the state has
		// settled so it activates reliably.
		GetGame().GetCallqueue().Remove(RefreshMenuVoNRetry);
		GetGame().GetCallqueue().CallLater(RefreshMenuVoNRetry, 300, false);
		GetGame().GetCallqueue().CallLater(RefreshMenuVoNRetry, 1200, false);
	}
	protected void RefreshMenuVoNRetry()
	{
		PS_MenuVoN.Refresh();
	}

	// Get controll on selected playable entity
	void ApplyPlayable()
	{
		PlayerController thisPlayerController = PlayerController.Cast(GetOwner());
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		int pid = thisPlayerController.GetPlayerId();
		RplId assignedPlayable = playableManager.GetPlayableByPlayer(pid);
		bool hasSlot = assignedPlayable != RplId.Invalid();
		PrintFormat("[PS_SpecDiag] ApplyPlayable: player=%1 hasSlot=%2 playableId=%3 isMenuSpeaker=%4",
			pid, hasSlot, assignedPlayable, SCR_VoNComponent.PS_IsMenuSpeaker(pid));
		if (!hasSlot)
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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=UnpinPlayer target=%2",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				playerId);
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=PinPlayer target=%2",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				playerId);
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=KickPlayer target=%2",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				playerId);
			return;
		}

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
		{
			PrintFormat("[PS_AntiCheat] ADMIN_ATTEMPT: %1 action=SetPlayableVehicleLocked vehicleId=%2 lock=%3",
				PS_GameModeCoop.PS_AntiCheatPlayerIdentity(thisPlayerController.GetPlayerId()),
				vehicleId, lock);
			return;
		}
		
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
			// FIX (STRAND): re-route voice to Global when a slot is released (deselect / kick).
			// Without this, the player stays on their old faction's VoN channel until the 5s
			// reconcile tick catches them. This moves them to Global immediately.
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameMode)
				gameMode.AssignPhaseVoiceChannel(playerId);
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
