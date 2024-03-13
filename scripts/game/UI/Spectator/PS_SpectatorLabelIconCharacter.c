class PS_SpectatorLabelIconCharacter : PS_SpectatorLabelIcon
{
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected SCR_CharacterControllerComponent m_ControllerComponent;
	protected PS_PlayableComponent m_cPlayableComponent;
	protected SCR_EditableCharacterComponent m_EditableCharacterComponent;
	
	protected ImageWidget m_wSpectatorLabelIconBackground;
	protected ImageWidget m_wSpectatorLabelIconCircle;
	
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	override void HandlerAttached(Widget w)
	{
		m_wSpectatorLabelIconBackground = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconBackground"));
		m_wSpectatorLabelIconCircle = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconCircle"));
		
		super.HandlerAttached(w);
	}
	
	override void SetEntity(IEntity entity, string boneName)
	{
		super.SetEntity(entity, boneName);
		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(entity);
		m_cPlayableComponent = PS_PlayableComponent.Cast(m_eChimeraCharacter.FindComponent(PS_PlayableComponent));
		m_ControllerComponent = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
		m_EditableCharacterComponent = SCR_EditableCharacterComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_EditableCharacterComponent));
		
		SCR_Faction faction = SCR_Faction.Cast(m_eChimeraCharacter.GetFaction());
		if (faction)
		{
			m_wSpectatorLabelIcon.SetColor(faction.GetOutlineFactionColor());
			m_wSpectatorLabelIconBackground.SetColor(faction.GetFactionColor());
			m_wSpectatorLabelIconCircle.SetColor(faction.GetOutlineFactionColor());
		}
		
		SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
		m_wSpectatorLabelIcon.LoadImageTexture(0, uiInfo.GetIconPath());
	}
	
	override void UpdateLabel()
	{
		if (m_cPlayableComponent)
		{
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			int playerId = playableManager.GetPlayerByPlayable(m_cPlayableComponent.GetId());
			if (playerId > 0)
			{
				PlayerManager playerManager = GetGame().GetPlayerManager();
				string playerName = playerManager.GetPlayerName(playerId);
				if (playerName != "")
					m_wSpectatorLabelText.SetText(playerName);
			} else {
				m_wSpectatorLabelText.SetText(m_cPlayableComponent.GetName());
			}
		}
		
		if (!m_bDead)
		{
			if (!m_bWounded && m_ControllerComponent.IsUnconscious()) {
				m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Wounded");
				m_bWounded = true;
			}
			if (m_bWounded && !m_ControllerComponent.IsUnconscious()) {
				SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
				m_wSpectatorLabelIcon.LoadImageTexture(0, uiInfo.GetIconPath());
				m_bWounded = false;
			}
			
			if (m_ControllerComponent.IsDead()) {
				m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Dead");
				m_wSpectatorLabelIcon.SetOpacity(0.9);
				m_wSpectatorLabelBackground.SetOpacity(0.9);
				m_wSpectatorLabelIconBackground.SetColor(Color.Gray);
				m_wSpectatorLabelIconBackground.SetOpacity(0.2);
				m_bDead = true;
			}
		}
		else 
		{
			m_wRoot.SetOpacity(0.6);
		}
	}
}