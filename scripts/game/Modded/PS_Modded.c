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









