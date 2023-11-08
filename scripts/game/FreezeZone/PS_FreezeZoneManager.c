//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_FreezeZoneManagerClass: SCR_BaseGameModeComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_FreezeZoneManager : SCR_BaseGameModeComponent
{
	[Attribute("30000", UIWidgets.EditBox, "Freeze time", "")]
	int m_iFreezeTime = ;
	
	protected ref array<PS_FreezeZone> m_aZones = new array<PS_FreezeZone>();
	
	void RegisterZone(PS_FreezeZone zone)
	{
		m_aZones.Insert(zone)
	}
	
	void UnRegisterZone(PS_FreezeZone zone)
	{
		m_aZones.RemoveItem(zone);
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
	}
	
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		if (state == SCR_EGameModeState.GAME) 
			GetGame().GetCallqueue().CallLater(removeFreezeZones, m_iFreezeTime);
	}
	
	void removeFreezeZones()
	{
		for (int i = 0; i < m_aZones.Count(); i++)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_aZones[i]);
		}
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_FreezeZoneManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_FreezeZoneManager.Cast(gameMode.FindComponent(PS_FreezeZoneManager));
		else
			return null;
	}
}