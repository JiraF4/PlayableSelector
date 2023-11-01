class PS_VoiceButton : PS_HideableButton
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	protected int m_iPlayer;
	
	void SetPlayer(int playerId)
	{
		m_iPlayer = playerId;
	}
	
	void Update()
	{
		if (m_iPlayer <= 0)
		{
			m_wImage.SetVisible(false);
			m_wButton.SetVisible(false);	
		}
		m_wImage.SetVisible(true);
		
		// Check OUR VoN is WE listen this player?
		PlayerController playerController = GetGame().GetPlayerController();
		IEntity entity = playerController.GetControlledEntity();
		PermissionState mute = PermissionState.DISALLOWED;
		if (m_iPlayer > 0) mute = playerController.GetPlayerMutedState(m_iPlayer);
		PS_LobbyVoNComponent von;
		if (entity) von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		if (von && mute != PermissionState.DISALLOWED)
		{
			if (von.IsPlayerSpeech(m_iPlayer)) {
				
				if (von.IsPlayerSpeechInChanel(m_iPlayer)) m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNChannel");
				else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNDirect");
			}
			else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNIdle");
		} else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNDisabled");
		m_wButton.SetVisible(playerController.GetPlayerId() != m_iPlayer);
		
		m_wButtonHandler.m_OnClick.Insert(VoiceMuteSwitch);
	}
	
	void VoiceMuteSwitch(Widget button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController.GetPlayerId() == m_iPlayer) return;
		PermissionState mute = playerController.GetPlayerMutedState(m_iPlayer);
		if (mute == PermissionState.ALLOWED) playerController.SetPlayerMutedState(m_iPlayer, PermissionState.DISALLOWED);
		else playerController.SetPlayerMutedState(m_iPlayer, PermissionState.ALLOWED);
	}
}