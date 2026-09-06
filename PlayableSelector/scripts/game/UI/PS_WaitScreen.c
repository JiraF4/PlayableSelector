modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	WaitScreen
}

class PS_WaitScreen: MenuBase
{
	static bool m_bWaitEnded;
	
	TextWidget m_wInfoText;
	
	override void OnMenuOpen()
	{
		m_wInfoText = TextWidget.Cast(GetRootWidget().FindAnyWidget("InfoText"));
		
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		GetGame().GetCallqueue().CallLater(AwaitPlayerController, 100, true);
	}
	
	void AwaitPlayerController()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			m_wInfoText.SetText("Awaiting gamemode entity.");
			return;
		}
		
		#ifdef WORKBENCH
		if (gameMode.GetState() != SCR_EGameModeState.GAME)
		{
			IEntity WBCharacter = SCR_PlayerController.GetLocalControlledEntity();
			if (WBCharacter)
			{
				SCR_VoNComponent WBVoN = SCR_VoNComponent.Cast(WBCharacter.FindComponent(SCR_VoNComponent));
				if (WBVoN)
				{
					PS_PlayableComponent WBPlayableComponent = PS_PlayableComponent.Cast(WBCharacter.FindComponent(PS_PlayableComponent));
					WBPlayableComponent.SetPlayable(true);
					RplComponent rplComponent = RplComponent.Cast(WBCharacter.FindComponent(RplComponent));
					PS_PlayableManager.GetInstance().SetPlayerPlayable(SCR_PlayerController.GetLocalPlayerId(), rplComponent.Id());
					gameMode.StartGameMode();
				}
			}
		}
		#endif
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
		{
			m_wInfoText.SetText("Awaiting player controller.");
			return;
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager.IsReplicated())
		{
			m_wInfoText.SetText("Awaiting playableManager replication.");
			return;
		}
		
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (!VoNRoomsManager.IsReplicated())
		{
			m_wInfoText.SetText("Awaiting VoNRoomsManager replication.");
			return;
		}
		
		if (playerController.GetPlayerId() == 0)
		{
			m_wInfoText.SetText("Awaiting player id.");
			return;
		}
		
		// Body-less: the player controls nothing in the lobby; wait for the VoN proxy (the
		// menu/spectator voice host) to stream in instead of for a controlled character.
		if (!PS_VoNProxyComponent.GetProxyEntity(playerController.GetPlayerId()))
		{
			m_wInfoText.SetText("Awaiting VoN proxy.");
			return;
		}
		
		// Channels are deterministic string keys created lazily, so there is no per-player room to wait
		// for anymore. The player's current channel key is read directly.
		string roomKey = VoNRoomsManager.GetPlayerChannel(playerController.GetPlayerId());

		m_bWaitEnded = true;
		Close();
		
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.NotReady);
		playableController.MoveToVoNRoomByKey(playerController.GetPlayerId(), roomKey);
		playableController.SwitchToMenu(gameMode.GetState());
		
		GetGame().GetCallqueue().Remove(AwaitPlayerController);
	}
	
	override void OnMenuClose()
	{
		
	}
}