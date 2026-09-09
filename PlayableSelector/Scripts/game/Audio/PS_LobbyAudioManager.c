// PS_LobbyAudioManager.c
// Client-side lobby world mute + earplugs (F2) for PlayableSelector.
// Mutes AudioSystem.SFX in PREGAME / SLOTSELECTION / BRIEFING so lobby chatter
// is not drowned by wind, vehicles and battlefield ambience. CUTSCENE stays
// unmuted (cutscene voice must be heard), GAME / DEBRIEFING / POSTGAME unmute.
// F2 pressed during a muted phase NEVER touches AudioSystem - it only flips
// the pending wish (m_bEarplugsPending), applied on the next unmuted phase.

class PS_LobbyAudioManager
{
	protected static ref PS_LobbyAudioManager s_Instance;

	//! World mute is currently holding SFX at 0. Source of truth - do NOT infer from GetMasterVolume().
	protected bool m_bLobbyMuted;
	//! Earplugs are actively applied right now (only meaningful when !m_bLobbyMuted).
	protected bool m_bEarplugsOn;
	//! Wish collected via F2 while muted. Materializes into m_bEarplugsOn on the next unmuted phase.
	protected bool m_bEarplugsPending;
	//! User's reference SFX level (AudioSettings VolumeSfx / 100). Restore target.
	protected float m_fDefaultSFX = 1.0;
	//! Computed quiet level = Default * (1 - Suppression / 100).
	protected float m_fEarplugsSFX = 0.2;

	protected bool m_bInitialized;
	protected InputManager m_InputManager;

	protected ref ScriptInvoker m_OnStateChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	//! Singleton accessor. Creates the instance on first use (any machine).
	static PS_LobbyAudioManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new PS_LobbyAudioManager();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Safe teardown, callable when no instance exists. Restores default volume if we hold a mute.
	static void DestroyInstance()
	{
		if (!s_Instance)
			return;

		s_Instance.Destroy();
		s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Lobby phases where world SFX must be muted. CUTSCENE is intentionally NOT muted.
	static bool IsLobbyMutedPhase(SCR_EGameModeState state)
	{
		switch (state)
		{
			case SCR_EGameModeState.PREGAME:
			case SCR_EGameModeState.SLOTSELECTION:
			case SCR_EGameModeState.BRIEFING:
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnStateChanged() { return m_OnStateChanged; }
	bool IsLobbyMuted() { return m_bLobbyMuted; }
	bool IsEarplugsOn() { return m_bEarplugsOn; }
	bool IsEarplugsPending() { return m_bEarplugsPending; }

	//------------------------------------------------------------------------------------------------
	//! Client-side init. Called once from PS_GameModeCoop.OnGameStart (non-dedicated only).
	void Init()
	{
		if (m_bInitialized)
			return;

		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		m_bInitialized = true;

		RefreshVolumes();

		GetGame().OnUserSettingsChangedInvoker().Insert(OnUserSettingsChanged);

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
			m_InputManager.AddActionListener("PS_ToggleEarplugs", EActionTrigger.DOWN, OnToggleKey);

		// Late-join sync: apply the current phase immediately (JIP during BRIEFING etc.).
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode)
			OnGamePhaseChanged(gameMode.GetState());
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (!m_bInitialized)
			return;

		m_bInitialized = false;

		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("PS_ToggleEarplugs", EActionTrigger.DOWN, OnToggleKey);
			m_InputManager = null;
		}

		GetGame().OnUserSettingsChangedInvoker().Remove(OnUserSettingsChanged);

		// Safety: never leave the session with SFX pinned to 0.
		if (m_bLobbyMuted)
		{
			m_bLobbyMuted = false;
			m_bEarplugsPending = false;
			if (m_bEarplugsOn)
				SetVolume(m_fEarplugsSFX);
			else
				SetVolume(m_fDefaultSFX);
		}

		m_bEarplugsOn = false;
	}

	//------------------------------------------------------------------------------------------------
	//! GameMode phase hook. Must be called AFTER super.OnGameStateChanged().
	void OnGamePhaseChanged(SCR_EGameModeState state)
	{
		if (!m_bInitialized)
			return;

		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (IsLobbyMutedPhase(state))
		{
			if (!m_bLobbyMuted)
			{
				m_bLobbyMuted = true;
				SetVolume(0.0);
				NotifyChanged();
			}
			return;
		}

		// Unmuted phase (CUTSCENE / GAME / DEBRIEFING / POSTGAME).
		bool wasMuted = m_bLobbyMuted;
		m_bLobbyMuted = false;

		// A wish collected in lobby materializes now.
		if (m_bEarplugsPending)
		{
			m_bEarplugsPending = false;
			m_bEarplugsOn = !m_bEarplugsOn;
			if (m_bEarplugsOn)
				SetVolume(m_fEarplugsSFX);
			else
				SetVolume(m_fDefaultSFX);

			NotifyChanged();
			return;
		}

		if (wasMuted)
		{
			if (m_bEarplugsOn)
				SetVolume(m_fEarplugsSFX);
			else
				SetVolume(m_fDefaultSFX);

			NotifyChanged();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! F2 handler. In muted phases only records the wish, AudioSystem is NOT touched.
	void ToggleEarplugs()
	{
		if (!m_bInitialized)
			return;

		if (m_bLobbyMuted)
		{
			m_bEarplugsPending = !m_bEarplugsPending;
			NotifyChanged();
			return;
		}

		m_bEarplugsOn = !m_bEarplugsOn;
		if (m_bEarplugsOn)
			SetVolume(m_fEarplugsSFX);
		else
			SetVolume(m_fDefaultSFX);

		NotifyChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Settings changed: re-read reference levels. Never SetVolume while the lobby mute holds,
	//! otherwise opening the settings menu would break the 0.0 mute (the "cossack wind" bug).
	void OnUserSettingsChanged()
	{
		if (!m_bInitialized)
			return;

		RefreshVolumes();

		if (m_bLobbyMuted)
			return;

		if (m_bEarplugsOn)
			SetVolume(m_fEarplugsSFX);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggleKey()
	{
		ToggleEarplugs();
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyChanged()
	{
		m_OnStateChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVolumes()
	{
		m_fDefaultSFX = FetchDefaultSFXVolume();
		m_fEarplugsSFX = FetchEarplugsVolume();
	}

	//------------------------------------------------------------------------------------------------
	protected float FetchDefaultSFXVolume()
	{
		float volume = AudioSystem.GetMasterVolume(AudioSystem.SFX);

		UserSettings engineSettings = GetGame().GetEngineUserSettings();
		if (engineSettings)
		{
			BaseContainer audioSettings = engineSettings.GetModule("AudioSettings");
			if (audioSettings)
			{
				audioSettings.Get("VolumeSfx", volume);
				volume *= 0.01;
			}
		}

		return volume;
	}

	//------------------------------------------------------------------------------------------------
	protected float FetchEarplugsVolume()
	{
		// Suppression 0..100. 80 means 20% of default loudness stays audible.
		int suppression = 80;

		UserSettings gameSettings = GetGame().GetGameUserSettings();
		if (gameSettings)
		{
			BaseContainer earplugSettings = gameSettings.GetModule("PS_EarplugSettings");
			if (earplugSettings)
				earplugSettings.Get("PS_Suppression", suppression);
		}

		suppression = Math.ClampInt(suppression, 0, 100);
		return m_fDefaultSFX * (1.0 - suppression * 0.01);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetVolume(float volume)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, volume);
	}
}
