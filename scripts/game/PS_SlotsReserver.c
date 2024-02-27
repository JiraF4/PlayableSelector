[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SlotsReserverClass: ScriptComponentClass
{
	
};

class PS_ReservedPlayerIdentitiesConfig: JsonApiStruct
{
	ref array<string> GUIDS = new array<string>;
	
	void DSGameConfig()
	{
		RegV("GUIDS");
	}
}

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_SlotsReserver : ScriptComponent
{
	static string m_configFilePath = "$profile:PS_SlotsReserver_Config.json";
	ref PS_ReservedPlayerIdentitiesConfig m_cReservedPlayerIdentitiesConfig = new PS_ReservedPlayerIdentitiesConfig();
	
	ref array<string> m_aPlayerIdentities = new array<string>();
	int m_iMaxPlayersCount;
	
	bool m_bEnabled = false;
	
	void SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.GetOnPlayerAuditSuccess().Insert(CheckReserved);
		
		SCR_JsonLoadContext configLoadContext = new SCR_JsonLoadContext();
		if (configLoadContext.LoadFromFile(m_configFilePath))
			if (configLoadContext.ReadValue("", m_cReservedPlayerIdentitiesConfig))
			{
				foreach (string GUID : m_cReservedPlayerIdentitiesConfig.GUIDS)
				{
					m_aPlayerIdentities.Insert(GUID);
				}
			}
		
		DSSession dSSession = GetGame().GetBackendApi().GetDSSession();
		if (dSSession)
			m_iMaxPlayersCount = dSSession.PlayerLimit();
	}
	
	void AddGUIDs(array<string> GUIDS)
	{
		m_aPlayerIdentities.InsertAll(GUIDS);
	}
	
	void CheckReserved(int playerId)
	{
		if (!m_bEnabled) 
			return;
		if (!Replication.IsServer()) 
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		string GUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (m_aPlayerIdentities.Contains(GUID)) 
			return;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager.GetPlayableByPlayer(playerId) != RplId.Invalid())
			return;
		
		playerManager.KickPlayer(playerId, PlayerManagerKickReason.KICK, 0);
	}
	
}