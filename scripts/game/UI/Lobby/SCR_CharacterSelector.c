class SCR_CharacterSelector : SCR_ButtonImageComponent
{
	protected SCR_PlayableComponent m_playable;
	
	ImageWidget m_wCharacterFactionColor;
	ImageWidget m_wCharacterStatusIcon;
	TextWidget m_wCharacterClassName;
	TextWidget m_wCharacterStatus;
	
	protected ResourceName m_sUIWrapper = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wCharacterStatusIcon = ImageWidget.Cast(w.FindAnyWidget("CharacterStatusIcon"));
		m_wCharacterClassName = TextWidget.Cast(w.FindAnyWidget("CharacterClassName"));
		m_wCharacterStatus = TextWidget.Cast(w.FindAnyWidget("CharacterStatus"));
	}
	
	void SetPlayable(SCR_PlayableComponent playable)
	{
		m_playable = playable;
		UpdatePlayableInfo();
	}
	
	void UpdatePlayableInfo()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
		SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
		
		m_wCharacterFactionColor.SetColor(faction.GetFactionColor());
		m_wCharacterClassName.SetText(m_playable.GetName());
		if (character.GetDamageManager().IsDestroyed()) 
		{
			m_wCharacterStatus.SetText("Dead");
			SetImage(m_sUIWrapper, "death");
		}else{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			int playerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(character);
			if (playerId != 0) 
			{
				m_wCharacterStatus.SetText(playerManager.GetPlayerName(playerId));
				SetImage(m_sUIWrapper, "player");
			}else{
				m_wCharacterStatus.SetText("-");
				SetImage(m_sUIWrapper, "careerCircleOutline");
			}
		}
	}
	
	int GetPlayableId()
	{
		return m_playable.GetId();
	}
}