class PS_PlayersList : ScriptedWidgetComponent
{
	// Const
	protected ResourceName m_sPlayerSelectorPrefab = "{B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout"; // Handler: PS_PlayerSelector
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PlayerManager m_PlayerManager;
	protected int m_iPlayerId;
	protected string m_sSearchText;
	
	// Widgets
	protected VerticalLayoutWidget m_wPlayersList;
	
	// Vars
	protected PS_CoopLobby m_CoopLobby;
	protected int m_iSelectedPlayer;
	
	ref map<int, PS_PlayerSelector> m_mPlayers = new map<int, PS_PlayerSelector>();
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Widgets
		m_wPlayersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("PlayersList"));
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_PlayerManager = GetGame().GetPlayerManager();
		if (!m_PlayerController)
			return;
		
		m_iPlayerId = m_PlayerController.GetPlayerId();
		
		// Events
		m_PlayableManager.GetOnPlayerConnected().Insert(AddPlayer);
		// The same invoker also fires on a name (re)bind (RPC_SetPlayerName re-invokes it), so one
		// subscription covers late joins and renames; the player id argument is unused by the sort itself.
		m_PlayableManager.GetOnPlayerConnected().Insert(OnPlayersOrderDirty);
	}
	
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnPlayerConnected().Remove(AddPlayer);
			m_PlayableManager.GetOnPlayerConnected().Remove(OnPlayersOrderDirty);
		}
	}
	
	void SetLobbyMenu(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
		m_iSelectedPlayer = m_CoopLobby.GetSelectedPlayer();
	}
	
	void InitPlayers()
	{
		array<int> outPlayers = {};
		m_PlayerManager.GetPlayers(outPlayers);
		
		foreach (int playerId : outPlayers)
		{
			AddPlayer(playerId);
		}
	}
	
	void AddPlayer(int playerId)
	{
		if (m_mPlayers.Contains(playerId))
			return;
		Widget playerSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sPlayerSelectorPrefab, m_wPlayersList);
		PS_PlayerSelector playerSelector = PS_PlayerSelector.Cast(playerSelectorRoot.FindHandler(PS_PlayerSelector));
		playerSelector.SetCoopLobby(m_CoopLobby);
		playerSelector.SetPlayersList(this);
		playerSelector.SetPlayer(playerId);
		m_mPlayers.Insert(playerId, playerSelector);
		playerSelector.SetSearchText(m_sSearchText);
		
		// A fresh row lands on the map in insertion order; sorting it right away keeps the list
		// correct no matter how the invoker sequences AddPlayer against OnPlayersOrderDirty.
		SortPlayers();
	}
	
	// Rows are reordered in place through their Z order, the way the vanilla scoreboard does it,
	// so a name change or a late join never recreates the list.
	void SortPlayers()
	{
		array<PS_PlayerSelector> rows = {};
		foreach (PS_PlayerSelector playerSelector : m_mPlayers)
		{
			rows.Insert(playerSelector);
		}
		
		int n = rows.Count();
		for (int i = 1; i < n; i++)
		{
			PS_PlayerSelector current = rows[i];
			int j = i - 1;
			while (j >= 0 && OrderedBefore(current, rows[j]))
			{
				rows[j + 1] = rows[j];
				j--;
			}
			rows[j + 1] = current;
		}
		
		for (int i = 0; i < n; i++)
		{
			Widget root = rows[i].GetRootWidget();
			if (root)
				root.SetZOrder(i);
		}
	}
	
	// Unit-tagged names first, then alphabetical ignoring case; identical names fall back to the
	// player id so every client reaches the same order. The tag rule runs on the markup-stripped
	// name, because "[" (ASCII 91) sorts after upper-case and before lower-case letters.
	protected static bool OrderedBefore(PS_PlayerSelector a, PS_PlayerSelector b)
	{
		string nameA = a.GetPlayerName();
		string nameB = b.GetPlayerName();
		
		bool unitA = PS_PlayersHelper.HasUnitTag(nameA);
		bool unitB = PS_PlayersHelper.HasUnitTag(nameB);
		if (unitA != unitB)
			return unitA;
		
		int order = nameA.Compare(nameB, false);
		if (order != 0)
			return order < 0;
		
		return a.GetPlayerId() < b.GetPlayerId();
	}
	
	// The invoker passes the player id of the connect/name event; the sort re-reads the whole map anyway.
	protected void OnPlayersOrderDirty(int playerId)
	{
		SortPlayers();
	}
	
	void SetSelectedPlayer(int playerId)
	{
		PS_PlayerSelector playerSelectorOld = m_mPlayers.Get(m_iSelectedPlayer);
		PS_PlayerSelector playerSelectorNew = m_mPlayers.Get(playerId);
		if (playerSelectorOld)
			playerSelectorOld.Deselect();
		if (playerSelectorNew)
			playerSelectorNew.Select();
		m_iSelectedPlayer = playerId;
	}
	
	void OnPlayerRemoved(int playerId)
	{
		m_mPlayers.Remove(playerId);
	}
	
	void SetSearchText(string searchText)
	{
		m_sSearchText = searchText;
		foreach (PS_PlayerSelector playerSelector : m_mPlayers)
		{
			playerSelector.SetSearchText(m_sSearchText);
		}
	}
}















