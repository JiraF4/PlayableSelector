// Widget displays info about voice channels in voice chat.
// Path: {35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout

class PS_VoiceChatList : SCR_ScriptedWidgetComponent
{
	[Attribute("{E705A59E59B577F2}UI/VoiceChat/VoiceChatRoom.layout")]
	protected ResourceName m_sVoiceChatRoomPrefab;

	// Global cached
	protected PlayerManager m_gPlayerManager;
	protected PS_PlayableManager m_gPlayableManager;
	protected PS_VoNRoomsManager m_gVoNRoomsManager;
	protected PlayerController m_pPlayerController;
	protected int m_iPlayerId;
	protected string m_sPublicChannelKey;

	// Local
	// List of voice channels
	protected VerticalLayoutWidget m_wRoomsList;
	protected ref map<string, PS_VoiceChatRoom> m_wRooms = new map<string, PS_VoiceChatRoom>;
	protected FactionKey m_sCurrentFactionKey;
	protected bool m_bFactionViewSelected;
	protected bool m_bRebuildQueued;

	// -------------------- Handler events --------------------
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;

		super.HandlerAttached(w);

		// global
		m_gPlayerManager   = GetGame().GetPlayerManager();
		m_gPlayableManager = PS_PlayableManager.GetInstance();
		m_gVoNRoomsManager = PS_VoNRoomsManager.GetInstance();

		// local
		m_wRoomsList = VerticalLayoutWidget.Cast(w.FindAnyWidget("RoomsList"));

		if (m_gVoNRoomsManager)
		{
			m_gVoNRoomsManager.m_eOnRoomChanged.Insert(MovePlayer);
			m_gVoNRoomsManager.m_eOnPlayerParkedChanged.Insert(OnPlayerParkedChanged);
			m_gVoNRoomsManager.m_eOnVoNSnapshotReady.Insert(OnVoNSnapshotReady);
		}

		QueueRebuild();

