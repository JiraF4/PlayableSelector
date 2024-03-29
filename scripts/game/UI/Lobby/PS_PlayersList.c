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
		m_iPlayerId = m_PlayerController.GetPlayerId();
		
		// Events
		m_PlayableManager.GetOnPlayerConnected().Insert(AddPlayer);
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
		Widget playerSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sPlayerSelectorPrefab, m_wPlayersList);
		PS_PlayerSelector playerSelector = PS_PlayerSelector.Cast(playerSelectorRoot.FindHandler(PS_PlayerSelector));
		playerSelector.SetCoopLobby(m_CoopLobby);
		playerSelector.SetPlayersList(this);
		playerSelector.SetPlayer(playerId);
		m_mPlayers.Insert(playerId, playerSelector);
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
}















