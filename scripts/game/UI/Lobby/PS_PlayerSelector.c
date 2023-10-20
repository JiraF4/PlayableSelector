// Widget displays info about connected player.
// Path: {B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_PlayerSelector : SCR_WLibComponentBase
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wPlayerFactionColor;
	TextWidget m_wPlayerName;
	TextWidget m_wPlayerFactionName;
	ImageWidget m_wReadyImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wPlayerFactionColor = ImageWidget.Cast(w.FindAnyWidget("PlayerFactionColor"));
		m_wPlayerName = TextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wPlayerFactionName = TextWidget.Cast(w.FindAnyWidget("PlayerFactionName"));
		m_wReadyImage = ImageWidget.Cast(w.FindAnyWidget("ReadyImage"));
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayer = playerId;
		UpdatePlayerInfo();
	}
	
	void UpdatePlayerInfo()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		int playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		SCR_ChimeraCharacter character;
		if (playableId != RplId.Invalid()) character = SCR_ChimeraCharacter.Cast(playableManager.GetPlayableById(playableId).GetOwner());
		if (character != null) 
		{
			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			m_wPlayerFactionColor.SetColor(faction.GetFactionColor());
			m_wPlayerFactionName.SetText(faction.GetFactionName());
		}else{
			m_wPlayerFactionColor.SetColor(Color.FromRGBA(0, 0, 0, 0));
			m_wPlayerFactionName.SetText("-");
		}
		m_wPlayerName.SetText(playerManager.GetPlayerName(m_iPlayer));
		
		
		m_wReadyImage.SetVisible(PS_PlayableManager.GetInstance().GetPlayerState(m_iPlayer) != PS_EPlayableControllerState.NotReady);
	}
}