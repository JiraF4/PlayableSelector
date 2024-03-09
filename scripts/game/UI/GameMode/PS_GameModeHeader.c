class PS_GameModeHeader : ScriptedWidgetComponent
{
	ButtonWidget m_wButtonPreview;
	ButtonWidget m_wButtonLobby;
	ButtonWidget m_wButtonBriefing;
	ButtonWidget m_wButtonInGame;
	ButtonWidget m_wButtonDebriefing;
	
	ImageWidget m_wGameModeHeaderLine;
	
	TextWidget m_wTitleText;
	RichTextWidget m_wAuthorRichText;
	
	ImageWidget m_wWeatherImage;
	ImageWidget m_wTimeImage;
	
	PS_GameModeHeaderButton m_hButtonPreview;
	PS_GameModeHeaderButton m_hButtonLobby;
	PS_GameModeHeaderButton m_hButtonBriefing;
	PS_GameModeHeaderButton m_hButtonInGame;
	PS_GameModeHeaderButton m_hButtonDebriefing;
	
	PS_GameModeHeaderButton m_bButtonAdvance;
	ImageWidget m_wAdvanceImage;
	
	SCR_MissionHeader m_MissionHeader;
	TimeAndWeatherManagerEntity m_TimeAndWeatherManagerEntity;
	
	ref array<ResourceName> timeImages = {
		"{5E793A7B8DA84847}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midnight.edds",
		"{94B936AA2CE13242}UI/Textures/Editor/Attributes/Time/Attribute_Time_EarlyMorning.edds",
		"{FF96EE66370C0CBB}UI/Textures/Editor/Attributes/Time/Attribute_Time_Morning.edds",
		"{1F170BA4B611B7D6}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midday.edds",
		"{1F170BA4B611B7D6}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midday.edds",
		"{F63FE753B10DD0F0}UI/Textures/Editor/Attributes/Time/Attribute_Time_Afternoon.edds",
		"{59ABC4E5F1CC0755}UI/Textures/Editor/Attributes/Time/Attribute_Time_Evening.edds",
		"{5E793A7B8DA84847}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midnight.edds",
		"{5E793A7B8DA84847}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midnight.edds"
	};
	
	override void HandlerAttached(Widget w)
	{
		m_wButtonPreview = ButtonWidget.Cast(w.FindAnyWidget("PreviewButton"));
		m_wButtonLobby = ButtonWidget.Cast(w.FindAnyWidget("LobbyButton"));
		m_wButtonBriefing = ButtonWidget.Cast(w.FindAnyWidget("BriefingButton"));
		m_wButtonInGame = ButtonWidget.Cast(w.FindAnyWidget("InGameButton"));
		m_wButtonDebriefing = ButtonWidget.Cast(w.FindAnyWidget("DebriefingButton"));
		
		m_wGameModeHeaderLine = ImageWidget.Cast(w.FindAnyWidget("GameModeHeaderLine"));
		
		m_wTitleText = TextWidget.Cast(w.FindAnyWidget("TitleText"));
		m_wAuthorRichText = RichTextWidget.Cast(w.FindAnyWidget("AuthorRichText"));
		
		m_wWeatherImage = ImageWidget.Cast(w.FindAnyWidget("WeatherImage"));
		m_wTimeImage = ImageWidget.Cast(w.FindAnyWidget("TimeImage"));
		
		m_hButtonPreview = PS_GameModeHeaderButton.Cast(m_wButtonPreview.FindHandler(PS_GameModeHeaderButton));
		m_hButtonLobby = PS_GameModeHeaderButton.Cast(m_wButtonLobby.FindHandler(PS_GameModeHeaderButton));
		m_hButtonBriefing = PS_GameModeHeaderButton.Cast(m_wButtonBriefing.FindHandler(PS_GameModeHeaderButton));
		m_hButtonInGame = PS_GameModeHeaderButton.Cast(m_wButtonInGame.FindHandler(PS_GameModeHeaderButton));
		m_hButtonDebriefing = PS_GameModeHeaderButton.Cast(m_wButtonDebriefing.FindHandler(PS_GameModeHeaderButton));	
		
		m_hButtonPreview.m_OnClicked.Insert(Action_PreviewOpen);
		m_hButtonLobby.m_OnClicked.Insert(Action_LobbyOpen);
		m_hButtonInGame.m_OnClicked.Insert(Action_InGameOpen);
		m_hButtonBriefing.m_OnClicked.Insert(Action_BriefingOpen);
		m_hButtonDebriefing.m_OnClicked.Insert(Action_DebriefingOpen);
		
		m_bButtonAdvance = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("AdvanceButton").FindHandler(PS_GameModeHeaderButton));
		m_wAdvanceImage = ImageWidget.Cast(w.FindAnyWidget("AdvanceButton").FindAnyWidget("AdvanceImage"));
		
		m_MissionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (m_MissionHeader)
		{
			m_wTitleText.SetText(m_MissionHeader.m_sName);
			m_wAuthorRichText.SetTextFormat("<color rgba='226,167,79,255'>#PS-GameModeHeader_Author:</color> %1", m_MissionHeader.m_sAuthor);
		}
		
		ChimeraWorld world = GetGame().GetWorld();
		m_TimeAndWeatherManagerEntity = world.GetTimeAndWeatherManager();
		
		UpdateTimeAndWeather();
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
		GetGame().GetCallqueue().Call(UpdateProgressLine);
	}
	
	void UpdateTimeAndWeather()
	{
		float time = m_TimeAndWeatherManagerEntity.GetTimeOfTheDay();
		WeatherState weatherState = m_TimeAndWeatherManagerEntity.GetCurrentWeatherState();
		
		m_wTimeImage.LoadImageTexture(0, timeImages[8 * (time/24.0)]);
		m_wWeatherImage.LoadImageTexture(0, weatherState.GetIconPath());
	}
	
	void AddOnClick()
	{
		m_bButtonAdvance.m_OnClicked.Insert(Action_Advance);
	}
	
	void TryUpdate()
	{
		Update();
	}
	
	void Update()
	{
		m_hButtonPreview.Update();
		m_hButtonLobby.Update();
		m_hButtonBriefing.Update();
		m_hButtonInGame.Update();
		m_hButtonDebriefing.Update();
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = GetGame().GetPlayerController();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		//m_bButtonAdvance.SetVisible(Replication.IsServer() || PS_PlayersHelper.IsAdminOrServer());
		if (!PS_PlayersHelper.IsAdminOrServer())
		{
			m_wAdvanceImage.SetColor(Color.Gray);
		} else {
			m_wAdvanceImage.SetColor(Color.FromInt(0xff008020));
		}
	}
	
	void UpdateProgressLine()
	{
		GetGame().GetCallqueue().Call(UpdateProgressLineWrap);
	}
	void UpdateProgressLineWrap()
	{
		float panelX, panelY;
		float panelSizeX, panelSizeY;
		ButtonWidget currentButton = GetCurrentGameModeButton();
		if (!currentButton)
			return;
		
		currentButton.GetScreenPos(panelX, panelY);
		currentButton.GetScreenSize(panelSizeX, panelSizeY);
		
		float lineSize = 0.0;
		lineSize = GetGame().GetWorkspace().DPIUnscale(panelX + panelSizeX);
		FrameSlot.SetSizeX(m_wGameModeHeaderLine, lineSize);
	}
	
	ButtonWidget GetCurrentGameModeButton()
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameModeState = gameModeCoop.GetState();
		switch (gameModeState)
		{
			case SCR_EGameModeState.PREGAME:
				return m_wButtonPreview;
			case SCR_EGameModeState.SLOTSELECTION:
				return m_wButtonLobby;
			case SCR_EGameModeState.BRIEFING:
				return m_wButtonBriefing;
			case SCR_EGameModeState.GAME:
				return m_wButtonInGame;
			case SCR_EGameModeState.DEBRIEFING:
				return m_wButtonDebriefing;
		}
		return null;
	}
	
	void Action_Advance(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) 
			return;
		
		if (PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetState() == SCR_EGameModeState.GAME) return;
		
		playableController.AdvanceGameState(SCR_EGameModeState.NULL);
	}
	void Action_PreviewOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_EGameModeState state = gameMode.GetState();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			if (state != SCR_EGameModeState.PREGAME) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.PREGAME);
	}
	void Action_LobbyOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_EGameModeState state = gameMode.GetState();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			if (state != SCR_EGameModeState.SLOTSELECTION && state != SCR_EGameModeState.BRIEFING) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.SLOTSELECTION);
	}
	void Action_BriefingOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_EGameModeState state = gameMode.GetState();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			if (state != SCR_EGameModeState.SLOTSELECTION && state != SCR_EGameModeState.BRIEFING) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.BRIEFING);
	}
	void Action_InGameOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_EGameModeState state = gameMode.GetState();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			if (state != SCR_EGameModeState.GAME) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.GAME);
	}
	void Action_DebriefingOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		SCR_EGameModeState state = gameMode.GetState();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			if (state != SCR_EGameModeState.DEBRIEFING) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.DEBRIEFING);
	}
}