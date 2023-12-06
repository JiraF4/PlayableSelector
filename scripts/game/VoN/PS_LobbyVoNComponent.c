//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponentClass : SCR_VoNComponentClass
{}

//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponent : VoNComponent 
{
	const float PS_TRANSMISSION_TIMEOUT_MS = 400;
	protected float PS_m_fTransmitingTimeout;
	ref map<int, float> m_fPlayerSpeachReciveTime = new map<int, float>();
	ref map<int, bool> m_fPlayerSpeachReciveIsChannel = new map<int, bool>();
	
	void PS_LobbyVoNComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		GetGame().GetCallqueue().CallLater(DisablePhysicForOwner, 0, false, ent);
	}
	
	void DisablePhysicForOwner(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		if (physics)
		{
			physics.SetVelocity("0 0 0");
			physics.SetAngularVelocity("0 0 0");
			physics.SetMass(0);
			physics.SetDamping(1, 1);
			//physics.ChangeSimulationState(SimulationState.NONE);
			physics.SetActive(ActiveState.INACTIVE);
		}
	}
	
	
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
	
	bool IsTransmiting()
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return PS_m_fTransmitingTimeout >= worldTime;
	}
	
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		PS_m_fTransmitingTimeout = GetGame().GetWorld().GetWorldTime();
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
	}
	
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
		if (receiver) m_fPlayerSpeachReciveIsChannel[playerId] = true;
		else m_fPlayerSpeachReciveIsChannel[playerId] = false;
	}
};
