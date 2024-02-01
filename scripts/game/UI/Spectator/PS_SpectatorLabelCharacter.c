[ComponentEditorProps(category: "GameScripted/Character", description: "Add label for observers", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelCharacterClass : PS_SpectatorLabelClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorLabelCharacter : PS_SpectatorLabel
{
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected SCR_CharacterControllerComponent m_ControllerComponent;
	protected PS_PlayableComponent m_cPlayableComponent;
	
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	override void AddToList(IEntity owner)
	{
		super.AddToList(owner);
		
		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(owner);
		m_cPlayableComponent = PS_PlayableComponent.Cast(m_eChimeraCharacter.FindComponent(PS_PlayableComponent));
		m_ControllerComponent = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
	}
	
	override bool UpdateMarker()
	{
		if (!super.UpdateMarker()) return false;
		
		if (!m_bDead)
		{
			if (!m_bWounded && m_ControllerComponent.IsUnconscious()) {
				m_hManualMarkerComponent.SetImage(m_rIconImageSet, "WoundedCharacter");
				m_bWounded = true;
			}
			if (m_bWounded && !m_ControllerComponent.IsUnconscious()) {
				m_hManualMarkerComponent.SetImage(m_rIconImageSet, "Character");
				m_bWounded = false;
			}
			
			if (m_ControllerComponent.IsDead()) {
				m_hManualMarkerComponent.SetImage(m_rIconImageSet, "DeadCharacter");
				m_bDead = true;
			}
		}
		else 
		{
			m_hManualMarkerComponent.SetOpacity(0.4);
		}
		
		return true;
	}
};