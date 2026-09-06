modded class SCR_MapMarkersUI
{
	protected const float DOUBLE_CLICK_MS = 400;
	protected const int DOUBLE_CLICK_MAX_DIST_PX = 12;
	protected float m_fLastClickTime = -float.MAX;
	protected int m_iLastClickX;
	protected int m_iLastClickY;
	protected float m_fQuickMarkerWorldX;
	protected float m_fQuickMarkerWorldY;
	protected bool m_bHasQuickMarkerPos;

	static bool PS_CanLocalPlayerPlaceMarkers()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameMode) return true;
		SCR_EGameModeState state = gameMode.GetState();
		if (state != SCR_EGameModeState.BRIEFING && state != SCR_EGameModeState.GAME && state != SCR_EGameModeState.SLOTSELECTION) return false;
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() <= 0 || GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu)) return false;
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && playableManager.GetPlayableByPlayer(pc.GetPlayerId()) == RplId.Invalid() && !SCR_FactionManager.SGetPlayerFaction(pc.GetPlayerId())) return false;
		if (state != SCR_EGameModeState.GAME) return true;
		if (gameMode.GetMarkersOnlyOnBriefing()) return false;
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (!entity) return false;
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
		return controller && !controller.IsDead();
	}

	protected bool RegisterClickAndCheckDouble()
	{
		BaseWorld world = GetGame().GetWorld(); if (!world) return false;
		float now = world.GetWorldTime(); int mouseX, mouseY; WidgetManager.GetMousePos(mouseX, mouseY);
		bool result = now - m_fLastClickTime <= DOUBLE_CLICK_MS && Math.AbsInt(mouseX - m_iLastClickX) <= DOUBLE_CLICK_MAX_DIST_PX && Math.AbsInt(mouseY - m_iLastClickY) <= DOUBLE_CLICK_MAX_DIST_PX;
		if (result)
			m_fLastClickTime = -float.MAX;
		else
			m_fLastClickTime = now;
		m_iLastClickX = mouseX;
		m_iLastClickY = mouseY;
		return result;
	}

	override void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		if (!PS_CanLocalPlayerPlaceMarkers()) return;
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance(); if (!mapEntity) return;
		mapEntity.GetMapCursorWorldPosition(m_fQuickMarkerWorldX, m_fQuickMarkerWorldY); m_bHasQuickMarkerPos = true;
		SCR_MapMarkerMenuEntry entry = new SCR_MapMarkerMenuEntry(); entry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM); PS_OnEntryPerformed(entry);
	}

	void PS_OnEntryPerformed(SCR_MapMarkerMenuEntry entry)
	{
		if (PS_CanLocalPlayerPlaceMarkers()) OnEntryPerformed(entry);
	}

	override void OnInsertMarker(bool isLocal)
	{
		if (!PS_CanLocalPlayerPlaceMarkers()) { m_bHasQuickMarkerPos = false; CleanupMarkerEditWidget(); return; }
		if (!m_bHasQuickMarkerPos) { super.OnInsertMarker(isLocal); return; }
		m_bHasQuickMarkerPos = false;
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		if (m_bIsMilitaryMarker) { marker.SetType(SCR_EMapMarkerType.PLACED_MILITARY); marker.SetFlags(m_eMilitaryTypeAIcon | m_eMilitaryTypeBIcon); marker.SetMarkerConfigID(m_iSelectedDimensionID * 100 + m_iSelectedFactionID); }
		else { marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM); marker.SetRotation(m_fRotation); marker.SetColorEntry(m_iSelectedColorID); marker.SetIconEntry(m_iSelectedIconID); }
		marker.SetCustomText(m_EditBoxComp.GetValue()); marker.SetWorldPos(m_fQuickMarkerWorldX, m_fQuickMarkerWorldY); marker.SetTimestampVisibility(m_bIsTimestampVisible);
		ChimeraWorld world = GetGame().GetWorld(); if (world) marker.SetTimestamp(world.GetServerTimestamp());
		if (!isLocal) { FactionManager fm = GetGame().GetFactionManager(); Faction faction = SCR_FactionManager.SGetPlayerFaction(GetGame().GetPlayerController().GetPlayerId()); if (fm && faction) marker.AddMarkerFactionFlags(fm.GetFactionIndex(faction)); }
		m_MarkerMgr.InsertStaticMarker(marker, isLocal); m_OnCustomMarkerPlaced.Invoke(m_fQuickMarkerWorldX, m_fQuickMarkerWorldY, isLocal); CleanupMarkerEditWidget();
	}

	override void OnEditCancelled(SCR_InputButtonComponent button) { m_bHasQuickMarkerPos = false; m_fLastClickTime = -float.MAX; super.OnEditCancelled(button); }
	override void OnDragWidget(Widget widget) { if (PS_CanLocalPlayerPlaceMarkers()) super.OnDragWidget(widget); }

	override void OnRadialMenuInit()
	{
		super.OnRadialMenuInit(); SCR_MapRadialUI radial = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI)); if (!radial || !m_RootCategoryEntry) return;
		array<ref SCR_SelectionMenuEntry> entries = m_RootCategoryEntry.GetEntries();
		if (entries) foreach (SCR_SelectionMenuEntry entry : entries) { SCR_MapMarkerMenuEntry markerEntry = SCR_MapMarkerMenuEntry.Cast(entry); if (markerEntry && markerEntry.GetMarkerType() == SCR_EMapMarkerType.PLACED_CUSTOM) return; }
		if (m_PlacedMarkerConfig) { SCR_MapMarkerMenuEntry entry = new SCR_MapMarkerMenuEntry(); entry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM); entry.SetName(m_PlacedMarkerConfig.GetMenuDescription()); entry.GetOnPerform().Insert(OnEntryPerformed); entry.SetIcon(m_PlacedMarkerConfig.GetMenuImageset(), m_PlacedMarkerConfig.GetMenuIcon()); radial.InsertCustomRadialEntry(entry, m_RootCategoryEntry); }
	}

	override void OnInputMapSelect(float value, EActionTrigger reason)
	{
		if (!PS_CanLocalPlayerPlaceMarkers() || (m_CursorModule && (m_CursorModule.GetCursorState() & SCR_MapCursorModule.STATE_POPUP_RESTRICTED) != 0) || m_EditBoxComp) return;
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		if (widgets) foreach (Widget widget : widgets) { if (widget && SCR_MapMarkerWidgetComponent.Cast(widget.FindHandler(SCR_MapMarkerWidgetComponent))) { m_fLastClickTime = -float.MAX; super.OnInputMapSelect(value, reason); return; } }
		if (!RegisterClickAndCheckDouble()) return;
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance(); if (!mapEntity) return;
		mapEntity.GetMapCursorWorldPosition(m_fQuickMarkerWorldX, m_fQuickMarkerWorldY); m_bHasQuickMarkerPos = true;
		SCR_MapMarkerMenuEntry entry = new SCR_MapMarkerMenuEntry(); entry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM); PS_OnEntryPerformed(entry);
	}
}
