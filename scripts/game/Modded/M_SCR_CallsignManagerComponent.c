// Callsings manual set
modded class SCR_CallsignManagerComponent
{
	// Make it public for manual remove
	override void RemoveAvailibleGroupCallsign(Faction faction, int companyIndex, int platoonIndex, int squadIndex)
	{
		super.RemoveAvailibleGroupCallsign(faction, companyIndex, platoonIndex, squadIndex);
	}
}