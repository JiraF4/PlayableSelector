
modded class SCR_CallsignGroupComponent
{
	override bool IsUniqueRoleInUse(int roleToCheck)
	{
		// HOW THIS CAN LOST !!!OWN!!! GROUP?
		if (!m_Group) return true;
		
		return super.IsUniqueRoleInUse(roleToCheck);
	}
}

// Говно из 1.2.1
// Долбаебы ниразу даже не пробовали запускать сервак для отладки
// Только так это говно могло попасть в релиз
modded class SCR_SaveWorkshopManager
{
	override static ResourceName GetCurrentScenarioId()
	{
		MissionHeader header = GetGame().GetMissionHeader();
		
		#ifdef WORKBENCH
		if (!header)
			return FALLBACK_SCENARIO_ID;
		#endif
		
		if (header)
			return header.GetHeaderResourceName();
		else
			return "";
	}
}
