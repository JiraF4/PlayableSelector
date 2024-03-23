void PS_ScriptInvokerOnCaptureMethod(BaseTransceiver transmitter);
typedef func PS_ScriptInvokerOnCaptureMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnCaptureMethod> PS_ScriptInvokerOnCapture;

void PS_ScriptInvokerOnReceiveMethod(int playerId, BaseTransceiver receiver, int frequency, float quality);
typedef func PS_ScriptInvokerOnReceiveMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnReceiveMethod> PS_ScriptInvokerOnReceive;

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
	
	ref PS_ScriptInvokerOnReceive m_ScriptInvokerOnReceiveStart = new PS_ScriptInvokerOnReceive();
	PS_ScriptInvokerOnReceive GetOnReceiveStart()
		return m_ScriptInvokerOnReceiveStart;
	ref ScriptInvokerInt m_ScriptInvokerOnReceiveEnd = new ScriptInvokerInt();
	ScriptInvokerInt GetOnReceiveEnd()
		return m_ScriptInvokerOnReceiveEnd;
	ref PS_ScriptInvokerOnCapture m_ScriptInvokerOnCaptured = new PS_ScriptInvokerOnCapture();
	PS_ScriptInvokerOnCapture GetOnCaptured()
		return m_ScriptInvokerOnCaptured;
	
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
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		OnReceiveHandle(playerId, transmitter, 0, 0);
	}
	
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{		
		OnReceiveHandle(playerId, receiver, frequency, quality);
	}
	
	protected void OnReceiveHandle(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!IsPlayerSpeech(playerId))
		{
			GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
		}
		bool alreadyReceive = IsPlayerSpeech(playerId);
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
		if (receiver) {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = true;
			if (!alreadyReceive || !isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}
		else {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = false;
			if (!alreadyReceive || isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}
	}
	
	void AwaitReceiveEnd(int playerId)
	{
		if (IsPlayerSpeech(playerId))
		{
			GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
			return;
		}
		
		m_ScriptInvokerOnReceiveEnd.Invoke(playerId);
	}
};
