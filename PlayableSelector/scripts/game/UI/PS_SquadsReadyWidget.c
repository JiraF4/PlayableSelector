// Squads Ready widget — shown during GAME freeze time.
// Displays every faction's groups with their leader name and ready status.
// Group leaders vote via F9 (ready) / F10 (not ready).

class PS_SquadsReadyWidget
{
	// --- widget handles ---
	protected Widget m_wRoot;
	protected VerticalLayoutWidget m_wFactionsList;
	protected TextWidget m_wHint;

	// --- caches ---
	protected PS_PlayableManager m_PlayableManager;
	protected PS_GameModeCoop m_GameModeCoop;
	protected FactionManager m_FactionManager;
	protected int m_iLocalPlayerId;
	protected bool m_bLocalIsGroupLeader;
	// Local one-shot: this client requests the (server-deduped) freeze end at most once per widget instance, so an
	// all-ready state re-evaluated on every group-ready change does not spam FreezeTimerEnd RPCs at the server.
	protected bool m_bFreezeEndRequested;

	// groupId → group label TextWidget (for text updates)
	protected ref map<int, TextWidget> m_mGroupLabels = new map<int, TextWidget>();
	// groupId → leaderPlayerId at last rebuild
	protected ref map<int, int> m_mGroupLeaders = new map<int, int>();


	// --------------------------------------------------------------------------------------------
	// Factory
	static PS_SquadsReadyWidget Create()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return null;

