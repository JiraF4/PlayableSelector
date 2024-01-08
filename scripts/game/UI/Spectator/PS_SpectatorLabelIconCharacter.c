class PS_SpectatorLabelIconCharacter : PS_SpectatorLabelIcon
{
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected PS_PlayableComponent m_cPlayableComponent;
	
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	override void SetEntity(IEntity entity, string boneName)
	{
		super.SetEntity(entity, boneName);
		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(entity);
		m_cPlayableComponent = PS_PlayableComponent.Cast(m_eChimeraCharacter.FindComponent(PS_PlayableComponent));
		
		SCR_Faction faction = SCR_Faction.Cast(m_eChimeraCharacter.GetFaction());
		if (faction)
		{
			m_wSpectatorLabelIcon.SetColor(faction.GetOutlineFactionColor());
		}
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
			SCR_CharacterControllerComponent characterDamageComp = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
			
			if (!m_bWounded && characterDamageComp.IsUnconscious()) {
				m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Wounded");
				m_bWounded = true;
			}
			if (m_bWounded && !characterDamageComp.IsUnconscious()) {
				m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Player");
				m_bWounded = false;
			}
			
			if (characterDamageComp.IsDead()) {
				m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Dead");
				m_bDead = true;
			}
		}
		else 
		{
			m_wRoot.SetOpacity(0.6);
		}
	}
}