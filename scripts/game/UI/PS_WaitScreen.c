modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	WaitScreen
}

class PS_WaitScreen: MenuBase
{
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
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
		{
			m_wInfoText.SetText("Await player controller.");
			return;
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager.IsReplicated())
		{
			m_wInfoText.SetText("Await playableManager replication.");
			return;
		}
		
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (!VoNRoomsManager.IsReplicated())
		{
			m_wInfoText.SetText("Await VoNRoomsManager replication.");
			return;
		}
		
		if (playerController.GetPlayerId() == 0)
		{
			m_wInfoText.SetText("Await player id.");
			return;
		}
		
		if (!playerController.GetControlledEntity())
		{
			m_wInfoText.SetText("Await initial character.");
			return;
		}
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableControllerComponent.isVonInit())
		{
			m_wInfoText.SetText("Await VoN Initialization.");
			return;
		}
		
		int publicRoomId = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Public" + playerController.GetPlayerId().ToString());
		int globalRoomId = VoNRoomsManager.GetRoomWithFaction("", "#PS-VoNRoom_Global");
		if (publicRoomId == -1 || globalRoomId == -1)
		{
			m_wInfoText.SetText("Await VoN room creation.");
			return;
		}
		
		int roomId = VoNRoomsManager.GetPlayerRoom(playerController.GetPlayerId());
		string roomKey = VoNRoomsManager.GetRoomName(roomId);
		
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		
		playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.NotReady);
		playableController.SwitchToMenu(gameMode.GetState());
		playableController.MoveToVoNRoomByKey(playerController.GetPlayerId(), roomKey);
		
		Close();
		GetGame().GetCallqueue().Remove(AwaitPlayerController);
	}
	
	override void OnMenuClose()
	{
		
	}
}