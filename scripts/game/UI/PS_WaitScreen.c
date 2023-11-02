modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	WaitScreen
}

class PS_WaitScreen: MenuBase
{
	override void OnMenuOpen()
	{
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		GetGame().GetCallqueue().CallLater(AwaitPlayerController, 100);
	}
	
	void AwaitPlayerController()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (playerController && playableManager.IsReplicated() && VoNRoomsManager.IsReplicated()) {
			if (playerController.GetPlayerId() != 0) {
				if (playerController.GetControlledEntity()) {
					PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
					if (playableControllerComponent.isVonInit()) {
						int roomId = VoNRoomsManager.GetPlayerRoom(playerController.GetPlayerId());
						string roomKey = VoNRoomsManager.GetRoomName(roomId);
						
						PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
						PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
						
						playableController.SetPlayerState(playerController.GetPlayerId(), PS_EPlayableControllerState.NotReady);
						playableController.MoveToVoNRoomByKey(playerController.GetPlayerId(), roomKey);
						playableController.SwitchToMenu(gameMode.GetState());
						return;
					}
				}
			}
		}
		
		GetGame().GetCallqueue().CallLater(AwaitPlayerController, 100);
	}
	
	override void OnMenuClose()
	{
		
	}
}