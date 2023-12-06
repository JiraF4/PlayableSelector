// Widget displays info about voice chat room.
// Path: {C976E42779159507}UI/VoiceChat/VoiceRoomHeader.layout
// Part of voice chat list PS_VoiceChatList ({35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout)

class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	ImageWidget m_wJoinRoomImage;
	TextWidget m_wRoomName;
	string m_sRoomName;
	int m_iRoomId;
	SCR_Faction m_fFaction;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wJoinRoomImage = ImageWidget.Cast(w.FindAnyWidget("JoinRoomImage"));
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void SetRoomName(SCR_Faction faction, string roomName, int roomId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		m_sRoomName = roomName;
		m_fFaction = faction;
		m_iRoomId = roomId;
		
		string name = roomName;
		if (name != "" && name.IsDigitAt(0)) {
			int CallSign = roomName.ToInt();
			name = playableManager.GroupCallsignToGroupName(faction, CallSign);
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
	
	void AddOnClick()
	{
		m_OnClicked.Insert(JoinRoomButtonClicked);
	}
	
	void UpdateInfo()
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		
		if (VoNRoomsManager.GetPlayerRoom(playerId) == m_iRoomId) {
			m_wJoinRoomImage.SetVisible(false);
			return;
		}
		m_wJoinRoomImage.SetVisible(true);
		
		// Bad hardcoded staff here
		if (m_sRoomName == "#PS-VoNRoom_Command")
		{
			if (!playableManager.IsPlayerGroupLeader(playerId)) m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "Lock");
			else m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
			return;
		}
		
		m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
	}

	// -------------------- Buttons events --------------------
	void JoinRoomButtonClicked(SCR_ButtonBaseComponent joinRoomButton)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		// Bad hardcoded staff here
		if (m_sRoomName == "#PS-VoNRoom_Command")
		{
			if (!playableManager.IsPlayerGroupLeader(playerId))
				return;
		}
		
		FactionKey factionKey = "";
		if (m_fFaction) factionKey = m_fFaction.GetFactionKey();
		playableController.MoveToVoNRoom(playerId, factionKey, m_sRoomName);
	}
}