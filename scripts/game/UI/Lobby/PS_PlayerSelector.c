// Widget displays info about connected player.
// Path: {B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_PlayerSelector : SCR_ButtonImageComponent
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wPlayerFactionColor;
	ImageWidget m_wSelectionFactionColor;
	TextWidget m_wPlayerName;
	TextWidget m_wPlayerFactionName;
	ImageWidget m_wReadyImage;
	ButtonWidget m_wKickButton;
	ButtonWidget m_wPinButton;
	ImageWidget m_wPinImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wPlayerFactionColor = ImageWidget.Cast(w.FindAnyWidget("PlayerFactionColor"));
		m_wSelectionFactionColor = ImageWidget.Cast(w.FindAnyWidget("PlayerSelectionColor"));
		m_wPlayerName = TextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wPlayerFactionName = TextWidget.Cast(w.FindAnyWidget("PlayerFactionName"));
		m_wReadyImage = ImageWidget.Cast(w.FindAnyWidget("ReadyImage"));
		m_wKickButton = ButtonWidget.Cast(w.FindAnyWidget("KickButton"));
		m_wPinButton = ButtonWidget.Cast(w.FindAnyWidget("PinButton"));
		m_wPinImage = ImageWidget.Cast(w.FindAnyWidget("PinImage"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		SCR_ButtonComponent kickButtonHandler = SCR_ButtonComponent.Cast(m_wKickButton.FindHandler(SCR_ButtonComponent));
		kickButtonHandler.m_OnClicked.Insert(kickButtonClicked);
		SCR_ButtonComponent pinButtonHandler = SCR_ButtonComponent.Cast(m_wPinButton.FindHandler(SCR_ButtonComponent));
		pinButtonHandler.m_OnClicked.Insert(pinButtonClicked);
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayer = playerId;
		UpdatePlayerInfo();
	}
	
	int GetPlayerId()
	{
		return m_iPlayer;
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
		
		m_wSelectionFactionColor.SetVisible(IsToggled());
		
		// if admin set player color
		m_wPlayerName.SetText(playerManager.GetPlayerName(m_iPlayer));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayer);
		if (playerRole == EPlayerRole.ADMINISTRATOR) m_wPlayerName.SetColor(Color.FromInt(0xffe9c543));
		else m_wPlayerName.SetColor(Color.FromInt(0xffffffff));
		
		// If admin show kick button for non admins
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		m_wKickButton.SetVisible(currentPlayerRole == EPlayerRole.ADMINISTRATOR && playerRole != EPlayerRole.ADMINISTRATOR);
		
		PS_EPlayableControllerState state = PS_PlayableManager.GetInstance().GetPlayerState(m_iPlayer);
		
		if (state == PS_EPlayableControllerState.NotReady) m_wReadyImage.LoadImageFromSet(0, m_sImageSet, "dotsMenu");
		if (state == PS_EPlayableControllerState.Ready) m_wReadyImage.LoadImageFromSet(0, m_sImageSet, "check");
		if (state == PS_EPlayableControllerState.Disconected) m_wReadyImage.LoadImageFromSet(0, m_sImageSet, "disconnection");
		if (state == PS_EPlayableControllerState.Playing) m_wReadyImage.LoadImageFromSet(0, m_sImageSet, "characters");
		
		// If pinned show pinImage or pinButton for admins
		if (playableManager.GetPlayerPin(m_iPlayer))
		{
			m_wPinImage.SetVisible(currentPlayerRole != EPlayerRole.ADMINISTRATOR);
			m_wPinButton.SetVisible(currentPlayerRole == EPlayerRole.ADMINISTRATOR);
		} else 
		{
			m_wPinImage.SetVisible(false);
			m_wPinButton.SetVisible(false);
		}
	}
	
	
	// -------------------- Buttons events --------------------
	// Admin may kick player
	void kickButtonClicked(SCR_ButtonBaseComponent kickButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.KickPlayer(m_iPlayer);
	}
	
	// Admin may unpin player
	void pinButtonClicked(SCR_ButtonBaseComponent pinButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.UnpinPlayer(m_iPlayer);
	}
	
}