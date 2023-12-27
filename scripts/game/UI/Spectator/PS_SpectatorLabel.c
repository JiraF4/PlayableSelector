[ComponentEditorProps(category: "GameScripted/Character", description: "Add label for observers", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelClass: ScriptComponentClass
{
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabel : ScriptComponent
{
	PS_SpectatorLabelsManager m_sSpectatorLabelsManager;
	
	[Attribute(defvalue: "{52CA8FF5F56C6F31}UI/Map/ManualMapMarkerBase.layout", params: "layout")]
	ResourceName m_sMarkerPrefab;
	
	[Attribute()]
	ref PS_ManualMarkerConfig m_cMapMarkerConfig;
	
	[Attribute(defvalue: "{8A4C0D6D625BB09D}UI/Spectator/SpectatorLabelIcon.layout", params: "layout")]
	ResourceName m_sSpectatorLabelLayout;
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddToList, 0, false, owner);
	}
	
	void AddToList(IEntity owner)
	{
		m_sSpectatorLabelsManager = PS_SpectatorLabelsManager.GetInstance();
		m_sSpectatorLabelsManager.RegistrateLabel(this);
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected FrameWidget m_wMapFrame;
	PS_ManualMarkerComponent m_hManualMarkerComponent;
	void UpdateMarker()
	{
		if (!m_cMapMarkerConfig) return;
		if (!m_MapEntity.IsOpen()) return;
		if (!m_hManualMarkerComponent) CreateMarker();
		
		m_hManualMarkerComponent.SetSlotWorld(GetOwner().GetOrigin()
			, GetOwner().GetYawPitchRoll(), m_cMapMarkerConfig.m_fWorldSize
			, m_cMapMarkerConfig.m_bUseWorldScale);
	}
	
	void CreateMarker()
	{
		// Get map frame
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame) mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame) return; // Somethig gone wrong
		
		// Create and init marker
		m_wRoot = Widget.Cast(GetGame().GetWorkspace().CreateWidgets(m_sMarkerPrefab, mapFrame));
		m_hManualMarkerComponent = PS_ManualMarkerComponent.Cast(m_wRoot.FindHandler(PS_ManualMarkerComponent));
		m_hManualMarkerComponent.SetImage(m_cMapMarkerConfig.m_sImageSet, m_cMapMarkerConfig.m_sQuadName);
		m_hManualMarkerComponent.SetImageGlow(m_cMapMarkerConfig.m_sImageSetGlow, m_cMapMarkerConfig.m_sQuadName);
		m_hManualMarkerComponent.SetDescription(m_cMapMarkerConfig.m_sDescription);
		m_hManualMarkerComponent.SetColor(m_cMapMarkerConfig.m_MarkerColor);
		m_hManualMarkerComponent.OnMouseLeave(null, null, 0, 0);
	}
	
	void PS_SpectatorLabel(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		
	}
	
	void ~PS_SpectatorLabel()
	{
		if (m_sSpectatorLabelsManager)
			m_sSpectatorLabelsManager.UnRegistrateLabel(this);
	}
}