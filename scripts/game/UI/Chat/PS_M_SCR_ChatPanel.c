modded class SCR_ChatPanel : ScriptedWidgetComponent
{
	override protected void UpdateChatMessages()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = GetGame().GetPlayerController();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return super.UpdateChatMessages();
		FactionKey factionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gameMode.IsChatDisabled() || gameMode.GetState() == SCR_EGameModeState.SLOTSELECTION || gameMode.GetState() == SCR_EGameModeState.BRIEFING || factionKey == "" || PS_PlayersHelper.IsAdminOrServer())
			super.UpdateChatMessages();
		else
		{
			array<ref SCR_ChatMessage> messages = SCR_ChatPanelManager().GetInstance().GetMessages();
	
			for (int i = 0; i < m_iMessageLineCount; i++)
			{
				if (i > m_aMessageLines.Count() - 1 || i < 0)
					continue;

				SCR_ChatMessageLineComponent lineComp = m_aMessageLines[i];
				lineComp.SetVisible(false);
			}
		}
	}
}
