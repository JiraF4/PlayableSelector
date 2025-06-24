[ComponentEditorProps(category: "GameScripted/Character", description: "Add label for observers", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelClass: ScriptComponentClass
{
};


class PS_SpectatorLabel : ScriptComponent
{
	PS_SpectatorLabelsManager m_sSpectatorLabelsManager;
	
	[Attribute(defvalue: "{52CA8FF5F56C6F31}UI/Map/ManualMapMarkerBase.layout", params: "layout")]
	ResourceName m_sMarkerPrefab;
	
	[Attribute()]
	ref PS_ManualMarkerConfig m_cMapMarkerConfig;
	
	[Attribute(defvalue: "{8A4C0D6D625BB09D}UI/Spectator/SpectatorLabelIcon.layout", params: "layout")]
	ResourceName m_sSpectatorLabelLayout;
	
	[Attribute()]
	string m_sBoneName;
	
	protected bool m_bForceShowName;
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddToList, 0, false, owner);
	}
	
	void AddToList(IEntity owner)
	{
		m_sSpectatorLabelsManager = PS_SpectatorLabelsManager.GetInstance();
		if (!m_sSpectatorLabelsManager)
			return;
		m_sSpectatorLabelsManager.RegistrateLabel(this);
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	protected Widget m_wRootLabel;
	protected PS_SpectatorLabelIcon m_hLabelIcon;
	protected FrameWidget m_wLabelsFrame;
	void CreateLabel(FrameWidget labelsFrame)
	{
		if (m_wRootLabel) return;
		m_wLabelsFrame = labelsFrame;
		
		m_wRootLabel = GetGame().GetWorkspace().CreateWidgets(m_sSpectatorLabelLayout, m_wLabelsFrame);
		m_hLabelIcon = PS_SpectatorLabelIcon.Cast(m_wRootLabel.FindHandler(PS_SpectatorLabelIcon));
		m_hLabelIcon.SetEntity(GetOwner(), m_sBoneName);
	}
	
	PS_SpectatorLabelIcon GetLabelIcon()
	{
		return m_hLabelIcon;
	}
	
	void UpdateLabel()
	{
		if (!m_wRootLabel) return;
		if (m_MapEntity.IsOpen()) return;
		
		m_hLabelIcon.Update();
	}
	
	void RemoveLabel()
	{
		if (!m_wRootLabel) return;
		
		m_wRootLabel.RemoveFromHierarchy();
	}
	
	protected Widget m_wRootMarker;
	protected SCR_MapEntity m_MapEntity;
	protected FrameWidget m_wMapFrame;
	protected PS_ManualMarkerComponent m_hManualMarkerComponent;
	bool UpdateMarker()
	{
		if (!m_cMapMarkerConfig) return false;
		if (!m_MapEntity.IsOpen()) return false;
		if (!m_hManualMarkerComponent) CreateMarker();
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.GetFriendliesSpectatorOnly())
		{
			SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(GetOwner());
			if (chimeraCharacter.GetFactionKey() != PS_PlayableManager.GetInstance().GetPlayerFactionKeyRemembered(GetGame().GetPlayerController().GetPlayerId()))
			{
				m_hManualMarkerComponent.SetOpacity(0);
			}
		}
		
		m_hManualMarkerComponent.SetSlotWorld(GetOwner().GetOrigin()
			, GetOwner().GetYawPitchRoll() + Vector(90, 0, 0), m_cMapMarkerConfig.m_fWorldSize
			, m_cMapMarkerConfig.m_bUseWorldScale, m_cMapMarkerConfig.m_fMinSize);
		
		return true;
	}
	
	void CreateMarker()
	{
		// Get map frame
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame) mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame) return; // Somethig gone wrong
		
		// Create and init marker
		m_wRootMarker = GetGame().GetWorkspace().CreateWidgets(m_sMarkerPrefab, mapFrame);
		m_hManualMarkerComponent = PS_ManualMarkerComponent.Cast(m_wRootMarker.FindHandler(PS_ManualMarkerComponent));
		m_hManualMarkerComponent.SetImage(m_cMapMarkerConfig.m_sImageSet, m_cMapMarkerConfig.m_sQuadName);
		m_hManualMarkerComponent.SetImageGlow(m_cMapMarkerConfig.m_sImageSetGlow, m_cMapMarkerConfig.m_sQuadName);
		m_hManualMarkerComponent.SetDescription(m_cMapMarkerConfig.m_sDescription);
		m_hManualMarkerComponent.SetColor(m_cMapMarkerConfig.m_MarkerColor);
		m_hManualMarkerComponent.OnMouseLeave(null, null, 0, 0);
		
		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(GetOwner());
		if (!chimeraCharacter)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(chimeraCharacter.GetFaction());
		if (faction)
			m_hManualMarkerComponent.SetColor(faction.GetOutlineFactionColor());
	}
	
	void RemoveMarker()
	{
		if (!m_wRootMarker) return;
		
		m_wRootMarker.RemoveFromHierarchy();
	}
	
	void PS_SpectatorLabel(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		
	}
	
	void ~PS_SpectatorLabel()
	{
		if (m_sSpectatorLabelsManager)
			m_sSpectatorLabelsManager.UnRegistrateLabel(this);
		
		RemoveLabel();
		RemoveMarker();
	}
}