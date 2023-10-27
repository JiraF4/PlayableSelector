//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponentClass : SCR_VoNComponentClass
{}

//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponent : SCR_VoNComponent 
{
	protected const float PS_TRANSMISSION_TIMEOUT_MS = 500;
	protected float PS_m_fTransmitingTimeout;
	ref map<int, float> m_fPlayerSpeachReciveTime = new map<int, float>();
	ref map<int, bool> m_fPlayerSpeachReciveIsChannel = new map<int, bool>();
	
	float GetPlayerSpeechTime(int playerId)
	{
		if (!m_fPlayerSpeachReciveTime.Contains(playerId)) return 0.0;
		return m_fPlayerSpeachReciveTime[playerId];
	}
	
	float IsPlayerSpeechInChanel(int playerId)
	{		
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController.GetPlayerId() == playerId) {
			return GetCommMethod() == ECommMethod.SQUAD_RADIO;
		}
		
		if (!m_fPlayerSpeachReciveIsChannel.Contains(playerId)) return false;
		return m_fPlayerSpeachReciveIsChannel[playerId];
	}
	
	bool IsPlayerSpeech(int playerId)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return GetPlayerSpeechTime(playerId) > worldTime;
	}
	
	override bool IsTransmiting()
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return PS_m_fTransmitingTimeout > worldTime;
	}
	
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		PS_m_fTransmitingTimeout = GetGame().GetWorld().GetWorldTime() + PS_TRANSMISSION_TIMEOUT_MS;
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + PS_TRANSMISSION_TIMEOUT_MS;
	}
	
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + PS_TRANSMISSION_TIMEOUT_MS;
		if (receiver) m_fPlayerSpeachReciveIsChannel[playerId] = true;
		else m_fPlayerSpeachReciveIsChannel[playerId] = false;
	}
};
