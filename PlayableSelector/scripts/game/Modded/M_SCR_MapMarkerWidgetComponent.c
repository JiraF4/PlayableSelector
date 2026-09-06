modded class SCR_MapMarkerWidgetComponent
{
	override void HandlerAttached(Widget w)
	{
		if (w && !w.FindAnyWidget("MarkerTimestamp")) { WorkspaceWidget workspace = GetGame().GetWorkspace(); if (workspace) { TextWidget timestamp = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.INHERIT_CLIPPING, Color.FromInt(0x00000000), 0, w)); if (timestamp) { timestamp.SetName("MarkerTimestamp"); timestamp.SetVisible(false); } } }
		super.HandlerAttached(w);
	}
	override void UpdateTimestamp(WorldTimestamp timestamp) { if (m_wMarkerTimestamp) super.UpdateTimestamp(timestamp); }
	override void SetTimestampVisibility(bool isVisible) { m_bIsTimestampVisible = isVisible; if (m_wMarkerTimestamp) m_wMarkerTimestamp.SetVisible(isVisible); }
}
