modded class SCR_MapMarkerSyncComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_iPlacedMarkerLimit = 100;
	}
}

class PS_MapMarkersBaseJson : JsonApiStruct
{
	ref array<ref PS_MapMarkerBaseJson> m_aMapMarkers = {};
	
	void PS_MapMarkersBaseJson()
	{
		RegAll();
	}
}

class PS_MapMarkerBaseJson : JsonApiStruct
{
	int m_eType;
	int m_iConfigID;
	int m_iFlags;
	int m_iPosWorldX;
	int m_iPosWorldY;
	int m_iFactionFlags;
	int m_iColorEntry;
	int m_iIconEntry;
	int m_iRotation;
	string m_sCustomText;
	
	SCR_MapMarkerBase GetMapMarkerBase()
	{
		SCR_MapMarkerBase mapMarkerBase = new SCR_MapMarkerBase();
		mapMarkerBase.PS_SetMapMarkerBaseJson(this);
		return mapMarkerBase;
	}
	
	void PS_MapMarkerBaseJson()
	{
		RegAll();
	}
}

modded class SCR_MapMarkerBase
{
	PS_MapMarkerBaseJson PS_GetMapMarkerBaseJson()
	{
		PS_MapMarkerBaseJson mapMarkerBaseJson = new PS_MapMarkerBaseJson();
		mapMarkerBaseJson.m_eType = m_eType;
		mapMarkerBaseJson.m_iConfigID = m_iConfigID;
		mapMarkerBaseJson.m_iFlags = m_iFlags;
		mapMarkerBaseJson.m_iPosWorldX = m_iPosWorldX;
		mapMarkerBaseJson.m_iPosWorldY = m_iPosWorldY;
		mapMarkerBaseJson.m_iFactionFlags = m_iFactionFlags;
		mapMarkerBaseJson.m_iColorEntry = m_iColorEntry;
		mapMarkerBaseJson.m_iIconEntry = m_iIconEntry;
		mapMarkerBaseJson.m_iRotation = m_iRotation;
		mapMarkerBaseJson.m_sCustomText = m_sCustomText;
		return mapMarkerBaseJson;
	}
	
	void PS_SetMapMarkerBaseJson(PS_MapMarkerBaseJson mapMarkerBaseJson)
	{
		m_eType = mapMarkerBaseJson.m_eType;
		m_iConfigID = mapMarkerBaseJson.m_iConfigID;
		m_iFlags = mapMarkerBaseJson.m_iFlags;
		m_iPosWorldX = mapMarkerBaseJson.m_iPosWorldX;
		m_iPosWorldY = mapMarkerBaseJson.m_iPosWorldY;
		m_iFactionFlags = mapMarkerBaseJson.m_iFactionFlags;
		m_iColorEntry = mapMarkerBaseJson.m_iColorEntry;
		m_iIconEntry = mapMarkerBaseJson.m_iIconEntry;
		m_iRotation = mapMarkerBaseJson.m_iRotation;
		m_sCustomText = mapMarkerBaseJson.m_sCustomText;
	}
}