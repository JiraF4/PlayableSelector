class PS_KillList : ScriptedWidgetComponent
{
	protected ResourceName m_sKillEntryPrefab = "{C0A1B2D3E4F5A6B7}UI/Spectator/KillListEntry.layout";

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

		// Kills / Team Kills are numbered lists showing the VICTIM (squad + nickname); Killed by shows the
		// KILLER nickname without a number.
		if (isLocalKiller && !killInfo.m_bIsTeamKill)
		{
			m_iYourKillsCount++;
			AddKillEntry(m_wYourKillsList, m_aYourKillsWidgets, killInfo, killInfo.GetVictimDisplayName(), m_iYourKillsCount);
		}

		if (killInfo.m_bIsTeamKill && isLocalKiller)
		{
			m_iTeamKillsCount++;
			AddKillEntry(m_wTeamKillsList, m_aTeamKillsWidgets, killInfo, killInfo.GetVictimDisplayName(), m_iTeamKillsCount);
		}

		if (isLocalVictim && killInfo.m_iKillerPlayerId > 0)
		{
			m_iKilledYouCount++;
			AddKillEntry(m_wKilledYouList, m_aKilledYouWidgets, killInfo, killInfo.GetKillerDisplayName(), 0);
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
		// Fill the (stretched) list width; the row's FillWeight text needs a non-zero parent width to show.
		LayoutSlot.SetHorizontalAlign(entryWidget, LayoutHorizontalAlign.Stretch);
		// Plain TextWidget: real-server player names carry clan-tag rich-text markup (added by the Podval
		// mods); PS_KillInfo.GetVictim/KillerDisplayName strips it to a plain nickname, so a plain TextWidget
		// shows it cleanly - matching peer-tool behaviour. (A RichTextWidget would RENDER the tags bold/colored,
		// which is not wanted here.)
		TextWidget textWidget = TextWidget.Cast(entryWidget.FindAnyWidget("KillEntryText"));
		if (textWidget)
		{
			textWidget.SetText(killInfo.FormatLine(displayName, number));
			// Team kills stand out in red; everything else white.
			if (killInfo.m_bIsTeamKill)
				textWidget.SetColor(Color.FromRGBA(255, 90, 90, 255));
			else
				textWidget.SetColor(Color.White);
		}
		PrintFormat("[PS_KillList] AddKillEntry OK name=%1 line=%2 textWidgetNull=%3 childrenNow=%4",
			displayName, killInfo.FormatLine(displayName, number), !textWidget, widgetArray.Count() + 1);

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
