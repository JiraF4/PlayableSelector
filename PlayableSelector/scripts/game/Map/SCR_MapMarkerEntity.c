//! Releases the marker widget when the map stops owning it.
//!
//! Vanilla SCR_MapMarkerEntity.OnDelete only removes the root from the hierarchy and never
//! clears m_wRoot, and OnMapClosed merely unsubscribes. That is safe in vanilla because closing
//! the map destroys the widget tree the marker was parented to. Here the spectator map is a
//! layer of a menu that is only hidden, so OnCreateMarker parents a fresh widget into the next
//! map's frame on every open and the old ones pile up in the tree. Nulling m_wRoot also lets
//! OnUpdateVisibility rebuild a marker that vanilla's OnDelete left hidden.

modded class SCR_MapMarkerEntity
{
	protected void PS_DropMarkerWidget()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_MarkerWidgetComp = null;
	}

	//! A stale root would keep a dead screen's tree referenced and would stop OnUpdateVisibility
	//! from recreating the marker on the screen that is actually open.
	override void OnCreateMarker()
	{
		PS_DropMarkerWidget();
		super.OnCreateMarker();
	}

	override protected void OnMapClosed(MapConfiguration config)
	{
		super.OnMapClosed(config);
		PS_DropMarkerWidget();
	}

	override void OnDelete()
	{
		super.OnDelete();
		PS_DropMarkerWidget();
	}
}