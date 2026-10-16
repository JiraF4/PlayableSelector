class PS_VoiceRoomsUI : SCR_ScriptedWidgetComponent
{
	[Attribute("{E705A59E59B577F2}UI/VoiceChat/VoiceChatRoom.layout")]
	protected ResourceName m_sVoiceChatRoomPrefab;
	protected VerticalLayoutWidget m_wRoomsList;
	protected PS_VoNRoomsManager m_Manager;
	protected PS_PlayableManager m_Playables;
	protected PS_GameModeCoop m_GameMode;
	protected ref map<int, PS_PlayerVoiceSelector> m_mPlayerRows = new map<int, PS_PlayerVoiceSelector>();
	protected FactionKey m_sCurrentFactionKey;
	protected bool m_bRebuildQueued;
	protected bool m_bAttached;
	protected bool m_bSpectator;
	protected int m_iSelectedPlayer = -1;
	protected static string m_sBeforeDeafen;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		if (!GetGame().InPlayMode())
			return;
		m_bAttached = true;
		m_wRoomsList = VerticalLayoutWidget.Cast(w.FindAnyWidget("RoomsList"));
		m_Manager = PS_VoNRoomsManager.GetInstance();
		m_Playables = PS_PlayableManager.GetInstance();
		m_GameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (m_GameMode)
		{
			m_GameMode.GetOnGameStateChange().Insert(OnGameStateChanged);
			m_GameMode.GetOnPlayerRoleChange().Insert(OnPlayerRoleChanged);
		}
		if (m_Manager)
		{
			m_Manager.m_OnPlayerChannelChanged.Insert(OnChannelChanged);
			m_Manager.GetOnPlayerParkedChanged().Insert(OnParkedChanged);
		}
		if (m_Playables)
		{
			m_sCurrentFactionKey = m_Playables.GetPlayerFactionKey(GetLocalPlayerId());
			m_Playables.m_eOnPlayerPlayableChange.Insert(OnPlayableChanged);
			m_Playables.GetOnFactionChange().Insert(OnFactionChanged);
			m_Playables.GetOnPlayableRegistered().Insert(OnPlayableRegistered);
			m_Playables.GetOnPlayableUnregistered().Insert(OnPlayableRegistered);
			m_Playables.GetOnPlayableChangeGroup().Insert(OnGroupChanged);
			m_Playables.GetOnPlayerConnected().Insert(OnPlayerConnected);
			m_Playables.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		}
		SCR_AIGroup.GetOnPlayerLeaderChanged().Insert(OnLeaderChanged);
		SCR_VoNComponent.PS_GetOnTalkingChanged().Insert(OnTalkingChanged);
		QueueRebuild();
	}

	// Remove callbacks while the menu still has its game context, before GC.
	override void HandlerDeattached(Widget w)
	{
		m_bAttached = false;
		GetGame().GetCallqueue().Remove(Rebuild);
		m_bRebuildQueued = false;
		if (m_GameMode)
		{
			m_GameMode.GetOnGameStateChange().Remove(OnGameStateChanged);
			m_GameMode.GetOnPlayerRoleChange().Remove(OnPlayerRoleChanged);
		}
		if (m_Manager)
		{
			m_Manager.m_OnPlayerChannelChanged.Remove(OnChannelChanged);
			m_Manager.GetOnPlayerParkedChanged().Remove(OnParkedChanged);
		}
		if (m_Playables)
		{
			m_Playables.m_eOnPlayerPlayableChange.Remove(OnPlayableChanged);
			m_Playables.GetOnFactionChange().Remove(OnFactionChanged);
			m_Playables.GetOnPlayableRegistered().Remove(OnPlayableRegistered);
			m_Playables.GetOnPlayableUnregistered().Remove(OnPlayableRegistered);
			m_Playables.GetOnPlayableChangeGroup().Remove(OnGroupChanged);
			m_Playables.GetOnPlayerConnected().Remove(OnPlayerConnected);
			m_Playables.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
		}
		SCR_AIGroup.GetOnPlayerLeaderChanged().Remove(OnLeaderChanged);
		SCR_VoNComponent.PS_GetOnTalkingChanged().Remove(OnTalkingChanged);
		m_mPlayerRows.Clear();
		super.HandlerDeattached(w);
	}

	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
			return controller.GetPlayerId();
		return 0;
	}

	void SwitchFaction(FactionKey factionKey)
	{
		m_sCurrentFactionKey = factionKey;
		QueueRebuild();
	}

	FactionKey GetFactionKey()
	{
		return m_sCurrentFactionKey;
	}

	void SetSpectatorContext(bool spectator)
	{
		m_bSpectator = spectator;
		QueueRebuild();
	}

	void SetSelectedPlayer(int playerId)
	{
		m_iSelectedPlayer = playerId;
		foreach (int id, PS_PlayerVoiceSelector row : m_mPlayerRows)
		{
			if (id == playerId)
				row.Select();
			else
				row.Deselect();
		}
	}

	protected void OnGameStateChanged(int state) { QueueRebuild(); }
	protected void OnPlayerRoleChanged(int playerId, EPlayerRole roles) { QueueRebuild(); }
	protected void OnChannelChanged(int playerId, string channelKey, string oldKey) { QueueRebuild(); }
	protected void OnParkedChanged(int playerId, bool parked) { QueueRebuild(); }
	protected void OnPlayableChanged(int playerId, RplId playableId) { QueueRebuild(); }
	protected void OnPlayableRegistered(RplId id, PS_PlayableContainer playable) { QueueRebuild(); }
	protected void OnGroupChanged(RplId id, PS_PlayableContainer playable, SCR_AIGroup group) { QueueRebuild(); }
	protected void OnLeaderChanged(int groupId, int playerId) { QueueRebuild(); }
	protected void OnPlayerConnected(int playerId) { QueueRebuild(); }
	protected void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout) { QueueRebuild(); }
	protected void OnFactionChanged(int playerId, FactionKey factionKey, FactionKey oldKey) { QueueRebuild(); }

	protected void OnTalkingChanged(int playerId, bool talking)
	{
		PS_PlayerVoiceSelector row = m_mPlayerRows.Get(playerId);
		if (row)
			row.SetTalking(talking);
	}

	void QueueRebuild()
	{
		if (!m_bAttached || m_bRebuildQueued)
			return;
		m_bRebuildQueued = true;
		GetGame().GetCallqueue().CallLater(Rebuild, 0, false);
	}

	protected void Rebuild()
	{
		m_bRebuildQueued = false;
		if (!m_bAttached || !m_wRoomsList || !m_Manager || !m_Playables)
			return;
		SCR_WidgetHelper.RemoveAllChildren(m_wRoomsList);
		m_mPlayerRows.Clear();
		int localId = GetLocalPlayerId();
		if (localId <= 0)
			return;
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		bool briefing = !m_bSpectator && gameMode && gameMode.GetState() == SCR_EGameModeState.BRIEFING;
		array<string> channels = {};
		string deafenKey = PS_VoNRoomsManager.GetDeafenChannelKey(localId);
		if (!briefing || m_Manager.GetPlayerChannel(localId) == deafenKey)
			channels.Insert(deafenKey);
		if (!briefing)
			channels.Insert(PS_VoNRoomsManager.GetLobbyChannelKey());
		if (!m_bSpectator && m_sCurrentFactionKey != "")
		{
			if (!briefing)
				channels.Insert(PS_VoNRoomsManager.GetFactionChannelKey(m_sCurrentFactionKey));
			channels.Insert(PS_VoNRoomsManager.GetCommandChannelKey(m_sCurrentFactionKey));
			foreach (PS_PlayableContainer playable : m_Playables.GetPlayablesSorted())
			{
				if (playable.GetFactionKey() != m_sCurrentFactionKey)
					continue;
				SCR_AIGroup group = m_Playables.GetPlayerGroupByPlayable(playable.GetRplId());
				if (!group)
					continue;
				string groupKey = PS_VoNRoomsManager.GetGroupChannelKey(group.GetGroupID(), m_sCurrentFactionKey);
				if (!channels.Contains(groupKey))
					channels.Insert(groupKey);
			}
		}
		if (!briefing)
		{
			channels.Insert(PS_VoNRoomsManager.GetPublicChannelKey(localId));
			array<string> publicKeys = {};
			foreach (int playerId, string key : m_Manager.m_mPlayerChannels)
			{
				if (PS_VoNRoomsManager.IsPublicChannelKey(key) && !publicKeys.Contains(key) && !channels.Contains(key) && !m_Manager.IsPlayerParked(playerId))
					publicKeys.Insert(key);
			}
			publicKeys.Sort();
			channels.InsertAll(publicKeys);
		}
		foreach (string channel : channels)
			CreateRoom(channel);
		if (m_iSelectedPlayer >= 0)
			SetSelectedPlayer(m_iSelectedPlayer);
	}

	protected void CreateRoom(string channelKey)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(m_sVoiceChatRoomPrefab, m_wRoomsList);
		if (!root)
			return;
		PS_VoiceChatRoom room = PS_VoiceChatRoom.Cast(root.FindHandler(PS_VoiceChatRoom));
		if (!room)
			return;
		room.InitRoom(this, channelKey);
		array<int> members = {};
		bool deafen = PS_VoNRoomsManager.IsDeafenChannelKey(channelKey);
		if (deafen)
		{
			int localId = GetLocalPlayerId();
			if (m_Manager.GetPlayerChannel(localId) == channelKey)
				members.Insert(localId);
		}
		else
		{
			foreach (int playerId, string key : m_Manager.m_mPlayerChannels)
			{
				if (key == channelKey)
					members.Insert(playerId);
			}
		}
		members.Sort();
		foreach (int member : members)
		{
			if (!deafen && m_Manager.IsPlayerParked(member))
				continue;
			PS_PlayerVoiceSelector row = room.AddPlayer(member);
			if (row)
				m_mPlayerRows.Set(member, row);
		}
	}

	void RequestJoin(string channelKey)
	{
		int localId = GetLocalPlayerId();
		string current = m_Manager.GetPlayerChannel(localId);
		if (PS_VoNRoomsManager.IsDeafenChannelKey(channelKey))
		{
			if (PS_VoNRoomsManager.IsDeafenChannelKey(current))
			{
				channelKey = m_sBeforeDeafen;
				if (channelKey == "" || !m_Manager.CanPlayerJoinChannel(localId, channelKey))
					channelKey = PS_VoNRoomsManager.GetLobbyChannelKey();
			}
			else
				m_sBeforeDeafen = current;
		}
		if (!m_Manager.CanPlayerJoinChannel(localId, channelKey))
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}
		PlayerController controller = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(controller.FindComponent(PS_PlayableControllerComponent));
		if (playableController)
			playableController.MoveToVoNRoom(localId, channelKey);
	}
}

