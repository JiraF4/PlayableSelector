class PS_GameModeHeader : ScriptedWidgetComponent
{
	
	SCR_ButtonBaseComponent m_bButtonPreview;
	SCR_ButtonBaseComponent m_bButtonLobby;
	SCR_ButtonBaseComponent m_bButtonBriefing;
	SCR_ButtonBaseComponent m_bButtonInGame;
	SCR_ButtonBaseComponent m_bButtonDebriefing;
	
	override void HandlerAttached(Widget w)
	{
		m_bButtonPreview = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("PreviewButton").FindHandler(SCR_ButtonBaseComponent));
		m_bButtonLobby = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("LobbyButton").FindHandler(SCR_ButtonBaseComponent));
		m_bButtonBriefing = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("BriefingButton").FindHandler(SCR_ButtonBaseComponent));
		m_bButtonInGame = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("InGameButton").FindHandler(SCR_ButtonBaseComponent));
		m_bButtonDebriefing = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("DebriefingButton").FindHandler(SCR_ButtonBaseComponent));	
		
		m_bButtonDebriefing.m_bUseColorization = false;
		
		UpdateActiveButtons();
	}
	
	
	void UpdateActiveButtons()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		
		// Disable not active
		switch (gameMode.GetState())
		{
			case SCR_EGameModeState.NULL:
				break;
			case SCR_EGameModeState.PREGAME:
				m_bButtonLobby.m_bUseColorization = false;
			case SCR_EGameModeState.SLOTSELECTION:
				m_bButtonBriefing.m_bUseColorization = false;
			case SCR_EGameModeState.BRIEFING:
				m_bButtonInGame.m_bUseColorization = false;
			case SCR_EGameModeState.GAME:
				m_bButtonDebriefing.m_bUseColorization = false;
		}
		
		// Add actions to active
		switch (gameMode.GetState())
		{
			case SCR_EGameModeState.NULL:
				break;
			case SCR_EGameModeState.POSTGAME:
				m_bButtonDebriefing.m_OnClicked.Insert(Action_DebriefingOpen);
			case SCR_EGameModeState.GAME:
				m_bButtonInGame.m_OnClicked.Insert(Action_InGameOpen);
			case SCR_EGameModeState.BRIEFING:
				m_bButtonBriefing.m_OnClicked.Insert(Action_BriefingOpen);
			case SCR_EGameModeState.SLOTSELECTION:
				m_bButtonLobby.m_OnClicked.Insert(Action_LobbyOpen);
		}
		m_bButtonPreview.m_OnClicked.Insert(Action_PreviewOpen);
	}
	
	void Action_PreviewOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.PREGAME);
	}
	void Action_LobbyOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.SLOTSELECTION);
	}
	void Action_BriefingOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.BRIEFING);
	}
	void Action_InGameOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.GAME);
	}
	void Action_DebriefingOpen(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(SCR_EGameModeState.POSTGAME);
	}
}