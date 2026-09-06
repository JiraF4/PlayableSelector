// Remove squad markers
modded class SCR_MapMarkerManagerComponent
{
	// Faction-restricted marker visibility fix for factionless players (spectators, lobby/briefing late-joiners).
	//
	// Vanilla filters a faction-restricted STATIC marker in OnAddSynchedMarker (and during JIP RplLoad) exactly
	// ONCE, on arrival, and it FAILS OPEN on a null faction: the guard is "if (localFaction && !isMyFaction)
	// remove". A client whose SCR_FactionManager.SGetLocalPlayerFaction() is still null (spectator, or anyone
	// whose SCR_PlayerFactionAffiliationComponent hasn't resolved yet) therefore passes the filter and KEEPS
	// every faction-restricted static marker - including the enemy's.
	//
	// The destructive refilter below only fires once localFaction becomes non-null, so for a PERMANENT
	// spectator (faction stays null for the whole mission) it never runs and they see enemy markers forever.
	//
	// Constraint that shapes the fix: vanilla never re-broadcasts static markers - OnPlayerFactionChanged only
	// REMOVES markers, never re-adds them. So we must NOT destructively drop a marker during the null-faction
	// window: a late-joiner who later picks faction X must still be able to see X's markers once X resolves.
	// Solution: a REVERSIBLE widget hide, re-evaluated every frame after super.Update(). While the local
	// faction is null (or doesn't match the marker's faction), the marker's widget is hidden; the instant the
	// faction resolves to a match, it reappears - no re-add needed. This covers permanent spectators AND
	// transient lobby/briefing late-joiners with zero regression, and only touches faction-RESTRICTED markers
	// (factionFlags != 0); unrestricted markers (flags == 0) are left alone so global markers still show.
	protected Faction m_PS_LastLocalFaction;
	// Cached alongside m_PS_LastLocalFaction so the per-frame visibility pass doesn't re-resolve the faction
	// manager + faction index every frame; both are recomputed ONLY when the local faction actually changes
	// (incl. the null<->faction transitions on spectate / reconnect), so they never go stale.
	protected FactionManager m_PS_FactionManager;
	protected int m_PS_LastLocalFactionIndex = -1;

	override void Update(float timeSlice)
	{
		if (!m_MapEntity)
		{
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			m_MapEntity.GetOnMapPanEnd().Insert(OnMapPanEnd);
		}

		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (localFaction != m_PS_LastLocalFaction)
		{
			m_PS_LastLocalFaction = localFaction;
			m_PS_LastLocalFactionIndex = -1;
			if (localFaction)
			{
				if (!m_PS_FactionManager)
					m_PS_FactionManager = GetGame().GetFactionManager();
				if (m_PS_FactionManager)
					m_PS_LastLocalFactionIndex = m_PS_FactionManager.GetFactionIndex(localFaction);
				PS_RefilterStaticMarkersByLocalFaction(localFaction);
			}
		}

		super.Update(timeSlice);

		// Re-evaluate reversible faction hiding every frame after super.Update(). Cheap: it only walks the
		// (small) static + disabled arrays and touches the widget solely in the hide direction.
		PS_ApplyFactionVisibility(m_PS_LastLocalFactionIndex);
	}

	// Hide faction-restricted static markers from viewers who shouldn't see them. Reversible by design: we
	// ONLY ever force-hide, never force-show, so vanilla stays the sole owner of the show path.
	//
	// Why force-hide-only: super.Update() re-runs SetStaticMarkerDisabled on every marker each frame, and
	// SetUpdateDisabled(false) unconditionally re-shows the widget for on-screen markers. A symmetric
	// hide-AND-show with a tracking map would be defeated by that churn (we'd skip re-hiding once "tracked as
	// hidden", leaving the enemy marker visible in steady state). By touching only the hide direction:
	//  - off-screen markers stay hidden (vanilla hid them via SetUpdateDisabled),
	//  - on-screen OWN markers show (vanilla shows them, we don't interfere),
	//  - on-screen faction-restricted markers for a factionless/wrong-faction viewer get hidden by us every
	//    frame, right after vanilla shows them.
	// Restore is automatic and needs no re-broadcast: the instant the local faction resolves to a match, the
	// "else" branch stops firing and vanilla's per-frame show takes over.
	protected void PS_ApplyFactionVisibility(int localFactionIndex)
	{
		// localFactionIndex is the cached index (recomputed only on faction change in Update); -1 == factionless.
		// Walk both the active and disabled arrays: a marker panned off-screen sits in m_aDisabledMarkers but
		// its widget still exists and must stay hidden from a factionless/wrong-faction viewer.
		int restrictedSeen = 0;
		restrictedSeen += PS_ApplyFactionVisibilityToArray(m_aStaticMarkers, localFactionIndex);
		restrictedSeen += PS_ApplyFactionVisibilityToArray(m_aDisabledMarkers, localFactionIndex);

		// Spectator-path diagnostic: if the local player has no faction and there are faction-restricted
		// markers around, note it once so the leak-suppression is observable in logs.
		if (localFactionIndex == -1)
			PS_LogNullFactionHideOnce(restrictedSeen);
	}

	// Returns the count of faction-restricted markers encountered (for the null-faction diagnostic).
	protected int PS_ApplyFactionVisibilityToArray(notnull array<ref SCR_MapMarkerBase> markers, int localFactionIndex)
	{
		int restricted = 0;
		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (!marker)
				continue;

			// Only faction-RESTRICTED markers are in scope. flags == 0 means "visible to everyone" and must be
			// left untouched (it would be wrong to hide global markers from spectators).
			if (marker.GetMarkerFactionFlags() == 0)
				continue;
			restricted++;

			// Visible to the local player only if they have a faction AND it matches. localFactionIndex == -1
			// covers both "no faction at all" (spectator / not-yet-picked) and any transient null - in both
			// cases a faction-restricted marker must NOT show.
			bool shouldBeVisible = (localFactionIndex != -1) && marker.IsFaction(localFactionIndex);
			if (!shouldBeVisible)
				marker.SetVisible(false); // force-hide only; vanilla re-shows once faction resolves to a match
		}
		return restricted;
	}

	// Drop static markers that are faction-restricted to a faction other than ours. Mirrors the removal path
	// (OnRemoveSynchedMarker): RemoveItem + OnDelete.
	protected void PS_RefilterStaticMarkersByLocalFaction(notnull Faction localFaction)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;
		int localIndex = factionManager.GetFactionIndex(localFaction);
		int removed = 0;
		int kept = 0;
		int unrestricted = 0;
		for (int i = m_aStaticMarkers.Count() - 1; i >= 0; i--)
		{
			SCR_MapMarkerBase marker = m_aStaticMarkers[i];
			if (!marker || marker.GetMarkerFactionFlags() == 0)
			{
				unrestricted++;
				continue; // 0 = no faction restriction -> visible to everyone
			}
			if (marker.IsFaction(localIndex))
			{
				kept++;
				continue; // belongs to our faction -> keep
			}
			m_aStaticMarkers.RemoveItem(marker);
			marker.OnDelete();
			removed++;
		}
		// Diagnostic - fires only when the local faction first resolves or changes (not per-frame), so safe to
		// keep. removed>0 means the vanilla fail-open had shown other-faction static markers before our faction
		// was known (the JIP/reconnect leak), and we just cleaned them up.
		string dbgKey = "?";
		SCR_Faction dbgFaction = SCR_Faction.Cast(localFaction);
		if (dbgFaction)
			dbgKey = dbgFaction.GetFactionKey();
		Print(string.Format("[PS_MarkerDBG] local faction '%1' resolved -> static markers re-filtered: removed=%2 (enemy/other), kept=%3 (own), unrestricted=%4",
			dbgKey, removed, kept, unrestricted), LogLevel.NORMAL);
	}

	// One-shot diagnostic for the factionless path (spectator / pre-pick). Logged from Update() the first time
	// we observe a null local faction alongside any faction-restricted marker, so the spectator leak is
	// visible in logs without spamming every frame.
	protected bool m_bPSLoggedNullFactionHide;
	protected void PS_LogNullFactionHideOnce(int restrictedCount)
	{
		if (m_bPSLoggedNullFactionHide || restrictedCount <= 0)
			return;
		m_bPSLoggedNullFactionHide = true;
		Print(string.Format("[PS_MarkerDBG] local faction is null (spectator/not-yet-picked) -> hiding %1 faction-restricted static marker(s) via reversible widget hide", restrictedCount), LogLevel.NORMAL);
	}

	override void OnMapPanEnd(float  x, float y)
	{
		if (m_MapEntity)
			super.OnMapPanEnd(x, y);
	}
}