class PS_VoiceChatRoom : SCR_ScriptedWidgetComponent
{
	[Attribute("{086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout")]
	protected ResourceName m_sPlayerVoiceSelectorPrefab;
	protected PS_VoiceRoomHeader m_Header;
	protected VerticalLayoutWidget m_wPlayersVerticalLayout;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Header = PS_VoiceRoomHeader.Cast(w.FindAnyWidget("VoiceRoomHeader").FindHandler(PS_VoiceRoomHeader));
		m_wPlayersVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("PlayersVerticalLayout"));
	}

	void InitRoom(PS_VoiceRoomsUI list, string channelKey)
	{
		m_Header.InitRoom(list, channelKey);
	}

	PS_PlayerVoiceSelector AddPlayer(int playerId)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(m_sPlayerVoiceSelectorPrefab, m_wPlayersVerticalLayout);
		if (!root)
			return null;
		PS_PlayerVoiceSelector row = PS_PlayerVoiceSelector.Cast(root.FindHandler(PS_PlayerVoiceSelector));
		if (row)
			row.SetPlayer(playerId);
		return row;
	}
}

class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	protected PS_VoiceRoomsUI m_List;
	protected string m_sChannelKey;
	protected ImageWidget m_wJoinRoomImage;
	protected ImageWidget m_wFactionColor;
	protected TextWidget m_wRoomName;
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wJoinRoomImage = ImageWidget.Cast(w.FindAnyWidget("JoinRoomImage"));
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
		m_OnClicked.Insert(OnHeaderClicked);
	}

	override void HandlerDeattached(Widget w)
	{
		m_OnClicked.Remove(OnHeaderClicked);
		m_List = null;
		super.HandlerDeattached(w);
	}

	void InitRoom(PS_VoiceRoomsUI list, string channelKey)
	{
		m_List = list;
		m_sChannelKey = channelKey;
		string name = "#PS-VoNRoom_Global";
		FactionKey factionKey = PS_VoNRoomsManager.GetChannelKeyFactionKey(channelKey);
		int groupId;
		string groupFaction;
		if (PS_VoNRoomsManager.ParseGroupChannelKey(channelKey, groupId, groupFaction))
		{
			factionKey = groupFaction;
			PS_PlayableManager playables = PS_PlayableManager.GetInstance();
			foreach (PS_PlayableContainer playable : playables.GetPlayablesSorted())
			{
				if (playable.GetFactionKey() != groupFaction)
					continue;
				SCR_AIGroup group = playables.GetPlayerGroupByPlayable(playable.GetRplId());
				if (group && group.GetGroupID() == groupId)
				{
					name = PS_GroupHelper.GetGroupName(group);
					break;
				}
			}
		}
		else if (PS_VoNRoomsManager.IsDeafenChannelKey(channelKey))
			name = "#PS-VoNRoom_Deafen";
		else if (PS_VoNRoomsManager.IsPublicChannelKey(channelKey))
		{
			int ownerId = PS_VoNRoomsManager.GetChannelKeyPlayerId(channelKey);
			string ownerName = PS_PlayableManager.GetInstance().GetPlayerName(ownerId);
			if (ownerName == "")
				ownerName = ownerId.ToString();
			name = WidgetManager.Translate("#PS-VoNRoom_Public", ownerName);
		}
		else if (channelKey == PS_VoNRoomsManager.GetCommandChannelKey(factionKey))
			name = "#PS-VoNRoom_Command";
		else if (channelKey == PS_VoNRoomsManager.GetFactionChannelKey(factionKey))
			name = "#PS-VoNRoom_Faction";
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		if (faction)
			m_wFactionColor.SetColor(faction.GetOutlineFactionColor());
		m_wRoomName.SetText(name);
		PS_VoNRoomsManager manager = PS_VoNRoomsManager.GetInstance();
		int localId = GetGame().GetPlayerController().GetPlayerId();
		bool joined = manager.GetPlayerChannel(localId) == channelKey;
		m_wJoinRoomImage.SetVisible(!joined);
		if (manager.CanPlayerJoinChannel(localId, channelKey))
			m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
		else
			m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "Lock");
	}

	protected void OnHeaderClicked(SCR_ButtonBaseComponent button)
	{
		if (m_List)
			m_List.RequestJoin(m_sChannelKey);
	}
}

