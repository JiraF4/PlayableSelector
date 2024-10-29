modded class SCR_PingEditorComponent
{
	override protected void ReceivePing(int reporterID, bool reporterInEditor, SCR_EditableEntityComponent reporterEntity, bool unlimitedOnly, vector position, RplId targetID)
	{
		super.ReceivePing(reporterID, reporterInEditor, reporterEntity, unlimitedOnly, position, targetID);
		
		SCR_EditorManagerEntity manager = GetManager();
		if (!SCR_Global.IsAdmin(manager.GetPlayerID()))
			return;
		
		Rpc(ReceivePingObserver, reporterID, reporterInEditor, unlimitedOnly, position, targetID);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ReceivePingObserver(int reporterID, bool reporterInEditor, bool unlimitedOnly, vector position, RplId targetID)
	{
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu));
		if (!spectatorMenu)
			return;
		
		spectatorMenu.ReceivePing(reporterID, position, targetID);
	}
}