modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	WaitScreen
}

class PS_WaitScreen: MenuBase
{
	override void OnMenuOpen()
	{
		GetGame().GetCallqueue().CallLater(AwaitPlayerController, 100);
	}
	
	void AwaitPlayerController()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		if (playerController && playableManager.IsReplicated()) {
			if (playerController.GetPlayerId() != 0) {
				if (playerController.GetControlledEntity()) {
					PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
					PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
					playableController.SwitchToMenu(gameMode.GetState());
					return;
				}
			}
		}
		
		GetGame().GetCallqueue().CallLater(AwaitPlayerController, 100);
	}
	
	override void OnMenuClose()
	{
		
	}
}