class PS_GroupHelper
{
	//------------------------------------------------------------------------------------------------
	//! Extracting group name
	//! \param[in] group
	//! \return group name in format: "<Callsign> (<Custom name if exist>)" or empty string if group == null
	static string GetGroupFullName(SCR_AIGroup group)
	{
		if (!group)
			return "";

		string customName = group.GetCustomName();

		string company, platoon, squad, character, format;
		group.GetCallsigns(company, platoon, squad, character, format);
		string callsign;
		callsign = WidgetManager.Translate(format, company, platoon, squad, "");

		if (customName != "")
		{
			callsign = string.Format("%1 (%2)", customName, callsign);
		}

		return callsign;
	}

	// Use only in UI
	static string GroupCallsignToGroupName(SCR_Faction faction, int groupCallSign)
	{
		int companyCallsignIndex, platoonCallsignIndex, squadCallsignIndex;
		companyCallsignIndex = groupCallSign * 0.000001;
		platoonCallsignIndex = (groupCallSign % 1000000) * 0.001;
		squadCallsignIndex = (groupCallSign % 1000) / 1;

		if (!faction)
			return "";

		SCR_FactionCallsignInfo callsignInfo = faction.GetCallsignInfo();
		string company = callsignInfo.GetCompanyCallsignName(companyCallsignIndex);
		string platoon = callsignInfo.GetPlatoonCallsignName(platoonCallsignIndex);
		string squad = callsignInfo.GetSquadCallsignName(squadCallsignIndex);
		string format = callsignInfo.GetCallsignFormat(false);

		return WidgetManager.Translate(format, company, platoon, squad, "");
	}
}
