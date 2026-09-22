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
		
		JsonLoadContext configLoadContext = new JsonLoadContext();
		if (configLoadContext.LoadFromFile(m_configFilePath))
			if (configLoadContext.ReadValue("", m_cReservedPlayerIdentitiesConfig))
			{
				foreach (string GUID : m_cReservedPlayerIdentitiesConfig.GUIDS)
				{
					Print("Reserve: " + GUID);
					m_aPlayerIdentities.Insert(GUID);
				}
			}
		
		ServerInfo serverInfo = GetGame().GetServerInfo();
		if (serverInfo)
			m_iMaxPlayersCount = serverInfo.GetPlayerLimit();
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
		
		string GUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (m_aPlayerIdentities.Contains(GUID)) 
			return;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager.GetPlayableByPlayer(playerId) != RplId.Invalid())
			return;
		
		playerManager.KickPlayer(playerId, PlayerManagerKickReason.KICK, 0);
	}
	
}