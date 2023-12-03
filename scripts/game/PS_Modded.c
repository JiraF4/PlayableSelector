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
	void SetGameModeState(SCR_EGameModeState state)
	{
		if (!IsMaster())
			return;

		m_eGameState = state;
		Replication.BumpMe();

		OnGameStateChanged();
	}
}











