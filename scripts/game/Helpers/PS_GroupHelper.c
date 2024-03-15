class PS_GroupHelper
{
	static string GetGroupFullName(SCR_AIGroup group)
	{
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
}