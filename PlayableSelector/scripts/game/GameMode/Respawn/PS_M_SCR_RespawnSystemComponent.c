modded class SCR_RespawnSystemComponentClass : RespawnSystemComponentClass
{
}

//! Scripted implementation that handles spawning and respawning of players.
//! Should be attached to a GameMode entity.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
modded class SCR_RespawnSystemComponent : RespawnSystemComponent
{
	[Attribute(category: "Respawn System")]
	protected ref SCR_SpawnLogic m_SpawnLogic;
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, category: "Respawn System")]
	protected bool m_bEnableRespawn;
	
	[Attribute("1", desc: "Handles visibility of the Respawn button in Pause menu.", category: "Respawn System")]
	protected bool m_bEnablePauseMenuRespawn;

	[Attribute("{A1CE9D1EC16DA9BE}UI/layouts/Menus/MainMenu/SplashScreen.layout", desc: "Optional: Layout shown during world load completion and respawn menu opening or automatically completing.", category: "Respawn System")]
	protected ResourceName m_sLoadingLayout;

	[Attribute("{A39BE59EB6F41125}Configs/Respawn/SpawnPointRequestResultInfoConfig.conf", desc: "Holds a config of all reasons why a specific spawnpoint can be disabled", category: "Respawn System")]
	protected ResourceName m_sSpawnPointRequestResultInfoHolder;

	protected ref SCR_SpawnPointRequestResultInfoConfig m_SpawnPointRequestResultInfoHolder;

	// Instance of this component
	private static SCR_RespawnSystemComponent s_Instance = null;

	// The parent of this entity which should be a gamemode
	protected SCR_BaseGameMode m_pGameMode;

	// Parent entity's rpl component
	protected RplComponent m_RplComponent;

	// Preload
	protected ref SimplePreload m_Preload;
	protected ref ScriptInvoker Event_OnRespawnEnabledChanged;
	
	protected Widget m_wLoadingPlaceholder;
	protected SCR_LoadingSpinner m_LoadingSpinner;

	//------------------------------------------------------------------------------------------------
	//! \param[in] requestComponent
	//! \param[in] response
	//! \param[in] data
	//! \return
	override SCR_BaseSpawnPointRequestResultInfo GetSpawnPointRequestResultInfo(SCR_SpawnRequestComponent requestComponent, SCR_ESpawnResult response, SCR_SpawnData data){	return null;	} 
	static override SCR_RespawnSystemComponent GetInstance()	{return null;}
	override RplComponent GetRplComponent()	{return null;}
	static override MenuBase OpenRespawnMenu()	{return null;} 
	static override void CloseRespawnMenu()	{return;} 
	override void ServerSetEnableRespawn(bool enableSpawning){return;}  
	override bool IsRespawnEnabled()	{return false;} 
	override bool IsPauseMenuRespawnEnabled() {return null;} 
	override bool IsFactionChangeAllowed() {return false;} 
	override ScriptInvoker GetOnRespawnEnabledChanged() {return null;} 
	override void OnPlayerRegistered_S(int playerId) {return;}	
	override void OnInit(IEntity owner) {return;}
	override void OnPlayerAuditSuccess_S(int playerId)	{return;}
}
