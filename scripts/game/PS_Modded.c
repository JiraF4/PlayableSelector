// WHY THIS THING POOP IN MY LOGS
// I'M NOT INTERESTED IN FUCKING SLOTS
// THIS EVEN HAPPEN ON NORMAL GAMEMODE, STOP IT, IT'S AGAINST HUMANITY, GOD AND (What is more important) AGAINST ME.
modded class SCR_ChatHud
{
	override void OnChatOpen()
	{
		m_slotHandler.GetSlotUIComponent().SetPriority(m_iHUDPriorityDefault + 1);
		//Print(m_slotHandler);
	}
}

modded class SCR_HUDSlotUIComponent
{
	override void Initialize()
	{
		if (m_bInitialized)
			return;

		Widget child = m_wRoot.GetChildren();
		
		
		/*
		if (!child)
			Print(m_wRoot.GetName() + " Has no Content!", LogLevel.WARNING);
		*/
		
		if (!GetGroupComponent())
			Debug.Error2("No Group Component", "A Slot's parent must always have a Group Component! Slot: " + m_wRoot.GetName());

		m_wWorkspace = GetGame().GetWorkspace();
		m_bInitialized = true;
	}
}
// Sorry i trigger a little...

modded class SCR_ManualCamera
{
	override protected bool IsDisabledByMenu()
	{
		if (!m_MenuManager) return false;
		
		if (m_MenuManager.IsAnyDialogOpen()) return true;
		
		MenuBase topMenu = m_MenuManager.GetTopMenu();
		
		// WHY IT'S HARDCODED?
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(PS_SpectatorMenu));
	}
};

modded class SCR_PlayerControllerGroupComponent
{
	void PS_AskJoinGroup(int groupID)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return;
		
		int groupIDAfter;
		if (m_iGroupID != -1)
			groupIDAfter = groupsManager.MovePlayerToGroup(GetPlayerID(), m_iGroupID, groupID);
		else
			groupIDAfter = groupsManager.AddPlayerToGroup(groupID, GetPlayerID());
		
		if (groupIDAfter != m_iGroupID)
		{
			m_iGroupID = groupIDAfter;
			Rpc(RPC_DoChangeGroupID, groupIDAfter);
		}
	}
}

modded class SCR_BaseGameMode
{
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		
		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually. 
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);

		if (IsMaster())
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (controlledEntity)
			{
				if (SCR_ReconnectComponent.GetInstance() && SCR_ReconnectComponent.GetInstance().IsReconnectEnabled())
				{
					if (SCR_ReconnectComponent.GetInstance().OnPlayerDC(playerId, cause))	// if conditions to allow reconnect pass, skip the entity delete  
					{
						CharacterControllerComponent charController = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
						if (charController)
						{
							charController.SetMovement(0, vector.Forward);
						}
						
						CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(controlledEntity.FindComponent(CompartmentAccessComponent)); // TODO nullcheck
						if (compAccess)
						{
							BaseCompartmentSlot compartment = compAccess.GetCompartment();
							if (compartment)
							{
								if(GetGame().GetIsClientAuthority())
								{
									CarControllerComponent carController = CarControllerComponent.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent));
									if (carController)
									{
										carController.Shutdown();
										carController.StopEngine(false);
									}
								}
								else
								{
									CarControllerComponent_SA carController = CarControllerComponent_SA.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent_SA));
									if (carController)
									{
										carController.Shutdown();
										carController.StopEngine(false);
									}
								}
							}
						}
						
						return;
					}
				}
				
				//RplComponent.DeleteRplEntity(controlledEntity, false);
			}
		}
	}
	
	void SetGameModeState(SCR_EGameModeState state)
	{
		if (!IsMaster())
			return;

		m_eGameState = state;
		Replication.BumpMe();

		OnGameStateChanged();
	}
}

modded enum SCR_EGameModeState
{
	SLOTSELECTION,
	BRIEFING,
	NULL,
}

modded class SCR_MapRadialUI
{
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.IsMarkersOnlyOnBriefing())
		{
			if (gameMode.GetState() != SCR_EGameModeState.BRIEFING) return;
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			PlayerController playerController = GetGame().GetPlayerController();
			if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		}
		if (m_RadialMenu && m_RadialMenu.IsOpened())
			return;
		
		m_RadialController.OnInputOpen();
	}
}

