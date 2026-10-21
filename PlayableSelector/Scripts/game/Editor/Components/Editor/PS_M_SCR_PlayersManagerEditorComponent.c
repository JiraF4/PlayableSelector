modded class SCR_PlayersManagerEditorComponent
{
	protected PS_GameModeCoop m_PS_DeathGameMode;

	override void EOnEditorOpenServerCallback()
	{
		super.EOnEditorOpenServerCallback();
		PS_StopListeningForPlayerDeath_S();
		m_PS_DeathGameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (m_PS_DeathGameMode)
			m_PS_DeathGameMode.GetOnHandlePlayerKilled().Insert(PS_OnPlayerKilled_S);
	}

	override void EOnEditorCloseServer()
	{
		PS_StopListeningForPlayerDeath_S();
		super.EOnEditorCloseServer();
	}

	override void EOnEditorDeleteServer()
	{
		PS_StopListeningForPlayerDeath_S();
		super.EOnEditorDeleteServer();
	}

	protected void PS_StopListeningForPlayerDeath_S()
	{
		if (m_PS_DeathGameMode)
			m_PS_DeathGameMode.GetOnHandlePlayerKilled().Remove(PS_OnPlayerKilled_S);
		m_PS_DeathGameMode = null;
		GetGame().GetCallqueue().Remove(PS_NotifyPlayerKilled_S);
	}

	/**
	 * The lobby suppresses the vanilla kill event. Defer this editor-only notification
	 * because its owner RPC cannot run in the kill callback's strict replication cycle.
	 */
	protected void PS_OnPlayerKilled_S(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		if (!playerEntity)
			return;

		SCR_InstigatorContextData context = new SCR_InstigatorContextData(playerId, playerEntity, killerEntity, killer);
		GetGame().GetCallqueue().CallLater(PS_NotifyPlayerKilled_S, 0, false, context);
	}

	protected void PS_NotifyPlayerKilled_S(notnull SCR_InstigatorContextData context)
	{
		if (!m_PS_DeathGameMode || !context.GetVictimEntity())
			return;

		OnDeathServer(context);
	}
}