class PS_PlayerVoiceSelector : SCR_ButtonBaseComponent
{
	protected int m_iPlayerId;


	ImageWidget m_wUnitIcon;
	ImageWidget m_wLeaderIcon;
	RichTextWidget m_wPlayerName;
	PS_VoiceButton m_wVoiceHideableButton;
	TextWidget m_wGroupName;
	ImageWidget m_wCharacterFactionColor;
	ImageWidget m_wImageCurrent;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wLeaderIcon = ImageWidget.Cast(w.FindAnyWidget("LeaderIcon"));
		m_wPlayerName = RichTextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wVoiceHideableButton = PS_VoiceButton.Cast(w.FindAnyWidget("VoiceHideableButton").FindHandler(PS_VoiceButton));
		m_wGroupName = TextWidget.Cast(w.FindAnyWidget("GroupName"));
		m_wImageCurrent = ImageWidget.Cast(w.FindAnyWidget("ImageCurrent"));
	}

	void SetPlayer(int playerId)
	{
		m_iPlayerId = playerId;
		m_wVoiceHideableButton.SetPlayer(playerId, false);
		UpdateInfo();
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	void UpdateInfo()
	{
		// global
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!playableManager)
			return;

		// player data
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayerId);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayerId);
		string playerName = playableManager.GetPlayerName(m_iPlayerId);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		int groupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);

		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();

		// update
		if (playerName != "") m_wPlayerName.SetText(playerName);
		m_wLeaderIcon.SetVisible(playableManager.IsPlayerGroupLeader(m_iPlayerId));
		if (faction) m_wCharacterFactionColor.SetColor(faction.GetFactionColor());
		else m_wCharacterFactionColor.SetColor(Color.FromInt(0xFF2c2c2c));

		int currentPlayerIdInt = currentPlayerId;
		PS_CoopLobby coopLobby = PS_CoopLobby.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (coopLobby)
			currentPlayerIdInt = coopLobby.GetSelectedPlayer();
		m_wImageCurrent.SetVisible(currentPlayerIdInt == m_iPlayerId);

		if (SCR_Global.IsAdmin(m_iPlayerId)) m_wPlayerName.SetColor(Color.FromInt(0xfff2a34b));
		else if (playerName == "") m_wPlayerName.SetColor(Color.FromInt(0xff999999));
		else m_wPlayerName.SetColor(Color.FromInt(0xffffffff));

		if (playableId != RplId.Invalid()) {
			PS_PlayableContainer playableComponent = playableManager.GetPlayableById(playableId);
			string groupName = PS_GroupHelper.GroupCallsignToGroupName(faction, groupCallSign);

			m_wUnitIcon.SetVisible(true);
			m_wGroupName.SetVisible(true);
			if (playableComponent)
				playableComponent.SetIconTo(m_wUnitIcon);
			m_wGroupName.SetText(groupName);
		}else{
			m_wUnitIcon.SetVisible(false);
			m_wGroupName.SetVisible(false);
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	// Context menu
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (button == 1)
		{
			OpenContext();
			return false;
		}
		if (button != 0)
			return false;

		return false;
	}

	void OpenContext()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return;

		string playerName = PS_PlayableManager.GetInstance().GetPlayerName(m_iPlayerId);
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(menu.GetRootWidget(), playerName);
		if (menu.IsInherited(PS_CoopLobby) && PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionPlayerSelect(m_iPlayerId);
			if (PS_PlayableManager.GetInstance().GetPlayerPin(m_iPlayerId))
				contextMenu.ActionUnpin(m_iPlayerId);
		}

		if (m_iPlayerId == GetGame().GetPlayerController().GetPlayerId())
		{
			if (PS_PlayersHelper.IsAdminOrServer())
			{
				contextMenu.ActionGetArmaId(m_iPlayerId);
			}
			return;
		}

		contextMenu.ActionDirectMessage(m_iPlayerId);


		// global
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();

		if (PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionGetArmaId(m_iPlayerId);
		}

		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		if (socialComp.IsMuted(m_iPlayerId))
			contextMenu.ActionUnmute(m_iPlayerId);
		else
			contextMenu.ActionMute(m_iPlayerId);
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionPlayerSelect(m_iPlayerId);

			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			RplId playableId = playableManager.GetPlayableByPlayerRemembered(m_iPlayerId);
			if (playableId != RplId.Invalid() && gameMode.GetState() == SCR_EGameModeState.GAME)
			{
				contextMenu.ActionRespawnInPlace(playableId, m_iPlayerId);
			}
		}

		int callerId = GetGame().GetPlayerController().GetPlayerId();
		string destination = GetVoiceKickDestination();
		if (PS_PlayersHelper.IsAdminOrServer() || VoNRoomsManager.CanPlayerVoiceKick(callerId, m_iPlayerId, destination))
			contextMenu.ActionVoiceKick(m_iPlayerId).Insert(OnActionVoiceKick);

		if (PS_PlayersHelper.IsAdminOrServer())
			contextMenu.ActionKick(m_iPlayerId);
	}

	// -------------------- Buttons events --------------------
	protected string GetVoiceKickDestination()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		FactionKey factionKey = playableManager.GetSlotFactionForPlayer(m_iPlayerId);
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode && gameMode.GetState() == SCR_EGameModeState.BRIEFING)
		{
			SCR_AIGroup group = playableManager.GetPlayerGroupByPlayable(playableManager.GetPlayableByPlayer(m_iPlayerId));
			if (group)
				return PS_VoNRoomsManager.GetGroupChannelKey(group.GetGroupID(), factionKey);
		}
		if (factionKey != "")
			return PS_VoNRoomsManager.GetFactionChannelKey(factionKey);
		return PS_VoNRoomsManager.GetLobbyChannelKey();
	}

	void OnActionVoiceKick(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
	{
		PlayerController controller = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(controller.FindComponent(PS_PlayableControllerComponent));
		if (playableController)
			playableController.MoveToVoNRoom(contextActionDataPlayer.GetPlayerId(), GetVoiceKickDestination());
	}

	void SetTalking(bool talking)
	{
		// PS_VoiceButton reads the same static talking state and preserves mute icons.
		if (m_wVoiceHideableButton)
			m_wVoiceHideableButton.UpdateState();
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	void Deselect()
	{
		m_wImageCurrent.SetVisible(false);
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	void Select()
	{
		m_wImageCurrent.SetVisible(true);
	}
}
