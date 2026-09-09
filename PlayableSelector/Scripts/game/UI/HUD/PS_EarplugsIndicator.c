// PS_EarplugsIndicator.c
// HUD badge for the PlayableSelector lobby mute + earplugs (F2) state.
// Widget layout and placement are 1:1 with the WCS earplugs overlay
// (right-center Overlay0 50x50, single additive Image0, hidden by default).
// Registered on our own player controller (DefaultPlayerControllerMP_Coop.et),
// so no vanilla prefab is touched.
// States on the single image: hidden when sound is normal, dimmed while the
// lobby world mute holds, solid while earplugs are on or an F2 wish is queued.

class PS_EarplugsIndicator : SCR_InfoDisplay
{
	protected ImageWidget m_wImage;

	//------------------------------------------------------------------------------------------------
	override void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);

		m_wImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("Image0"));

		PS_LobbyAudioManager.GetInstance().GetOnStateChanged().Insert(UpdateState);
		UpdateState();
	}

	//------------------------------------------------------------------------------------------------
	override void OnStopDraw(IEntity owner)
	{
		super.OnStopDraw(owner);

		PS_LobbyAudioManager manager = PS_LobbyAudioManager.GetInstance();
		if (manager)
			manager.GetOnStateChanged().Remove(UpdateState);

		m_wImage = null;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateState()
	{
		if (!m_wImage)
			return;

		PS_LobbyAudioManager manager = PS_LobbyAudioManager.GetInstance();
		if (!manager)
		{
			m_wImage.SetOpacity(0);
			return;
		}

		if (manager.IsLobbyMuted())
		{
			// Queued F2 wish reads as solid, plain system mute as dimmed.
			if (manager.IsEarplugsPending())
				m_wImage.SetOpacity(1);
			else
				m_wImage.SetOpacity(0.45);

			return;
		}

		if (manager.IsEarplugsOn() || manager.IsEarplugsPending())
		{
			m_wImage.SetOpacity(1);
			return;
		}

		m_wImage.SetOpacity(0);
	}
}
