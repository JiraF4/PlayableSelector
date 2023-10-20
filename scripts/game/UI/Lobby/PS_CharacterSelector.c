// Widget displays info about playable character.
// Path: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)
// Lobby insert it into PS_RolesGroup widget

class PS_CharacterSelector : SCR_ButtonImageComponent
{
	protected PS_PlayableComponent m_playable;
	
	ImageWidget m_wCharacterFactionColor;
	ImageWidget m_wCharacterStatusIcon;
	ImageWidget m_wUnitIcon;
	TextWidget m_wCharacterClassName;
	TextWidget m_wCharacterStatus;
	
	protected ResourceName m_sUIWrapper = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wCharacterStatusIcon = ImageWidget.Cast(w.FindAnyWidget("CharacterStatusIcon"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wCharacterClassName = TextWidget.Cast(w.FindAnyWidget("CharacterClassName"));
		m_wCharacterStatus = TextWidget.Cast(w.FindAnyWidget("CharacterStatus"));
	}
	
	void SetPlayable(PS_PlayableComponent playable)
	{
		m_playable = playable;
		UpdatePlayableInfo();
	}
	
	void UpdatePlayableInfo()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
		SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		
		// Set base playable info role icon, faction color, custome/role name
		m_wUnitIcon.LoadImageTexture(0, uiInfo.GetIconPath());
		m_wCharacterFactionColor.SetColor(faction.GetFactionColor());
		m_wCharacterClassName.SetText(m_playable.GetName());
		
		int playerId = playableManager.GetPlayerByPlayable(m_playable.GetId());
		PlayerManager playerManager = GetGame().GetPlayerManager();
		string playerName = playerManager.GetPlayerName(playerId);
		
		// Set current state Dead, Selected by player, Selected by disconnected player
		if (character.GetDamageManager().IsDestroyed()) 
		{
			m_wCharacterStatus.SetText("Dead");
			SetImage(m_sUIWrapper, "death");
		} else if (playerId != -1 && playerName == "")
		{
			m_wCharacterStatus.SetText("Disconnected");
			SetImage(m_sUIWrapper, "disconnection");
		} else {
			if (playerId != 0) 
			{
				m_wCharacterStatus.SetText(playerName);
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