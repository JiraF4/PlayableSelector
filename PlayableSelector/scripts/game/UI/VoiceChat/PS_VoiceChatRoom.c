class PS_VoiceChatRoom : SCR_ScriptedWidgetComponent
{
	[Attribute("{086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout")]
	protected ResourceName m_sPlayerVoiceSelectorPrefab;
	
	// Global cached
	protected PlayerManager m_gPlayerManager;
	protected PS_PlayableManager m_gPlayableManager;
	protected PS_VoNRoomsManager m_gVoNRoomsManager;
	
	// Local
	int m_iRoomId;
	PS_VoiceRoomHeader m_hRoomHandler;
	VerticalLayoutWidget m_wPlayersVerticalLayout;
	ref map<int, PS_PlayerVoiceSelector> m_mPlayers = new map<int, PS_PlayerVoiceSelector>;
	
	int m_iSelectedPlayer = -1;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		if (!GetGame().InPlayMode())
			return;
		
		// global
		m_gPlayerManager   = GetGame().GetPlayerManager();
		m_gPlayableManager = PS_PlayableManager.GetInstance();
		m_gVoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		
		// local
		m_hRoomHandler = PS_VoiceRoomHeader.Cast(w.FindAnyWidget("VoiceRoomHeader").FindHandler(PS_VoiceRoomHeader));
		m_wPlayersVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("PlayersVerticalLayout"));
	}
	
	void UpdateInfo()
	{
		m_hRoomHandler.UpdateInfo();
		foreach (int playerId, PS_PlayerVoiceSelector playerVoiceSelector : m_mPlayers)
		{
			playerVoiceSelector.UpdateInfo();
		}
	}
	
	void SetRoomId(int roomId)
	{
		m_iRoomId = roomId;
		
		string roomName = m_gVoNRoomsManager.GetRoomName(roomId);
		FactionKey factionKey = "";
		if (roomName == "") roomName = "Room not registered on server";
		else {
			array<string> outTokens = new array<string>();
			roomName.Split("|", outTokens, false);
			factionKey = outTokens[0];
			roomName = outTokens[1];
		}
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		m_hRoomHandler.SetRoomName(faction, roomName, roomId);
	}
	
	void AddPlayer(int playerId)
	{
		if (m_mPlayers.Contains(playerId))
			return;
		
		Widget playerSelector = GetGame().GetWorkspace().CreateWidgets(m_sPlayerVoiceSelectorPrefab);
		PS_PlayerVoiceSelector handler = PS_PlayerVoiceSelector.Cast(playerSelector.FindHandler(PS_PlayerVoiceSelector));
		
		m_mPlayers[playerId] = handler;
		
		handler.SetPlayer(playerId);
		m_wPlayersVerticalLayout.AddChild(playerSelector);
	}
	
	void RemovePlayer(int playerId)
	{
		if (!m_mPlayers.Contains(playerId)) return;
		
		PS_PlayerVoiceSelector handler = m_mPlayers[playerId];
		handler.GetRootWidget().RemoveFromHierarchy();
		m_mPlayers.Remove(playerId);
	}
	
	void SetSelectedPlayer(int playerId)
	{
		PS_PlayerVoiceSelector playerSelectorOld = m_mPlayers.Get(m_iSelectedPlayer);
		PS_PlayerVoiceSelector playerSelectorNew = m_mPlayers.Get(playerId);
		if (playerSelectorOld)
			playerSelectorOld.Deselect();
		if (playerSelectorNew)
			playerSelectorNew.Select();
		m_iSelectedPlayer = playerId;
	}
}







