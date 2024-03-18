class PS_VoiceButton : PS_HideableButton
{
	static ref ScriptInvokerInt m_OnMuteStateChanged = new ScriptInvokerInt();
	
	// Const
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	// Cache global
	protected PlayerController m_PlayerController;
	protected PS_LobbyVoNComponent m_LobbyVoNComponent;
	protected int m_iCurrentPlayerId;
	
	// Vars
	protected int m_iPlayerId;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wButtonHandler.m_OnClicked.Insert(VoiceMuteSwitch);
		
		// Cache global
		m_PlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		IEntity entity = m_PlayerController.GetControlledEntity();
		if (!entity)
		{
			GetGame().GetCallqueue().CallLater(LateVoNInit, 0, true);
			UpdateState();
			return;
		}
		
		m_LobbyVoNComponent = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		m_LobbyVoNComponent.GetOnReceiveStart().Insert(OnReceiveStart);
		m_LobbyVoNComponent.GetOnReceiveEnd().Insert(OnReceiveEnd);
		
		m_OnMuteStateChanged.Insert(UpdateMute);
		
		UpdateState();
	}
	
	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyVoNComponent)
		{
			m_LobbyVoNComponent.GetOnReceiveStart().Remove(OnReceiveStart);
			m_LobbyVoNComponent.GetOnReceiveEnd().Remove(OnReceiveEnd);
		}
		
		m_OnMuteStateChanged.Remove(UpdateMute);
	}
	
	void LateVoNInit()
	{
		IEntity entity = m_PlayerController.GetControlledEntity();
		if (!entity)
			return;
		
		m_LobbyVoNComponent = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		if (!m_LobbyVoNComponent)
			return;
		
		m_LobbyVoNComponent.GetOnReceiveStart().Insert(OnReceiveStart);
		m_LobbyVoNComponent.GetOnReceiveEnd().Insert(OnReceiveEnd);
		
		GetGame().GetCallqueue().Remove(LateVoNInit);
		UpdateState();
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayerId = playerId;
		
		UpdateState();
	}
	
	void OnReceiveStart(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (playerId != m_iPlayerId)
			return;
		
		UpdateState();
	}
	
	void OnReceiveEnd(int playerId)
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
		
		// Check OUR VoN is WE listen this player?
		PermissionState mute = PermissionState.DISALLOWED;
		if (m_iPlayerId > 0) mute = m_PlayerController.GetPlayerMutedState(m_iPlayerId);
		if (m_LobbyVoNComponent && mute != PermissionState.DISALLOWED)
		{
			if (m_LobbyVoNComponent.IsPlayerSpeech(m_iPlayerId)) {
				
				if (m_LobbyVoNComponent.IsPlayerSpeechInChanel(m_iPlayerId)) m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNChannel");
				else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNDirect");
			}
			else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNIdle");
		} else m_wImage.LoadImageFromSet(0, m_sImageSetPS, "VoNDisabled");
		m_wButton.SetVisible(m_PlayerController.GetPlayerId() != m_iPlayerId && m_iPlayerId > 0);
	}
	
	void VoiceMuteSwitch(Widget button)
	{
		if (m_iCurrentPlayerId == m_iPlayerId) return;
		PermissionState mute = m_PlayerController.GetPlayerMutedState(m_iPlayerId);
		if (mute == PermissionState.ALLOWED) m_PlayerController.SetPlayerMutedState(m_iPlayerId, PermissionState.DISALLOWED);
		else m_PlayerController.SetPlayerMutedState(m_iPlayerId, PermissionState.ALLOWED);
		
		m_OnMuteStateChanged.Invoke(m_iPlayerId);
	}
}