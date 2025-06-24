// Callsings manual set
modded class SCR_CallsignManagerComponent
{
	// Make it public for manual remove
	void RemoveAvailibleGroupCallsign(Faction faction, int companyIndex, int platoonIndex, int squadIndex)
	{
		super.RemoveAvailableGroupCallsign(faction, companyIndex, platoonIndex, squadIndex);
	}
}