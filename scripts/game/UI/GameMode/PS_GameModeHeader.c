class PS_GameModeHeader : ScriptedWidgetComponent
{
	
	PS_GameModeHeaderButton m_bButtonPreview;
	PS_GameModeHeaderButton m_bButtonLobby;
	PS_GameModeHeaderButton m_bButtonBriefing;
	PS_GameModeHeaderButton m_bButtonInGame;
	PS_GameModeHeaderButton m_bButtonDebriefing;
	
	PS_GameModeHeaderButton m_bButtonAdvance;
	
	override void HandlerAttached(Widget w)
	{
		m_bButtonPreview = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("PreviewButton").FindHandler(PS_GameModeHeaderButton));
		m_bButtonLobby = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("LobbyButton").FindHandler(PS_GameModeHeaderButton));
		m_bButtonBriefing = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("BriefingButton").FindHandler(PS_GameModeHeaderButton));
		m_bButtonInGame = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("InGameButton").FindHandler(PS_GameModeHeaderButton));
		m_bButtonDebriefing = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("DebriefingButton").FindHandler(PS_GameModeHeaderButton));	
		
		m_bButtonPreview.m_OnClicked.Insert(Action_PreviewOpen);
		m_bButtonLobby.m_OnClicked.Insert(Action_LobbyOpen);
		m_bButtonInGame.m_OnClicked.Insert(Action_InGameOpen);
		m_bButtonBriefing.m_OnClicked.Insert(Action_BriefingOpen);
		m_bButtonDebriefing.m_OnClicked.Insert(Action_DebriefingOpen);
		
		m_bButtonAdvance = PS_GameModeHeaderButton.Cast(w.FindAnyWidget("AdvanceButton").FindHandler(PS_GameModeHeaderButton));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
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
		m_bButtonPreview.Update();
		m_bButtonLobby.Update();
		m_bButtonBriefing.Update();
		m_bButtonInGame.Update();
		m_bButtonDebriefing.Update();
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController thisPlayerController = GetGame().GetPlayerController();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(thisPlayerController.GetPlayerId());
		m_bButtonAdvance.SetVisible(Replication.IsServer() || playerRole == EPlayerRole.ADMINISTRATOR);
	}
	
	void Action_Advance(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (playerRole != EPlayerRole.ADMINISTRATOR && !Replication.IsServer()) return;
		
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
		
		if (playerRole != EPlayerRole.ADMINISTRATOR)
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
		
		if (playerRole != EPlayerRole.ADMINISTRATOR)
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
		
		if (playerRole != EPlayerRole.ADMINISTRATOR)
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
		
		if (playerRole != EPlayerRole.ADMINISTRATOR)
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
		
		if (playerRole != EPlayerRole.ADMINISTRATOR && !Replication.IsServer())
			if (state != SCR_EGameModeState.POSTGAME) return;
		
		playableController.SwitchToMenu(SCR_EGameModeState.POSTGAME);
	}
}