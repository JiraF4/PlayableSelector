class SCR_PlayerSelector : SCR_WLibComponentBase
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
		SCR_PlayerController controller = SCR_PlayerController.Cast(playerManager.GetPlayerController(m_iPlayer));
		SCR_PlayableControllerComponent playableController = SCR_PlayableControllerComponent.Cast(controller.FindComponent(SCR_PlayableControllerComponent));
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(controller.GetControlledEntity());
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
		
		
		m_wReadyImage.SetVisible(playableController.GetReady());
	}
}