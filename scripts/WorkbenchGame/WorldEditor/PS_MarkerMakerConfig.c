[BaseContainerProps(configRoot: true)]
class PS_MarkerMakerConfig
{
	[Attribute(defvalue: "{CD85ADE9E0F54679}PrefabsEditable/Markers/EditableMarker.et", params: "et")]
	ResourceName m_sManualMarkerPrefab;
	
	[Attribute("")]
	ref PS_ManualMarkerConfig m_mManualMarkerConfig;
	
	[Attribute("")]
	ref array<ref PS_FactionManualMarkerConfig> m_mFactionsManualMarkerConfig;
	
	[Attribute("")]
	ref array<ref PS_VehicleManualMarkerConfig> m_mVehiclesManualMarkerConfig;
}

[BaseContainerProps()]
class PS_FactionManualMarkerConfig
{
	[Attribute("")]
	FactionKey m_sFactionKey;
	
	[Attribute("")]
	ref PS_ManualMarkerConfig m_mManualMarkerConfig;
}

[BaseContainerProps()]
class PS_VehicleManualMarkerConfig
{
	[Attribute("")]
	EVehicleType m_iVehicleType;
	
	[Attribute("")]
	ref PS_ManualMarkerConfig m_mManualMarkerConfig;
}

[BaseContainerProps()]
class PS_ManualMarkerConfig
{
	[Attribute("{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset")]
	ResourceName m_sImageSet;
	[Attribute("")]
	ResourceName m_sImageSetGlow;
	[Attribute("1 1 1 1", UIWidgets.ColorPicker)]
	ref Color m_MarkerColor;
	[Attribute("empty")]
	string m_sQuadName;
	[Attribute("5.0")]
	float m_fWorldSize;
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sDescription;
	[Attribute("true")]
	bool m_bUseWorldScale;
	[Attribute("")]
	ref array<FactionKey> m_aVisibleForFactions;
	[Attribute("")]
	bool m_bVisibleForEmptyFaction;
}