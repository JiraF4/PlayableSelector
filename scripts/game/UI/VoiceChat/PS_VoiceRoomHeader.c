class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	ImageWidget m_wJoinRoomImage;
	TextWidget m_wRoomName;
	string m_sRoomName;
	SCR_Faction m_fFaction;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wJoinRoomImage = ImageWidget.Cast(w.FindAnyWidget("JoinRoomImage"));
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void SetRoomName(SCR_Faction faction, string roomName)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		m_sRoomName = roomName;
		m_fFaction = faction;
		
		string name = roomName;
		if (name.IsDigitAt(0)) {
			int CallSign = roomName.ToInt();
			name = playableManager.GroupCallsignToGroupName(faction, CallSign);
		}
		m_wRoomName.SetText(name);
	}
	
	void AddOnClick()
	{
		m_OnClicked.Insert(JoinRoomButtonClicked);
	}
	
	void UpdateInfo()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		
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