		PS_SquadsReadyWidget widget = new PS_SquadsReadyWidget();
		widget.BuildUI();
		widget.Subscribe();
		widget.Rebuild();
		return widget;
	}

	// --------------------------------------------------------------------------------------------
	// Create a TextWidget with a Cyrillic-capable font. Programmatically-created TextWidgets default to a font
	// WITHOUT Cyrillic glyphs, so Cyrillic player/faction names render BLANK here - the layout-based widgets
	// (Factions ready, Alive players, Voice channels) show Cyrillic only because their .layout assigns Roboto.
	// Set the same font (matches vanilla SCR_SlotLabelsComponent, which renders Cyrillic slot names this way).
	protected TextWidget CreateTextWidget(Widget parent)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		TextWidget text = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE, Color.FromInt(0x00000000), 0, parent));
		text.SetFont("{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt");
		return text;
	}

	// --------------------------------------------------------------------------------------------
	// Build the widget tree. Keep it minimal — TextWidgets in VerticalLayout auto-size correctly.
	protected void BuildUI()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		Color clearColor = Color.FromInt(0x00000000);

		// Root overlay — dark background, top-left corner
		m_wRoot = workspace.CreateWidget(WidgetType.OverlayWidgetTypeID, WidgetFlags.VISIBLE, Color.Black, 0, workspace);
		FrameSlot.SetAnchorMin(m_wRoot, 0.0, 0.0);
		FrameSlot.SetAnchorMax(m_wRoot, 0.35, 0.70);
		m_wRoot.SetOpacity(0.8);

		// Main vertical layout
		VerticalLayoutWidget mainVLayout = VerticalLayoutWidget.Cast(workspace.CreateWidget(WidgetType.VerticalLayoutWidgetTypeID, WidgetFlags.VISIBLE, clearColor, 0, m_wRoot));
		OverlaySlot.SetHorizontalAlign(mainVLayout, LayoutHorizontalAlign.Stretch);
		OverlaySlot.SetVerticalAlign(mainVLayout, LayoutVerticalAlign.Stretch);

		// Title
		TextWidget title = CreateTextWidget(mainVLayout);
		title.SetText("SQUADS READY");
		title.SetColor(Color.FromInt(0xFFDDDDDD));
		title.SetExactFontSize(24);

		// Hint text on its own line below title (visible to everyone)
		m_wHint = CreateTextWidget(mainVLayout);
		m_wHint.SetText("F9 = Ready   F10 = Not Ready");
		m_wHint.SetColor(Color.FromInt(0xFFAAAAAA));
		m_wHint.SetExactFontSize(14);

		// 2 empty lines spacing before the squad list
		TextWidget spacer1 = CreateTextWidget(mainVLayout);
		spacer1.SetExactFontSize(14);
		TextWidget spacer2 = CreateTextWidget(mainVLayout);
		spacer2.SetExactFontSize(14);

		// Faction/group list — direct child of mainVLayout (same pattern as title, which renders fine)
		m_wFactionsList = VerticalLayoutWidget.Cast(workspace.CreateWidget(WidgetType.VerticalLayoutWidgetTypeID, WidgetFlags.VISIBLE, clearColor, 0, mainVLayout));
		OverlaySlot.SetHorizontalAlign(m_wFactionsList, LayoutHorizontalAlign.Stretch);
	}

	// --------------------------------------------------------------------------------------------
	// Subscribe to events + input
	protected void Subscribe()
	{
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			m_iLocalPlayerId = pc.GetPlayerId();

		m_PlayableManager.GetOnGroupReadyChanged().Insert(OnGroupReadyChanged);
		m_PlayableManager.GetOnPlayerPlayableChange().Insert(OnPlayerPlayableChanged);
		m_PlayableManager.GetOnFactionChange().Insert(OnFactionChanged);

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.AddActionListener("InstantVote", EActionTrigger.DOWN, OnVoteYes);
			im.AddActionListener("InstantVoteAbstain", EActionTrigger.DOWN, OnVoteNo);
		}
		GetGame().GetCallqueue().CallLater(InstantVotingContextUpdate, 0, true);
	}

	// --------------------------------------------------------------------------------------------
	// Unsubscribe everything
	void Destroy()
	{
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnGroupReadyChanged().Remove(OnGroupReadyChanged);
			m_PlayableManager.GetOnPlayerPlayableChange().Remove(OnPlayerPlayableChanged);
			m_PlayableManager.GetOnFactionChange().Remove(OnFactionChanged);
		}

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("InstantVote", EActionTrigger.DOWN, OnVoteYes);
			im.RemoveActionListener("InstantVoteAbstain", EActionTrigger.DOWN, OnVoteNo);
		}
		GetGame().GetCallqueue().Remove(InstantVotingContextUpdate);

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}

	// --------------------------------------------------------------------------------------------
	// Full rebuild — recreate all faction/group rows using only TextWidgets
	void Rebuild()
	{
		if (!m_PlayableManager)
			return;

		SCR_WidgetHelper.RemoveAllChildren(m_wFactionsList);
		m_mGroupLabels.Clear();
		m_mGroupLeaders.Clear();

		Print("[PS_SquadsFreezeTime] Rebuild started, player=" + m_iLocalPlayerId);

		// Collect factions → groups
		ref map<FactionKey, ref array<int>> factionGroups = new map<FactionKey, ref array<int>>();
		ref map<int, FactionKey> groupFaction = new map<int, FactionKey>();
		ref map<int, int> groupCallsign = new map<int, int>();

		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		Print("[PS_SquadsFreezeTime] playables count=" + playables.Count());

		foreach (PS_PlayableContainer playable : playables)
		{
			int playerId = m_PlayableManager.GetPlayerByPlayable(playable.GetRplId());
			if (playerId <= 0)
				continue;

			SCR_AIGroup group = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetRplId());
			if (!group)
				continue;

			int groupId = group.GetGroupID();
			FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(playerId);
			if (factionKey == "")
				continue;

			if (!groupFaction.Contains(groupId))
			{
				groupFaction[groupId] = factionKey;
				groupCallsign[groupId] = m_PlayableManager.GetGroupCallsignByPlayable(playable.GetRplId());
			}

			if (!factionGroups.Contains(factionKey))
			{
				factionGroups.Insert(factionKey, new array<int>());
			}

			array<int> groups = factionGroups[factionKey];
			if (!groups.Contains(groupId))
				groups.Insert(groupId);

			if (!m_mGroupLeaders.Contains(groupId))
				m_mGroupLeaders[groupId] = playerId;
		}

		Print("[PS_SquadsFreezeTime] factions=" + factionGroups.Count());

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		Color clearColor = Color.FromInt(0x00000000);

		foreach (FactionKey fk, array<int> groupIds : factionGroups)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(fk));

			// ── Faction header: flag image + faction name on same line ──
			HorizontalLayoutWidget factionHeader = HorizontalLayoutWidget.Cast(workspace.CreateWidget(WidgetType.HorizontalLayoutWidgetTypeID, WidgetFlags.VISIBLE, clearColor, 0, m_wFactionsList));
			OverlaySlot.SetHorizontalAlign(factionHeader, LayoutHorizontalAlign.Left);
			OverlaySlot.SetVerticalAlign(factionHeader, LayoutVerticalAlign.Center);

			TextWidget factionName = CreateTextWidget(factionHeader);
			ImageWidget factionFlag = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.STRETCH, clearColor, 0, factionHeader));
			if (scrFaction)
			{
				ResourceName flagTex = scrFaction.GetFactionFlag();
				if (flagTex == "")
				{
					UIInfo uiInfo = scrFaction.GetUIInfo();
					if (uiInfo)
						flagTex = uiInfo.GetIconPath();
				}
				if (flagTex != "")
				{
					factionFlag.LoadImageTexture(0, flagTex);
					Print("[PS_SquadsFreezeTime] Loaded flag " + flagTex + " for " + fk);
				}
				else
				{
					Print("[PS_SquadsFreezeTime] WARNING: No flag for " + fk);
				}
				factionFlag.SetColor(Color.White);
				factionFlag.SetSize(42, 24);
				factionFlag.SetVisible(true);
				factionName.SetText(scrFaction.GetFactionName());
				factionName.SetColor(scrFaction.GetFactionColor());
			}
			else
			{
				factionFlag.SetVisible(false);
				factionName.SetText(fk);
				factionName.SetColor(Color.White);
			}
			factionName.SetExactFontSize(20);

			Print("[PS_SquadsFreezeTime] faction=" + fk + " groups=" + groupIds.Count());

			// ── Group rows: plain TextWidgets directly in VerticalLayout ──
			foreach (int gid : groupIds)
			{
				// Row layout: HorizontalLayout with label
				Widget rowLayout = workspace.CreateWidget(WidgetType.HorizontalLayoutWidgetTypeID, WidgetFlags.VISIBLE, clearColor, 0, m_wFactionsList);
				OverlaySlot.SetHorizontalAlign(rowLayout, LayoutHorizontalAlign.Stretch);

				// Group label (name + leader)
				TextWidget groupLabel = CreateTextWidget(rowLayout);
				groupLabel.SetColor(Color.White);
				groupLabel.SetExactFontSize(16);

				m_mGroupLabels[gid] = groupLabel;
				UpdateGroupRow(gid, groupCallsign[gid], scrFaction);
			}
		}

		// Leader status
		m_bLocalIsGroupLeader = m_PlayableManager.IsPlayerGroupLeader(m_iLocalPlayerId);
		Print("[PS_SquadsFreezeTime] isLeader=" + m_bLocalIsGroupLeader);

		// Check if all factions are ready → auto end freeze time
		CheckAllFactionsReady();
	}

	// --------------------------------------------------------------------------------------------
	// Check if every faction's groups are all ready — if so, end freeze time (like /fte)
	protected void CheckAllFactionsReady()
	{
		if (!m_PlayableManager)
			return;

		// SAFETY: do not end freeze time when the list is empty (early init / no players slotted)
		int groupCount = m_mGroupLeaders.Count();
		if (groupCount == 0)
		{
			Print("[PS_SquadsFreezeTime] CheckAllFactionsReady: no groups to check (empty list)");
			return;
		}

		// Check ALL group ready states (not faction-level ready)
		foreach (int groupId, int leaderId : m_mGroupLeaders)
		{
			int ready = m_PlayableManager.GetGroupReady(groupId);
			Print("[PS_SquadsFreezeTime] CheckAll: group=" + groupId + " leader=" + leaderId + " ready=" + ready);
			if (ready != 1)
			{
				Print("[PS_SquadsFreezeTime] Freeze blocked: group " + groupId + " not ready (" + ready + ")");
				return; // at least one group not ready
			}
		}

		// All groups ready — end freeze time (same as /fte). Local one-shot: request only once per widget instance;
		// the server (PS_GameModeCoop.FreezeTimerEnd) is also guarded, so even with several clients requesting, the
		// freeze ends and the notification fires exactly once.
		if (m_bFreezeEndRequested)
			return;
		Print("[PS_SquadsFreezeTime] ALL " + groupCount + " groups ready! Calling FreezeTimerEnd.");
		PS_PlayableControllerComponent ctrl = PS_PlayableControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(PS_PlayableControllerComponent));
		if (ctrl)
		{
			m_bFreezeEndRequested = true;
			ctrl.FreezeTimerEnd();
			Print("[PS_SquadsFreezeTime] FreezeTimerEnd RPC sent.");
		}
		else
			Print("[PS_SquadsFreezeTime] ERROR: PS_PlayableControllerComponent not found for FreezeTimerEnd!");
	}

	// --------------------------------------------------------------------------------------------
	// Update a single group row
	void UpdateGroupRow(int groupId, int callsign, SCR_Faction faction)
	{
		// Show PodvalLobby nickname [ClanTag]Nickname (markup stripped)
		string leaderName = "";
		int leaderId;
		if (m_mGroupLeaders.Find(groupId, leaderId) && leaderId > 0)
		{
			string rawName = m_PlayableManager.GetPlayerName(leaderId);
			leaderName = PS_FactionReadyWidgetComponent.StripMarkup(rawName);
			if (leaderName == "")
			{
				PlayerManager pm = GetGame().GetPlayerManager();
				if (pm)
					leaderName = pm.GetPlayerName(leaderId);
			}
		}

		int ready = m_PlayableManager.GetGroupReady(groupId);

		TextWidget nameLabel = m_mGroupLabels[groupId];
		if (nameLabel)
		{
			string displayText = "";
			if (leaderName != "")
				displayText += leaderName;
			else
				displayText += "(empty)";
			nameLabel.SetText(displayText);
			// Green when ready, white when not ready
			if (ready == 1)
				nameLabel.SetColor(Color.FromInt(0xFF55DD55));
			else
				nameLabel.SetColor(Color.White);
			Print("[PS_SquadsFreezeTime] group=" + groupId + " text='" + displayText + "' ready=" + ready);
		}


	}

	// --------------------------------------------------------------------------------------------
	// Event handlers
	void OnGroupReadyChanged(int groupId, int readyValue)
	{
		Print("[PS_SquadsFreezeTime] OnGroupReadyChanged group=" + groupId + " ready=" + readyValue);
		SCR_Faction faction = null;
		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup group;
		if (groupsMgr)
			group = SCR_AIGroup.Cast(groupsMgr.FindGroup(groupId));
		if (group)
			faction = SCR_Faction.Cast(group.GetFaction());

		int callsign = -1;
		if (group)
			callsign = group.GetCallsignNum();

		UpdateGroupRow(groupId, callsign, faction);

		// Check if all factions are now ready
		CheckAllFactionsReady();
	}

	void OnPlayerPlayableChanged(int playerId, RplId playableId)
	{
		Rebuild();
	}

	void OnFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		Rebuild();
	}

	void OnVoteYes()
	{
		Print("[PS_SquadsFreezeTime] OnVoteYes called");
		m_bLocalIsGroupLeader = m_PlayableManager.IsPlayerGroupLeader(m_iLocalPlayerId);
		if (!m_bLocalIsGroupLeader)
		{
			Print("[PS_SquadsFreezeTime] Not a group leader, ignoring vote");
			return;
		}

		int groupId = GetLocalPlayerGroupId();
		Print("[PS_SquadsFreezeTime] Vote YES, groupId=" + groupId);
		if (groupId < 0)
			return;

		PS_PlayableControllerComponent ctrl = PS_PlayableControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(PS_PlayableControllerComponent));
		if (ctrl)
		{
			ctrl.SetGroupReady(groupId, 1);
			Print("[PS_SquadsFreezeTime] SetGroupReady(1) called, ctrl valid");
		}
		else
			Print("[PS_SquadsFreezeTime] ERROR: PS_PlayableControllerComponent not found!");
	}

	void OnVoteNo()
	{
		Print("[PS_SquadsFreezeTime] OnVoteNo called");
		m_bLocalIsGroupLeader = m_PlayableManager.IsPlayerGroupLeader(m_iLocalPlayerId);
		if (!m_bLocalIsGroupLeader)
		{
			Print("[PS_SquadsFreezeTime] Not a group leader, ignoring vote");
			return;
		}

		int groupId = GetLocalPlayerGroupId();
		Print("[PS_SquadsFreezeTime] Vote NO, groupId=" + groupId);
		if (groupId < 0)
			return;

		PS_PlayableControllerComponent ctrl = PS_PlayableControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(PS_PlayableControllerComponent));
		if (ctrl)
		{
			ctrl.SetGroupReady(groupId, 0);
			Print("[PS_SquadsFreezeTime] SetGroupReady(0) called, ctrl valid");
		}
		else
			Print("[PS_SquadsFreezeTime] ERROR: PS_PlayableControllerComponent not found!");
	}

	// --------------------------------------------------------------------------------------------
	protected void InstantVotingContextUpdate()
	{
		GetGame().GetInputManager().ActivateContext("InstantVotingContext");
	}

	// --------------------------------------------------------------------------------------------
	protected int GetLocalPlayerGroupId()
	{
		if (!m_PlayableManager)
			return -1;
		RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iLocalPlayerId);
		if (playableId == RplId.Invalid())
			return -1;
		SCR_AIGroup group = m_PlayableManager.GetPlayerGroupByPlayable(playableId);
		if (!group)
			return -1;
		return group.GetGroupID();
	}
}
