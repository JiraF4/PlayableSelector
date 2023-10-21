//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponentClass : VoNComponentClass
{}

//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponent : VoNComponent 
{
	private const float TRANSMISSION_TIMEOUT_MS = 100;
	private float m_fTransmitingTimeout;
	ref map<int, float> m_fPlayerSpeachReciveTime = new map<int, float>();
	
	float GetPlayerSpeechTime(int playerId)
	{
		if (!m_fPlayerSpeachReciveTime.Contains(playerId)) return 0.0;
		return m_fPlayerSpeachReciveTime[playerId];
	}
	
	bool IsPlayerSpeech(int playerId)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return GetPlayerSpeechTime(playerId) > worldTime;
	}
	
	bool IsTransmiting()
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return m_fTransmitingTimeout > worldTime;
	}
	
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		m_fTransmitingTimeout = GetGame().GetWorld().GetWorldTime() + TRANSMISSION_TIMEOUT_MS;
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + TRANSMISSION_TIMEOUT_MS;
	}
	
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + TRANSMISSION_TIMEOUT_MS;
	}
};
