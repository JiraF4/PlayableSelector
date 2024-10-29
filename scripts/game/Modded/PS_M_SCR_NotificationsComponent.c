modded class SCR_NotificationsComponent
{
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	override protected void ReceiveSCR_NotificationData(ENotification id, SCR_NotificationData data)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			super.ReceiveSCR_NotificationData(id, data);
			return;
		}
		if (!gameMode.IsChatDisabled())
		{
			super.ReceiveSCR_NotificationData(id, data);
			return;
		}
		if (id >= 400 && id < 500)
		{
			super.ReceiveSCR_NotificationData(id, data);
			return;
		}
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		FactionKey factionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
		if (gameMode.GetState() == SCR_EGameModeState.SLOTSELECTION || gameMode.GetState() == SCR_EGameModeState.BRIEFING || factionKey == "" || PS_PlayersHelper.IsAdminOrServer())
			super.ReceiveSCR_NotificationData(id, data);
	}
}