		GetGame().GetCallqueue().CallLater(UpdateInfo, 100, true);
	}

	// FIX (TIMER LEAK + VME): HandlerDeattached fires when the widget is removed from the
	// hierarchy (menu close, server restart). All cleanup must happen HERE, not in the
	// destructor (~PS_VoiceChatList), because the GC may collect the object AFTER the
	// game context and singletons (GetGame(), m_gVoNRoomsManager) have already been
	// destroyed, causing VME crashes on m_eOnRoomChanged.Remove(MovePlayer).
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(UpdateInfo);
			GetGame().GetCallqueue().Remove(Rebuild);
		}
		m_bRebuildQueued = false;
		m_wRoomsList = null;

		if (m_gVoNRoomsManager)
		{
			m_gVoNRoomsManager.m_eOnRoomChanged.Remove(MovePlayer);
			m_gVoNRoomsManager.m_eOnPlayerParkedChanged.Remove(OnPlayerParkedChanged);
			m_gVoNRoomsManager.m_eOnVoNSnapshotReady.Remove(OnVoNSnapshotReady);
		}
	}

	// -------------------- Update content functions --------------------
	void Clear()
	{
		SCR_WidgetHelper.RemoveAllChildren(m_wRoomsList);
		m_wRooms.Clear();
	}

	// Coalesce assignment bursts; detaching cancels the pending callback.
	protected void QueueRebuild()
	{
		if (m_bRebuildQueued || !m_wRoomsList)
			return;

		m_bRebuildQueued = true;
		GetGame().GetCallqueue().CallLater(Rebuild, 0, false);
	}

	void Rebuild()
	{
		m_bRebuildQueued = false;
		if (!m_wRoomsList)
			return;

		if (!m_gVoNRoomsManager)
		{
			m_gVoNRoomsManager = PS_VoNRoomsManager.GetInstance();
			if (m_gVoNRoomsManager)
			{
				m_gVoNRoomsManager.m_eOnRoomChanged.Insert(MovePlayer);
				m_gVoNRoomsManager.m_eOnPlayerParkedChanged.Insert(OnPlayerParkedChanged);
				m_gVoNRoomsManager.m_eOnVoNSnapshotReady.Insert(OnVoNSnapshotReady);
			}
		}
		if (!m_gPlayableManager)
			m_gPlayableManager = PS_PlayableManager.GetInstance();
		m_pPlayerController = GetGame().GetPlayerController();
		if (!m_pPlayerController || !m_gPlayableManager || !m_gVoNRoomsManager)
			return;

		int playerId = m_pPlayerController.GetPlayerId();
		if (playerId <= 0)
			return;

		m_iPlayerId = playerId;
		if (!m_bFactionViewSelected)
			m_sCurrentFactionKey = m_gPlayableManager.GetPlayerFactionKey(m_iPlayerId);
		m_sPublicChannelKey = m_gVoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Public" + m_iPlayerId.ToString());
		Clear();

		// Create initial list of visible channels
		array<string> visibleRooms = new array<string>();
		GetVisibleRooms(visibleRooms);
		foreach (string channelKey : visibleRooms)
		{
			CreateRoom(channelKey);
		}

		UpdateInfo();
	}

	void CreateRoomIfNeed(string channelKey)
	{
		if (m_gVoNRoomsManager.IsPublicRoom(channelKey))
			CreateRoom(channelKey);
	}

	void CreateRoom(string channelKey)
	{
		if (channelKey == "" || !m_gVoNRoomsManager || m_wRooms.Contains(channelKey))
			return;
		Widget roomWidget = GetGame().GetWorkspace().CreateWidgets(m_sVoiceChatRoomPrefab);
		if (!roomWidget)
			return;
		PS_VoiceChatRoom voiceChatRoom = PS_VoiceChatRoom.Cast(roomWidget.FindHandler(PS_VoiceChatRoom));
		if (!voiceChatRoom || !voiceChatRoom.SetChannelKey(channelKey))
		{
			roomWidget.RemoveFromHierarchy();
			return;
		}

		array<int> playersInRoom = new array<int>();
		m_gVoNRoomsManager.GetPlayersInRoom(playersInRoom, channelKey);
		foreach (int playerId : playersInRoom)
		{
			voiceChatRoom.AddPlayer(playerId);
		}

		m_wRoomsList.AddChild(roomWidget);
		m_wRooms[channelKey] = voiceChatRoom;
	}

	void RemoveRoomIfNeed(string channelKey)
	{
		if (m_gVoNRoomsManager.IsGlobalRoom(channelKey)) return;
		if (m_gVoNRoomsManager.IsLocalRoom(channelKey)) return;

		// current player
		if (m_gVoNRoomsManager.IsPublicRoom(channelKey))
		{
			if (m_sPublicChannelKey != channelKey)
			{
				array<int> playersInRoom = new array<int>();
				m_gVoNRoomsManager.GetPlayersInRoom(playersInRoom, channelKey);
				if (playersInRoom.IsEmpty())
					RemoveRoom(channelKey);
			}
		} else {
			if (!m_gVoNRoomsManager.IsFactionRoom(channelKey, m_sCurrentFactionKey) && m_gVoNRoomsManager.GetPlayerChannel(m_iPlayerId) != channelKey)
			{
				RemoveRoom(channelKey);
			}
		}
	}

	void RemoveRoom(string channelKey)
	{
		if (!m_wRooms.Contains(channelKey)) return;
		PS_VoiceChatRoom voiceChatRoom = m_wRooms[channelKey];
		voiceChatRoom.GetRootWidget().RemoveFromHierarchy();
		m_wRooms.Remove(channelKey);
	}

	void SwitchFaction(FactionKey factionKey)
	{
		m_bFactionViewSelected = true;
		if (m_sCurrentFactionKey == factionKey) return;
		m_sCurrentFactionKey = factionKey;
		QueueRebuild();
	}

	FactionKey GetFactionKey()
	{
		return m_sCurrentFactionKey;
	}

	void MovePlayer(int playerId, string channelKey, string oldChannelKey)
	{
		QueueRebuild();
	}

	void OnPlayerParkedChanged(int playerId, bool parked)
	{
		QueueRebuild();
	}

	void OnVoNSnapshotReady()
	{
		QueueRebuild();
	}

	void UpdateInfo()
	{
		if (!m_wRoomsList)
			return;
		if (!m_gVoNRoomsManager || !m_gPlayableManager || m_iPlayerId <= 0)
		{
			QueueRebuild();
			return;
		}

		foreach (string channelKey, PS_VoiceChatRoom voiceChatRoom : m_wRooms)
		{
			voiceChatRoom.UpdateInfo();
		}
	}

	// ----- Actions -----
	protected void Action_PlayerClick(SCR_ButtonBaseComponent playerSelector)
	{
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
	}

	// -------------------- Extra lobby functions --------------------
	void GetVisibleRooms(out array<string> outRoomsArray)
	{
		// global
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		if (!gameMode || !playerManager || !playableManager || !VoNRoomsManager || !currentPlayerController)
			return;
		int currentPlayerId = currentPlayerController.GetPlayerId();
		if (currentPlayerId <= 0)
			return;
		SCR_EGameModeState gameState = gameMode.GetState();

		// current player
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		FactionKey currentPlayerFactionKey = m_sCurrentFactionKey;

		if (gameState != SCR_EGameModeState.BRIEFING)
		{
			// Local channel if you want some privacy
			string localRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Local" + currentPlayerId.ToString());
			outRoomsArray.Insert(localRoom);

			// Global channel
			string globalRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Global");
			outRoomsArray.Insert(globalRoom);

			if (currentPlayerFactionKey != "")
			{
				// Faction channel
				string factionRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Faction");
				outRoomsArray.Insert(factionRoom);

				// Channel for commanders
				string commandRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Command");
				outRoomsArray.Insert(commandRoom);
			}

			// Channel for each group
			array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
			for (int i = 0; i < playables.Count(); i++) {
				PS_PlayableContainer playable = playables[i];
				SCR_Faction faction = playable.GetFaction();
				FactionKey factionKey = faction.GetFactionKey();

				if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip

				// Group VoN channel is keyed by the unique group id below (GetGroupVonRoomName), not the colliding callsign num.
				string groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, playableManager.GetGroupVonRoomName(playable.GetRplId()));
				if (!outRoomsArray.Contains(groupRoom))
					outRoomsArray.Insert(groupRoom);
			}

			// Player public channel
			string publicRoom = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Public" + currentPlayerId.ToString());
			outRoomsArray.Insert(publicRoom);

			// Other players public channels (lazy - only the ones someone is actually in)
			array<string> playersPublicRooms = new array<string>();
			VoNRoomsManager.GetPlayersPublicChannels(playersPublicRooms);
			foreach (string channelKey : playersPublicRooms)
			{
				if (!outRoomsArray.Contains(channelKey))
					outRoomsArray.Insert(channelKey);
			}
		} else {
			// Briefing: the command (HQ) channel + EVERY group channel of your faction, so any member can
			// SEE and join other same-faction squads. Previously only group leaders saw all groups while
			// members saw only their own - now everyone on a faction sees the whole faction's squad channels.
			if (currentPlayerFactionKey != "")
			{
				// Channel for commanders (HQ)
				string commandRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, "#PS-VoNRoom_Command");
				outRoomsArray.Insert(commandRoom);

				// Channel for each group of our faction
				array<PS_PlayableContainer> playables = playableManager.GetPlayablesSorted();
				for (int i = 0; i < playables.Count(); i++) {
					PS_PlayableContainer playable = playables[i];
					FactionKey factionKey = playable.GetFactionKey();

					if (currentPlayerFactionKey != factionKey) continue; // not our faction, skip

					// Group VoN channel is keyed by the unique group id below (GetGroupVonRoomName), not the colliding callsign num.
					string groupRoom = VoNRoomsManager.GetRoomWithFaction(currentPlayerFactionKey, playableManager.GetGroupVonRoomName(playable.GetRplId()));
					if (!outRoomsArray.Contains(groupRoom))
						outRoomsArray.Insert(groupRoom);
				}
			}
		}

		// Current channel if something gone wrong
		string currentRoom = VoNRoomsManager.GetPlayerChannel(currentPlayerId);
		if (currentRoom != "" && !outRoomsArray.Contains(currentRoom))
			outRoomsArray.Insert(currentRoom);
	}

	void SetSelectedPlayer(int playerId)
	{
		foreach (PS_VoiceChatRoom room : m_wRooms)
		{
			room.SetSelectedPlayer(playerId);
		}
	}
};
