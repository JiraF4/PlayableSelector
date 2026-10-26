class PS_KillList : ScriptedWidgetComponent
{
	protected ResourceName m_sKillEntryPrefab = "{FCAD3464FF75D530}UI/Spectator/KillListEntry.layout";

	protected WorkspaceWidget m_WorkspaceWidget;
	protected PS_PlayableManager m_PlayableManager;
	protected PS_KillListManager m_KillListManager;
	protected int m_iLocalPlayerId;

	protected VerticalLayoutWidget m_wYourKillsList;
	protected VerticalLayoutWidget m_wKilledYouList;
	protected VerticalLayoutWidget m_wTeamKillsList;
	protected TextWidget m_wYourKillsCount;
	protected TextWidget m_wKilledYouCount;
	protected TextWidget m_wTeamKillsCount;
	protected Widget m_wYourKillsSection;
	protected Widget m_wKilledYouSection;
	protected Widget m_wTeamKillsSection;
	protected Widget m_wScroll;
	protected Widget m_wContent;

	protected ref array<Widget> m_aYourKillsWidgets = {};
	protected ref array<Widget> m_aKilledYouWidgets = {};
	protected ref array<Widget> m_aTeamKillsWidgets = {};

	protected int m_iYourKillsCount;
	protected int m_iKilledYouCount;
	protected int m_iTeamKillsCount;

	protected ref SCR_AmmoTypeInfoConfig m_AmmoTypeConf;

	protected void InitAmmoTypeConfig()
	{
		if (m_AmmoTypeConf)
			return;

		Resource holder = BaseContainerTools.LoadContainer("{8D3E102893955B15}Configs/Inventory/ItemHints/MagazineAmmoType.conf");
		if (!holder || !holder.IsValid())
			return;

		BaseContainer container = holder.GetResource().ToBaseContainer();
		if (!container)
			return;

		m_AmmoTypeConf = SCR_AmmoTypeInfoConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
	}

	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;

		super.HandlerAttached(w);

		m_WorkspaceWidget = GetGame().GetWorkspace();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_KillListManager = PS_KillListManager.GetInstance();

		m_wYourKillsList = VerticalLayoutWidget.Cast(w.FindAnyWidget("YourKillsList"));
		m_wKilledYouList = VerticalLayoutWidget.Cast(w.FindAnyWidget("KilledYouList"));
		m_wTeamKillsList = VerticalLayoutWidget.Cast(w.FindAnyWidget("TeamKillsList"));
		m_wYourKillsCount = TextWidget.Cast(w.FindAnyWidget("YourKillsCount"));
		m_wKilledYouCount = TextWidget.Cast(w.FindAnyWidget("KilledYouCount"));
		m_wTeamKillsCount = TextWidget.Cast(w.FindAnyWidget("TeamKillsCount"));
		m_wYourKillsSection = w.FindAnyWidget("YourKillsSection");
		m_wKilledYouSection = w.FindAnyWidget("KilledYouSection");
		m_wTeamKillsSection = w.FindAnyWidget("TeamKillsSection");

		// The scroll content and its whole nested VerticalLayout chain MUST be forced to stretch horizontally
		// to the scroll width. Every visible element here (section labels AND the kill rows) is a TextWidget
		// with FillWeight, which reports no intrinsic width - so a default (Left-aligned) VerticalLayout
		// collapses the content to ~0 width and NOTHING inside the scroll renders. Only the "Kill feed" header
		// stays visible because it lives outside the scroll. The working AlivePlayersList and vanilla
		// SCR_ConfigurableDialogUI / SCR_DownloadManagerListComponent apply the same Stretch. Runtime-created
		// rows are stretched the same way in AddKillEntry.
		m_wScroll = w.FindAnyWidget("KillListScroll");
		m_wContent = w.FindAnyWidget("KillListContent");
		if (m_wContent)
		{
			AlignableSlot.SetHorizontalAlign(m_wContent, LayoutHorizontalAlign.Stretch);
			// Top-align (NOT stretch) vertically so the content grows downward past the viewport and the
			// ScrollLayout engages once the kill list overflows, instead of being squashed to fit.
			AlignableSlot.SetVerticalAlign(m_wContent, LayoutVerticalAlign.Top);
		}
		StretchToParentWidth(m_wYourKillsSection);
		StretchToParentWidth(m_wTeamKillsSection);
		StretchToParentWidth(m_wKilledYouSection);
		StretchToParentWidth(m_wYourKillsList);
		StretchToParentWidth(m_wTeamKillsList);
		StretchToParentWidth(m_wKilledYouList);
		// Headers too, so the "N" count is pushed to the right edge (its label uses FillWeight 1).
		StretchToParentWidth(w.FindAnyWidget("YourKillsHeader"));
		StretchToParentWidth(w.FindAnyWidget("TeamKillsHeader"));
		StretchToParentWidth(w.FindAnyWidget("KilledYouHeader"));
		StretchToParentWidth(w.FindAnyWidget("KillListBorder"));

		// DIAGNOSTIC: did the component attach, are the child widgets found, is the manager wired? Logs only
		// these once at attach so we can tell whether the (empty) feed is a wiring bug vs a routing filter.
		int dbgHistory = -1;
		if (m_KillListManager)
			dbgHistory = m_KillListManager.GetKillHistory().Count();
		PrintFormat("[PS_KillList] HandlerAttached yourList=%1 killedYouList=%2 teamKillsList=%3 mgrNull=%4 history=%5",
			m_wYourKillsList, m_wKilledYouList, m_wTeamKillsList, !m_KillListManager, dbgHistory);

		// Routing uses the local playerId only - it is stable for the whole session and matches the ids the
		// server stamps into each kill event. (The old GUID round-trip resolved empty on peer/test clients
		// and re-ran ReplayHistory on every reply, spamming the log; it bought nothing playerId did not.)
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
			m_iLocalPlayerId = playerController.GetPlayerId();

		PrintFormat("[PS_KillList] HandlerAttached resolved localPid=%1", m_iLocalPlayerId);

		if (m_KillListManager)
			m_KillListManager.GetOnKillEvent().Insert(OnKillEvent);

		Reset();
		ReplayHistory();
	}

	void ~PS_KillList()
	{
		if (!GetGame() || !GetGame().InPlayMode())
			return;

		// Mirror PS_VoiceChatList's destructor guard: unsubscribe only if the manager AND its invoker are
		// still valid. (The callqueue access that used to fault here at shutdown was removed with the
		// LogSizes diagnostic; this hardens the remaining listener removal against a torn-down manager.)
		if (!m_KillListManager)
			return;
		PS_ScriptInvokerKillEvent onKillEvent = m_KillListManager.GetOnKillEvent();
		if (onKillEvent)
			onKillEvent.Remove(OnKillEvent);
	}

	void ReplayHistory()
	{
		if (!m_KillListManager)
			return;

		array<ref PS_KillInfo> history = m_KillListManager.GetKillHistory();
		foreach (PS_KillInfo killInfo : history)
		{
			ProcessKillInternal(killInfo);
		}
		UpdateCounts();
	}

	void OnKillEvent(PS_KillInfo killInfo)
	{
		PrintFormat("[PS_KillList] OnKillEvent victim=%1 killer=%2 localPid=%3",
			killInfo.m_iVictimPlayerId, killInfo.m_iKillerPlayerId, m_iLocalPlayerId);
		ProcessKillInternal(killInfo);
		UpdateCounts();
	}

	protected void ProcessKillInternal(PS_KillInfo killInfo)
	{
		bool isLocalVictim = killInfo.IsVictim(m_iLocalPlayerId);
		bool isLocalKiller = killInfo.IsKiller(m_iLocalPlayerId);

		PrintFormat("[PS_KillList] ProcessKill victim=%1 killer=%2 teamKill=%3 localPid=%4 killerMatch=%5 victimMatch=%6 -> your=%7 killed=%8 team=%9",
			killInfo.m_iVictimPlayerId, killInfo.m_iKillerPlayerId, killInfo.m_bIsTeamKill, m_iLocalPlayerId,
			isLocalKiller, isLocalVictim,
			isLocalKiller && !killInfo.m_bIsTeamKill,
			isLocalVictim && killInfo.m_iKillerPlayerId > 0,
			killInfo.m_bIsTeamKill && isLocalKiller);

		bool isSelfKill = (killInfo.m_iKillerPlayerId > 0 && killInfo.m_iKillerPlayerId == killInfo.m_iVictimPlayerId);

		// Kills / Team Kills are numbered lists showing the VICTIM (squad + nickname); Killed by shows the
		// KILLER nickname without a number. Self-inflicted deaths (suicide, fall) do not count as kills.
		if (isLocalKiller && !killInfo.m_bIsTeamKill && !isSelfKill)
		{
			m_iYourKillsCount++;
			AddKillEntry(m_wYourKillsList, m_aYourKillsWidgets, killInfo, killInfo.GetVictimDisplayName(), m_iYourKillsCount);
		}

		if (killInfo.m_bIsTeamKill && isLocalKiller && !isSelfKill)
		{
			m_iTeamKillsCount++;
			AddKillEntry(m_wTeamKillsList, m_aTeamKillsWidgets, killInfo, killInfo.GetVictimDisplayName(), m_iTeamKillsCount);
		}

		if (isLocalVictim && (killInfo.m_iKillerPlayerId > 0 || killInfo.m_sWeaponName == "#PS-KillList_CauseFall" || killInfo.m_sWeaponName == "#PS-KillList_CauseBleeding"))
		{
			m_iKilledYouCount++;
			string killerDisplay = killInfo.GetKillerDisplayName();
			if (isSelfKill || killInfo.m_iKillerPlayerId <= 0)
				killerDisplay = killInfo.GetVictimDisplayName();
			AddKillEntry(m_wKilledYouList, m_aKilledYouWidgets, killInfo, killerDisplay, m_iKilledYouCount);
		}
	}

	protected void AddKillEntry(VerticalLayoutWidget listWidget, inout array<Widget> widgetArray, PS_KillInfo killInfo, string displayName, int number)
	{
		if (!listWidget)
		{
			PrintFormat("[PS_KillList] AddKillEntry SKIP listWidget=null name=%1", displayName);
			return;
		}
		Widget entryWidget = m_WorkspaceWidget.CreateWidgets(m_sKillEntryPrefab, listWidget);
		if (!entryWidget)
		{
			PrintFormat("[PS_KillList] AddKillEntry SKIP createFailed name=%1", displayName);
			return;
		}
		// Stretch each card to the available list width.
		LayoutSlot.SetHorizontalAlign(entryWidget, LayoutHorizontalAlign.Stretch);

		bool isKilledYou = (listWidget == m_wKilledYouList);

		ImageWidget accentBar = ImageWidget.Cast(entryWidget.FindAnyWidget("AccentBar"));
		TextWidget timeText = TextWidget.Cast(entryWidget.FindAnyWidget("TimeText"));
		Widget indexBadge = entryWidget.FindAnyWidget("IndexBadge");
		Widget indexBg = entryWidget.FindAnyWidget("IndexBg");
		Widget indexBorder = entryWidget.FindAnyWidget("IndexBorder");
		TextWidget indexText = TextWidget.Cast(entryWidget.FindAnyWidget("IndexText"));
		ImageWidget headshotIcon = ImageWidget.Cast(entryWidget.FindAnyWidget("HeadshotIcon"));
		Widget playerRow = entryWidget.FindAnyWidget("PlayerRow");
		TextWidget playerNameText = TextWidget.Cast(entryWidget.FindAnyWidget("PlayerNameText"));
		ImageWidget factionBg = ImageWidget.Cast(entryWidget.FindAnyWidget("FactionBg"));
		TextWidget factionText = TextWidget.Cast(entryWidget.FindAnyWidget("FactionText"));
		TextWidget distanceText = TextWidget.Cast(entryWidget.FindAnyWidget("DistanceText"));
		TextWidget metaDivider = TextWidget.Cast(entryWidget.FindAnyWidget("MetaDivider"));
		ImageWidget metaBg = ImageWidget.Cast(entryWidget.FindAnyWidget("MetaBg"));
		TextWidget hitZoneText = TextWidget.Cast(entryWidget.FindAnyWidget("HitZoneText"));
		ItemPreviewWidget weaponPreview = ItemPreviewWidget.Cast(entryWidget.FindAnyWidget("WeaponPreview"));
		ImageWidget weaponIcon = ImageWidget.Cast(entryWidget.FindAnyWidget("WeaponIcon"));
		Widget weaponIconSlot = entryWidget.FindAnyWidget("WeaponIconSlot");
		TextWidget weaponNameText = TextWidget.Cast(entryWidget.FindAnyWidget("WeaponNameText"));
		TextWidget bulletDivider = TextWidget.Cast(entryWidget.FindAnyWidget("BulletDivider"));
		TextWidget magazineText = TextWidget.Cast(entryWidget.FindAnyWidget("MagazineText"));
		TextWidget divider2 = TextWidget.Cast(entryWidget.FindAnyWidget("Divider2"));
		TextWidget ammoDetailText = TextWidget.Cast(entryWidget.FindAnyWidget("AmmoDetailText"));
		TextWidget divider3 = TextWidget.Cast(entryWidget.FindAnyWidget("Divider3"));
		ImageWidget ammoTypeIcon = ImageWidget.Cast(entryWidget.FindAnyWidget("AmmoTypeIcon"));
		TextWidget ammoTypeText = TextWidget.Cast(entryWidget.FindAnyWidget("AmmoTypeText"));

		// 1. Timecode (clean MM:SS without brackets)
		if (timeText)
			timeText.SetText(killInfo.GetFormattedTimestamp());

		// 2. Accent bar colors
		if (killInfo.m_bIsTeamKill)
		{
			if (accentBar)
				accentBar.SetColor(Color.FromRGBA(239, 68, 68, 255));
		}
		else if (isKilledYou)
		{
			if (accentBar)
				accentBar.SetColor(Color.FromRGBA(239, 68, 68, 255));
		}
		else
		{
			if (accentBar)
				accentBar.SetColor(Color.FromRGBA(234, 179, 8, 255));
		}

		// 3. Index badge (clean uniform 26px badge across all lists)
		if (indexText)
		{
			indexText.SetVisible(true);
			if (number < 10)
				indexText.SetText("0" + number.ToString());
			else
				indexText.SetText(number.ToString());

			indexText.SetColor(Color.FromRGBA(225, 225, 225, 255));
			FrameSlot.SetPosX(indexText, 0);
			FrameSlot.SetSizeX(indexText, 26);
			if (indexBadge)
				FrameSlot.SetSizeX(indexBadge, 26);
			if (indexBg)
				FrameSlot.SetSizeX(indexBg, 26);
			if (indexBorder)
				FrameSlot.SetSizeX(indexBorder, 26);
			if (playerRow)
			{
				FrameSlot.SetPosX(playerRow, 88);
				FrameSlot.SetSizeX(playerRow, 246);
			}
		}

		// 4. Nickname & TeamKill badge
		Widget teamKillBadge = entryWidget.FindAnyWidget("TeamKillBadge");
		if (teamKillBadge)
			teamKillBadge.SetVisible(killInfo.m_bIsTeamKill);

		if (playerNameText)
		{
			if (!teamKillBadge && killInfo.m_bIsTeamKill)
				playerNameText.SetText(displayName + " [TK]");
			else
				playerNameText.SetText(displayName);
		}

		// 5. Faction badge & color (clean text without brackets)
		string fKey;
		string fName;
		if (isKilledYou)
		{
			fKey = killInfo.m_sKillerFactionKey;
			fName = killInfo.GetKillerFactionDisplayName();
		}
		else
		{
			fKey = killInfo.m_sVictimFactionKey;
			fName = killInfo.GetVictimFactionDisplayName();
		}

		if (factionText)
			factionText.SetText(fName);

		if (factionBg)
		{
			Color fColor = Color.FromRGBA(30, 40, 50, 200);
			FactionManager fMgr = GetGame().GetFactionManager();
			if (fMgr && fKey != "" && fKey != "-")
			{
				Faction faction = fMgr.GetFactionByKey(fKey);
				if (faction)
					fColor = faction.GetFactionColor();
			}
			factionBg.SetColor(fColor);
		}

		// 6. Meta badge (Amber distance + White HitZone, dark neutral background)
		string distStr = killInfo.GetFormattedDistance();
		bool hasDistance = (distStr != "" && distStr != "—");
		if (distanceText)
		{
			if (hasDistance)
			{
				distanceText.SetText(distStr);
				//distanceText.SetColor(Color.FromRGBA(234, 198, 115, 255));
				distanceText.SetVisible(true);
			}
			else
			{
				distanceText.SetVisible(false);
			}
		}

		string hitZone = killInfo.GetLocalizedHitZone();
		bool isHeadshot = (killInfo.m_eLastHitZoneGroup == ECharacterHitZoneGroup.HEAD);
		if (headshotIcon)
			headshotIcon.SetVisible(isHeadshot);

		if (hitZone != "" || hasDistance)
		{
			if (hitZoneText)
			{
				if (hitZone != "")
				{
					hitZoneText.SetText(hitZone);
					if (isHeadshot)
						hitZoneText.SetColor(Color.FromRGBA(255, 120, 120, 255));
					else
						hitZoneText.SetColor(Color.FromRGBA(245, 245, 245, 255));
					hitZoneText.SetVisible(true);
				}
				else
				{
					hitZoneText.SetVisible(false);
				}
			}
			if (metaDivider)
				metaDivider.SetVisible(hasDistance && hitZone != "");
			if (metaBg)
			{
				metaBg.SetColor(Color.FromRGBA(14, 14, 14, 220));
				metaBg.SetVisible(true);
			}
		}
		else
		{
			if (hitZoneText)
				hitZoneText.SetVisible(false);
			if (metaDivider)
				metaDivider.SetVisible(false);
			if (headshotIcon)
				headshotIcon.SetVisible(false);
			if (metaBg)
				metaBg.SetVisible(false);
		}

		// 7. Bottom row: 3D ItemPreview / 2D fallback icon, weapon name, magazine, caliber
		string weapon = killInfo.GetWeaponDisplayName();
		bool hasWeapon = (weapon != "" && weapon != "—");

		bool previewShown = false;
		if (killInfo.m_sWeaponIconPath != "" && killInfo.m_sWeaponIconPath != "-")
		{
			if (killInfo.m_sWeaponIconPath.EndsWith(".et"))
			{
				ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
				if (world)
				{
					ItemPreviewManagerEntity previewManager = world.GetItemPreviewManager();
					if (!previewManager)
					{
						Resource rsc = Resource.Load("{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et");
						if (rsc && rsc.IsValid())
							GetGame().SpawnEntityPrefabLocal(rsc, world);
						previewManager = world.GetItemPreviewManager();
					}
					if (previewManager && weaponPreview)
					{
						previewManager.SetPreviewItemFromPrefab(weaponPreview, killInfo.m_sWeaponIconPath);
						weaponPreview.SetResolutionScale(1, 1);
						weaponPreview.SetVisible(true);
						previewShown = true;
					}
				}
			}
		}

		if (weaponPreview && !previewShown)
			weaponPreview.SetVisible(false);

		if (weaponIcon)
		{
			if (previewShown)
			{
				weaponIcon.SetVisible(false);
			}
			else if (killInfo.m_sWeaponIconPath != "" && killInfo.m_sWeaponIconPath != "-")
			{
				if (killInfo.m_sWeaponIconPath.Contains("@"))
				{
					array<string> iconParts = {};
					killInfo.m_sWeaponIconPath.Split("@", iconParts, false);
					if (iconParts.Count() >= 2)
						weaponIcon.LoadImageFromSet(0, iconParts[0], iconParts[1]);
					else
						weaponIcon.LoadImageTexture(0, iconParts[0]);
				}
				else
				{
					weaponIcon.LoadImageTexture(0, killInfo.m_sWeaponIconPath);
				}
				weaponIcon.SetColor(Color.FromRGBA(225, 225, 225, 240));
				weaponIcon.SetVisible(true);
			}
			else if (hasWeapon && killInfo.m_sWeaponIconPath != "-")
			{
				weaponIcon.LoadImageFromSet(0, "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset", "WeaponRifle");
				weaponIcon.SetColor(Color.FromRGBA(225, 225, 225, 240));
				weaponIcon.SetVisible(true);
			}
			else
			{
				weaponIcon.SetVisible(false);
			}
		}

		if (weaponIconSlot)
			weaponIconSlot.SetVisible(previewShown || (weaponIcon && weaponIcon.IsVisible()));

		if (weaponNameText)
		{
			if (hasWeapon)
				weaponNameText.SetText(weapon);
			else
				weaponNameText.SetText("—");
			weaponNameText.SetVisible(true);
		}

		if (hasWeapon)
		{
			string magazine = killInfo.GetFormattedMagazine();
			if (magazine == "")
				magazine = killInfo.GetMagazineDisplayName();

			if (magazine != "" && magazine != "-")
			{
				if (magazineText)
				{
					magazineText.SetText(magazine);
					magazineText.SetVisible(true);
				}
				if (bulletDivider)
					bulletDivider.SetVisible(true);
			}
			else
			{
				if (magazineText)
					magazineText.SetVisible(false);
				if (bulletDivider)
					bulletDivider.SetVisible(false);
			}

			string caliber = killInfo.GetFormattedCaliber();
			if (caliber != "" && caliber != "-")
			{
				if (ammoDetailText)
				{
					ammoDetailText.SetText(caliber);
					ammoDetailText.SetVisible(true);
				}
				if (divider2)
					divider2.SetVisible(magazineText && magazineText.IsVisible());
			}
			else
			{
				if (ammoDetailText)
					ammoDetailText.SetVisible(false);
				if (divider2)
					divider2.SetVisible(false);
			}

			// Ammo type icon & text (AP, FMJ, HE, etc.)
			int ammoFlags = killInfo.GetAmmoTypeFlags();
			string ammoTypeName = killInfo.GetAmmoTypeName();
			bool hasAmmoType = (ammoFlags != 0 || (ammoTypeName != "" && ammoTypeName != "-"));

			if (hasAmmoType)
			{
				bool iconLoaded = false;
				if (ammoFlags != 0)
				{
					InitAmmoTypeConfig();
					if (m_AmmoTypeConf && m_AmmoTypeConf.m_aAmmoTypes)
					{
						foreach (SCR_AmmoTypeInfoConfigEntry entry : m_AmmoTypeConf.m_aAmmoTypes)
						{
							if (entry.m_eAmmoType & ammoFlags)
							{
								if (ammoTypeIcon)
								{
									ammoTypeIcon.LoadImageFromSet(0, m_AmmoTypeConf.m_sImagesetPath, entry.m_sQuadName);
									ammoTypeIcon.SetColor(Color.FromRGBA(225, 225, 225, 240));
									ammoTypeIcon.SetVisible(true);
									iconLoaded = true;
								}
								if (ammoTypeName == "" && entry.m_sDescription != "")
									ammoTypeName = WidgetManager.Translate(entry.m_sDescription);
								break;
							}
						}
					}
				}

				if (ammoTypeIcon && !iconLoaded)
					ammoTypeIcon.SetVisible(false);

				if (ammoTypeText)
				{
					if (ammoTypeName != "" && ammoTypeName != "-")
					{
						ammoTypeText.SetText(ammoTypeName);
						ammoTypeText.SetVisible(true);
					}
					else
					{
						ammoTypeText.SetVisible(false);
					}
				}

				if (divider3)
					divider3.SetVisible(ammoDetailText && ammoDetailText.IsVisible() && (iconLoaded || (ammoTypeText && ammoTypeText.IsVisible())));
			}
			else
			{
				if (ammoTypeIcon)
					ammoTypeIcon.SetVisible(false);
				if (ammoTypeText)
					ammoTypeText.SetVisible(false);
				if (divider3)
					divider3.SetVisible(false);
			}
		}
		else
		{
			if (magazineText)
				magazineText.SetVisible(false);
			if (bulletDivider)
				bulletDivider.SetVisible(false);
			if (ammoDetailText)
				ammoDetailText.SetVisible(false);
			if (divider2)
				divider2.SetVisible(false);
			if (ammoTypeIcon)
				ammoTypeIcon.SetVisible(false);
			if (ammoTypeText)
				ammoTypeText.SetVisible(false);
			if (divider3)
				divider3.SetVisible(false);
		}

		widgetArray.Insert(entryWidget);
	}

	// Force a widget that sits inside a Vertical/HorizontalLayoutWidget to fill its parent's width.
	protected void StretchToParentWidth(Widget widget)
	{
		if (widget)
			LayoutSlot.SetHorizontalAlign(widget, LayoutHorizontalAlign.Stretch);
	}

	protected void UpdateCounts()
	{
		if (m_wYourKillsCount)
			m_wYourKillsCount.SetText(m_iYourKillsCount.ToString());
		if (m_wKilledYouCount)
			m_wKilledYouCount.SetText(m_iKilledYouCount.ToString());
		if (m_wTeamKillsCount)
			m_wTeamKillsCount.SetText(m_iTeamKillsCount.ToString());

		if (m_wYourKillsSection)
			m_wYourKillsSection.SetVisible(m_iYourKillsCount > 0);
		if (m_wKilledYouSection)
			m_wKilledYouSection.SetVisible(m_iKilledYouCount > 0);
		if (m_wTeamKillsSection)
			m_wTeamKillsSection.SetVisible(m_iTeamKillsCount > 0);
	}

	void Reset()
	{
		ClearList(m_wYourKillsList, m_aYourKillsWidgets);
		ClearList(m_wKilledYouList, m_aKilledYouWidgets);
		ClearList(m_wTeamKillsList, m_aTeamKillsWidgets);
		m_iYourKillsCount = 0;
		m_iKilledYouCount = 0;
		m_iTeamKillsCount = 0;
		UpdateCounts();
	}

	protected void ClearList(VerticalLayoutWidget listWidget, inout array<Widget> widgetArray)
	{
		if (!listWidget)
			return;

		SCR_WidgetHelper.RemoveAllChildren(listWidget);
		widgetArray.Clear();
	}

	void RefreshLocalPlayerId()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
			m_iLocalPlayerId = playerController.GetPlayerId();
	}

	void OnSpectatorMenuOpen()
	{
		RefreshLocalPlayerId();
		Reset();
		ReplayHistory();
	}

}
