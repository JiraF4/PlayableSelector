class PS_VoiceRoomHeader : SCR_ButtonBaseComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	ImageWidget m_wJoinRoomImage;
	TextWidget m_wRoomName;
	string m_sRoomName;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wJoinRoomImage = ImageWidget.Cast(w.FindAnyWidget("JoinRoomImage"));
		m_wRoomName = TextWidget.Cast(w.FindAnyWidget("RoomName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void SetRoomName(string name)
	{
		m_wRoomName.SetText(name);
		m_sRoomName = name;
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
		if (m_sRoomName == "Command")
		{
			if (!playableManager.IsPlayerGroupLeader(playerId)) m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
			else m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSet, "VON_directspeech");
			return;
		}
		
		m_wJoinRoomImage.LoadImageFromSet(0, m_sImageSet, "VON_directspeech");
	}

	// -------------------- Buttons events --------------------
	void JoinRoomButtonClicked(SCR_ButtonBaseComponent joinRoomButton)
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		// Bad hardcoded staff here
		if (m_sRoomName == "Command")
		{
			if (!playableManager.IsPlayerGroupLeader(playerId))
				return;
		}
		
		playableController.MoveToVoNRoom(playerId, playableManager.GetPlayerFactionKey(playerId), m_sRoomName);
	}
}