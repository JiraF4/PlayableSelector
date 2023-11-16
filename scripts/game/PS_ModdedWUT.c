
modded class SCR_CallsignGroupComponent
{
	override bool IsUniqueRoleInUse(int roleToCheck)
	{
		// HOW THIS CAN LOST !!!OWN!!! GROUP?
		if (!m_Group) return true;
		
		if (m_Group.IsSlave())
		{
			SCR_AIGroup master = m_Group.GetMaster();
			
			if (master)
			{
				SCR_CallsignGroupComponent masterCallsignComponent = SCR_CallsignGroupComponent.Cast(master.FindComponent(SCR_CallsignGroupComponent));
				
				if (masterCallsignComponent)
					return masterCallsignComponent.IsUniqueRoleInUse(roleToCheck);
			}
		}
			
		//~ Was not slave or master group could not be obtained for some reason
		return m_mAIRoleCallsigns.Contains(roleToCheck) || m_mPlayerRoleCallsigns.Contains(roleToCheck);
	}
}

modded class SCR_SupportStationManagerComponent
{
	override void AddSupportStation(notnull SCR_BaseSupportStationComponent supportStation)
	{
		if (supportStation.GetSupportStationType() == ESupportStationType.NONE)
		{
			Print("Cannot add SupportStation of type NONE! Make sure to overwrite GetSupportStationType() in inherited class.", LogLevel.ERROR);
			return;
		}
		
		array<SCR_BaseSupportStationComponent> supportStations = {};
		GetArrayOfSupportStationType(supportStation.GetSupportStationType(), supportStations);
		
		//~ Already in list		
		if (supportStations.Contains(supportStation))
			return;
		
		for (int i = 0, count = supportStations.Count(); i < count; i++)
		{
			// A? How null can get here
			// What ever, I just add check
			auto ss = supportStations[i];
			ESupportStationPriority prior = ESupportStationPriority.LOW;
			if (ss) prior = ss.GetSupportStationPriority();
			
			//~ Make sure it is added near the at priority in the array
			if (supportStation.GetSupportStationPriority() >= prior)
			{
				//~ Insert in list at the correct location
				m_mSupportStations[supportStation.GetSupportStationType()].InsertAt(supportStation, i);
				return;
			}
		}
		
		//~ Add to last position in array as loop above did not add it
		m_mSupportStations[supportStation.GetSupportStationType()].Insert(supportStation);
	}
}