modded class SCR_MapMarkersUI
{
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode.IsMarkersOnlyOnBriefing())
		{
			if (gameMode.GetState() != SCR_EGameModeState.BRIEFING) return;
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			PlayerController playerController = GetGame().GetPlayerController();
			if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		}
		
		SCR_MapRadialUI mapRadial = SCR_MapRadialUI.GetInstance();
		if (!mapRadial)
			return;
		
		mapRadial.GetRadialController().OnInputOpen();
		mapRadial.GetRadialController().GetRadialMenu().PerformEntry(m_RootCategoryEntry);
	
	}
}

modded class SCR_ButtonComponent
{
	ref ScriptInvoker<Widget> m_OnHoverWithWidget = new ScriptInvoker();
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);
		m_OnHover.Invoke(); // Why not pass by default???
		m_OnHoverWithWidget.Invoke(w);
		
		return false;
	}
}


// Callsings manual set
modded class SCR_CallsignManagerComponent
{
	// Make it public for manual remove
	override void RemoveAvailibleGroupCallsign(Faction faction, int companyIndex, int platoonIndex, int squadIndex)
	{
		super.RemoveAvailibleGroupCallsign(faction, companyIndex, platoonIndex, squadIndex);
	}
}
modded class SCR_CallsignGroupComponent
{
	void ReAssignGroupCallsign(int company, int platoon, int squad)
	{		
		if (!m_CallsignManager)
			return;
		
		// Make old calsign availible
		int companyOld, platoonOld, squadOld;
		GetCallsignIndexes(companyOld, platoonOld, squadOld);
		m_CallsignManager.MakeGroupCallsignAvailible(m_Faction, companyOld, platoonOld, squadOld);
		
		// Remove manuale set callsign from availibles
		m_CallsignManager.RemoveAvailibleGroupCallsign(m_Faction, company, platoon, squad);
		
		AssignCallsignBroadcast(company, platoon, squad);
		Rpc(AssignCallsignBroadcast, company, platoon, squad);
	}
}

// Remove squad markers
modded class SCR_MapMarkerEntrySquadLeader
{
	override void RegisterMarker(SCR_MapMarkerSquadLeader marker, SCR_AIGroup group)
	{
		
	}
}

modded class SCR_PlayersRestrictionZoneManagerComponent
{
	ref set<SCR_EditorRestrictionZoneEntity> GetZones()
	{
		return m_aRestrictionZones;
	}
}

/*
// Someone rewrite it please
modded class SCR_PlayersRestrictionZoneManagerComponent
{
	override protected void KillPlayerOutOfZone(int playerID, IEntity playerEntity)
	{
		if (!playerEntity)
			return;
		
		vector playerEntityPosition = playerEntity.GetOrigin();
		float minDistanceSqrXZ = 99999999;
		SCR_EditorRestrictionZoneEntity nearestZone = null;
		for (int i = 0; i < m_aRestrictionZones.Count(); i++)
		{
			SCR_EditorRestrictionZoneEntity zone = m_aRestrictionZones.Get(i);
			vector restrictionZonePosition = zone.GetOrigin();
			float distanceSqrXZ = vector.DistanceSqXZ(playerEntityPosition, restrictionZonePosition);
			if (minDistanceSqrXZ > distanceSqrXZ)
			{
				minDistanceSqrXZ = distanceSqrXZ;
				nearestZone = zone;
			}
		}
		
		if (nearestZone)
		{
			vector restrictionZonePosition = nearestZone.GetOrigin();
			float distance = vector.DistanceXZ(playerEntityPosition, restrictionZonePosition);
			float maxDistance = nearestZone.GetRestrictionZoneRadius();
			float moveDistance = (distance - maxDistance) * 5.0;
			vector moveVector = restrictionZonePosition - playerEntityPosition;
			moveVector[2] = 0;
			moveVector.Normalize();
			playerEntity.SetOrigin(playerEntityPosition - moveVector * moveDistance);
		}
	}
}
*/
