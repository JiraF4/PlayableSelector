modded class SCR_PlayerDelegateEditorComponent
{
	protected PS_GameModeCoop m_PS_DeathGameMode;

	override void EOnEditorInitServer()
	{
		super.EOnEditorInitServer();
		m_PS_DeathGameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (m_PS_DeathGameMode && m_PlayerDelegate)
			m_PS_DeathGameMode.GetOnHandlePlayerKilled().Insert(PS_OnPlayerKilled_S);
	}

	override void EOnEditorDeleteServer()
	{
		if (m_PS_DeathGameMode)
			m_PS_DeathGameMode.GetOnHandlePlayerKilled().Remove(PS_OnPlayerKilled_S);
		m_PS_DeathGameMode = null;
		GetGame().GetCallqueue().Remove(PS_NotifyPlayerKilled_S);
		super.EOnEditorDeleteServer();
	}

	/**
	 * Bridge only this player's lobby death to the vanilla delegate cleanup.
	 * Its broadcast must run after the kill callback's strict replication cycle.
	 */
	protected void PS_OnPlayerKilled_S(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		if (playerId != m_iPlayerID || !m_PlayerDelegate || !playerEntity)
			return;

		SCR_InstigatorContextData context = new SCR_InstigatorContextData(playerId, playerEntity, killerEntity, killer);
		GetGame().GetCallqueue().CallLater(PS_NotifyPlayerKilled_S, 0, false, context);
	}

	protected void PS_NotifyPlayerKilled_S(notnull SCR_InstigatorContextData context)
	{
		if (!m_PS_DeathGameMode || !m_PlayerDelegate || !context.GetVictimEntity())
			return;

		OnDeathServer(context);
	}
}
