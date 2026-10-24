//! Re-points the map tool menu at the screen that is actually opening.
//!
//! Vanilla caches m_wToolMenuRoot / m_wToolMenuBar on first use and only re-resolves them when
//! they are null, so every fullscreen map in this lobby — MapMenu, BriefingMapMenu,
//! PreviewMapMenu, SpectatorMenu — shares this one component and the cache survives the switch.
//! The next screen then dereferences widgets that belong to a hidden menu layer. Vanilla never
//! notices because closing the map destroys the widget tree; here the spectator map is a layer
//! that is only hidden.
//!
//! PopulateToolMenu also only appends and OnMapClose removes nothing, so without clearing the
//! generated buttons every map press stacks another full set of tools.

modded class SCR_MapToolMenuUI
{
	protected bool m_bPopulating;

	//! Drops exactly the widgets PopulateToolMenu created, by the name it gave them. A blanket
	//! RemoveAllChildren would also destroy whatever the layout author put into the tool bar.
	protected void PS_ClearGeneratedButtons()
	{
		foreach (SCR_MapToolEntry entry : m_aMenuEntries)
		{
			if (!entry || !entry.m_ButtonComp)
				continue;

			Widget button = entry.m_ButtonComp.GetRootWidget();
			if (button)
				button.RemoveFromHierarchy();

			entry.m_ButtonComp = null;
		}
	}

	//! Rebuilds the buttons for entries whose widget belonged to a screen that is gone.
	//! PopulateToolMenu only appends widgets and never fires entry invokers, so it cannot
	//! re-enter this; m_bPopulating is kept as a guard against unexpected indirect calls
	//! during our OnMapOpen path, where super's populate runs with restored entries.
	void PS_EnsureEntryButtons()
	{
		if (m_bPopulating || !m_wToolMenuRoot || !m_wToolMenuBar)
			return;

		bool missing = false;
		foreach (SCR_MapToolEntry entry : m_aMenuEntries)
		{
			if (entry && !entry.m_ButtonComp)
			{
				missing = true;
				break;
			}
		}

		if (!missing)
			return;

		PS_ClearGeneratedButtons();

		m_bPopulating = true;
		PopulateToolMenu();
		m_bPopulating = false;
	}

	override void OnMapOpen(MapConfiguration config)
	{
		Widget menuRoot;
		if (m_MapEntity)
			menuRoot = m_MapEntity.GetMapMenuRoot();

		if (menuRoot)
		{
			// Comparing first keeps a reopened screen free of tree walks.
			Widget toolMenuRoot = menuRoot.FindAnyWidget(m_sToolMenuRootName);
			if (toolMenuRoot && toolMenuRoot != m_wToolMenuRoot)
			{
				m_wToolMenuRoot = toolMenuRoot;
				m_wToolMenuBar = toolMenuRoot.FindAnyWidget(m_sToolBarName);

				// Cached against the previous screen's tree by RegisterToolMenuEntry.
				m_wBackground = null;
				if (m_wToolMenuRoot)
					m_wBackground = m_wToolMenuRoot.FindAnyWidget(m_sBackgroundName);
			}
		}

		// No tool bar on this screen. Vanilla dereferences the bar unconditionally right after
		// resolving it, so the component is skipped rather than allowed to crash the client.
		if (!m_wToolMenuRoot || !m_wToolMenuBar)
			return;

		PS_ClearGeneratedButtons();

		m_bPopulating = true;
		super.OnMapOpen(config);
		m_bPopulating = false;
	}

	override void OnMapClose(MapConfiguration config)
	{
		PS_ClearGeneratedButtons();
		super.OnMapClose(config);
	}
}