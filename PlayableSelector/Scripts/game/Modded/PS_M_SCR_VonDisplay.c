// PS_M_SCR_VonDisplay.c
// Filter floating VoN HUD indicator for menu speakers and hide internal proxy frequency.

modded class SCR_VonDisplay
{
	override protected bool UpdateTransmission(TransmissionData data, BaseTransceiver radioTransceiver, int frequency, bool IsReceiving)
	{
		if (IsReceiving && data)
		{
			// Filter out incoming transmissions from menu speakers (lobby / briefing / spectator)
			// The voice room panel already shows active speakers, so the floating HUD popup is redundant
			if (data.m_iPlayerID > 0 && SCR_VoNComponent.PS_IsMenuSpeaker(data.m_iPlayerID))
				return false;
		}

		bool result = super.UpdateTransmission(data, radioTransceiver, frequency, IsReceiving);

		// Outgoing transmission for a menu speaker - keep mic indicator, hide internal proxy frequency
		if (!IsReceiving && data && data.m_Widgets && data.m_Widgets.m_wFrequency)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
				data.m_Widgets.m_wFrequency.SetVisible(false);
		}

		return result;
	}
}
