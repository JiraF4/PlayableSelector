class PS_VoiceButton : PS_HideableButton
{
	static ref ScriptInvokerInt m_OnMuteStateChanged = new ScriptInvokerInt();

	// Const
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";

	// Cache global
	protected PlayerController m_PlayerController;
	protected int m_iCurrentPlayerId;

	// Vars
	protected int m_iPlayerId;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wButtonHandler.m_OnClicked.Insert(VoiceMuteSwitch);

		// Cache global
		m_PlayerController = GetGame().GetPlayerController();
		if (!m_PlayerController)
			return;
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();

		// Body-less: talking state is tracked statically on the modded SCR_VoNComponent (fed by the
		// per-player VoN proxies), not by a controlled body's PS_LobbyVoNComponent.
		SCR_VoNComponent.PS_GetOnTalkingChanged().Insert(OnTalkingChanged);
		m_OnMuteStateChanged.Insert(UpdateMute);

		UpdateState();
	}

	override void HandlerDeattached(Widget w)
	{
		SCR_VoNComponent.PS_GetOnTalkingChanged().Remove(OnTalkingChanged);
		m_OnMuteStateChanged.Remove(UpdateMute);
	}

	void SetPlayer(int playerId)
	{
		m_iPlayerId = playerId;

		UpdateState();
	}

	void OnTalkingChanged(int playerId, bool talking)
	{
		if (playerId != m_iPlayerId)
			return;

		UpdateState();
	}

	void UpdateMute(int playerId)
	{
		if (playerId != m_iPlayerId)
			return;

		UpdateState();
	}

	void UpdateState()
	{
		if (m_iPlayerId <= 0)
		{
			m_wImage.SetVisible(false);
			m_wButton.SetVisible(false);

			return;
		}
		m_wImage.SetVisible(true);

		// Check OUR VoN: are WE listening to this player?
		PermissionState mute = PermissionState.DISALLOWED;
		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		if (m_iPlayerId > 0) mute = socialComp.IsMuted(m_iPlayerId);
		if (mute != PermissionState.DISALLOWED)
		{
			if (SCR_VoNComponent.PS_IsTalking(m_iPlayerId))
				m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNChannel");
			else
				m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNIdle");
		} else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNDisabled");
		m_wButton.SetVisible(m_PlayerController.GetPlayerId() != m_iPlayerId && m_iPlayerId > 0);
	}

	void VoiceMuteSwitch(Widget button)
	{
		if (m_iCurrentPlayerId == m_iPlayerId) return;
		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		bool mute = socialComp.IsMuted(m_iPlayerId);
		socialComp.SetMuted(m_iPlayerId, !mute);

		m_OnMuteStateChanged.Invoke(m_iPlayerId);
	}
}
