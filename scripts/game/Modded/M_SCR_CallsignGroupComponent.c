modded class SCR_CallsignGroupComponent
{
	void ReAssignGroupCallsign(int company, int platoon, int squad)
	{		
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		m_CallsignManager = SCR_CallsignManagerComponent.Cast(gameMode.FindComponent(SCR_CallsignManagerComponent));
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