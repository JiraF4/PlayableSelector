// Widget displays info about voice chat room.
// Path: {C976E42779159507}UI/VoiceChat/VoiceRoomHeader.layout
// Part of voice chat list PS_VoiceChatList ({35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout)

class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	ImageWidget m_wJoinRoomImage;
	ImageWidget m_wFactionColor;
	TextWidget m_wRoomName;
	string m_sRoomName;
	string m_sChannelKey;
	SCR_Faction m_fFaction;
	
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;
		
		super.HandlerAttached(w);
		m_wJoinRoomImage = ImageWidget.Cast(w.FindAnyWidget("JoinRoomImage"));
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void SetRoomName(SCR_Faction faction, string roomName, string channelKey)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		m_sRoomName = roomName;
		m_fFaction = faction;
		m_sChannelKey = channelKey;
		
		if (m_fFaction)
		{
			m_wFactionColor.SetColor(m_fFaction.GetOutlineFactionColor());
		}
		
		string name = roomName;
		if (name != "" && name.IsDigitAt(0)) {
			int CallSign = roomName.ToInt();
			name = PS_GroupHelper.GroupCallsignToGroupName(faction, CallSign);
		}
		if (name.StartsWith("#PS-VoNRoom_Group")) {
			// Group channels are keyed by unique group id ("#PS-VoNRoom_Group<id>"); map back to the group's
			// callsign name (e.g. "Buran-11"). Checked before "_Global" - both start with "#PS-VoNRoom_G", but
			// StartsWith("#PS-VoNRoom_Group") matches only the group rooms.
			int groupId = name.Substring(17, name.Length() - 17).ToInt();
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			SCR_AIGroup group;
			if (groupsManager)
				group = groupsManager.FindGroup(groupId);
			if (group)
				name = PS_GroupHelper.GetGroupName(group);
		}
		if (name.StartsWith("#PS-VoNRoom_Local")) name = "#PS-VoNRoom_Local";
		if (name.StartsWith("#PS-VoNRoom_Public")) {
			int playerId = name.Substring(18, name.Length() - 18).ToInt();
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName == "") playerName = playerId.ToString();
			name = playerName + "'s #PS-VoNRoom_Public"; 
		}
		m_wRoomName.SetText(name);
	}
	
	string GetChannelKey()
	{
		return m_sChannelKey;
	}
	
	void AddOnClick()
	{
		m_OnClicked.Insert(JoinRoomButtonClicked);
	}
	
	void UpdateInfo()
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		int playerId = playerController.GetPlayerId();
		
		if (VoNRoomsManager.GetPlayerChannel(playerId) == m_sChannelKey) {
			m_wJoinRoomImage.SetVisible(false);
			return;
		}
		//m_wJoinRoomImage.SetVisible(true);
		
		// Bad hardcoded staff here
		if (m_sRoomName == "#PS-VoNRoom_Command")
		{
			PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (CanJoinCommand(playableManager, gamemode, playerId)) m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
			else m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "Lock");
			return;
		}

		m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
	}

	// HQ/Command is joinable by leaders, when public-command-briefing is on, OR by anyone during the
	// briefing phase (members default to their group channel but may opt into HQ). Outside briefing it
	// stays leader-only.
	protected bool CanJoinCommand(PS_PlayableManager playableManager, PS_GameModeCoop gamemode, int playerId)
	{
		if (!gamemode)
			return true;
		if (gamemode.GetState() == SCR_EGameModeState.BRIEFING)
			return true;
		return playableManager.IsPlayerGroupLeader(playerId) || gamemode.m_bPublicCommandBriefing;
	}

	// -------------------- Buttons events --------------------
	void JoinRoomButtonClicked(SCR_ButtonBaseComponent joinRoomButton)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		int playerId = playerController.GetPlayerId();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		if (m_sRoomName == "#PS-VoNRoom_Command")
		{
			PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (!CanJoinCommand(playableManager, gamemode, playerId))
				return;
		}

		FactionKey factionKey = "";
		if (m_fFaction) factionKey = m_fFaction.GetFactionKey();

		if (factionKey != "")
		{
			FactionKey playerFaction = playableManager.GetPlayerFactionKey(playerId);
			if (playerFaction != "" && playerFaction != factionKey)
			{
				Print(string.Format("[PS_VoN] VoiceRoomHeader: blocked cross-faction room join - player %1 (%2) tried to join room of faction '%3'", playerId, playerFaction, factionKey), LogLevel.WARNING);
				return;
			}
		}

		playableController.MoveToVoNRoom(playerId, factionKey, m_sRoomName);
	}
}