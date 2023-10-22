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
	ButtonWidget m_wVoiceButton;
	ImageWidget m_wVoiceImage;
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
		m_wVoiceButton = ButtonWidget.Cast(w.FindAnyWidget("VoiceButton"));
		m_wVoiceImage = ImageWidget.Cast(w.FindAnyWidget("VoiceImage"));
		m_wPinImage = ImageWidget.Cast(w.FindAnyWidget("PinImage"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		SCR_ButtonBaseComponent kickButtonHandler = SCR_ButtonBaseComponent.Cast(m_wKickButton.FindHandler(SCR_ButtonBaseComponent));
		kickButtonHandler.m_OnClicked.Insert(KickButtonClicked);
		SCR_ButtonBaseComponent pinButtonHandler = SCR_ButtonBaseComponent.Cast(m_wPinButton.FindHandler(SCR_ButtonBaseComponent));
		pinButtonHandler.m_OnClicked.Insert(PinButtonClicked);
		SCR_ButtonBaseComponent voiceButtonHandler = SCR_ButtonBaseComponent.Cast(m_wVoiceButton.FindHandler(SCR_ButtonBaseComponent));
		voiceButtonHandler.m_OnClicked.Insert(VoiceButtonClicked);
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
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayer);
		if (factionKey != "") 
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
			m_wPlayerFactionColor.SetColor(faction.GetOutlineFactionColor());
			m_wPlayerFactionName.SetText(faction.GetFactionName());
		}else{
			m_wPlayerFactionColor.SetColor(Color.FromRGBA(0, 0, 0, 0));
			m_wPlayerFactionName.SetText("-");
		}
		
		m_wSelectionFactionColor.SetVisible(IsToggled());
		
		// if admin set player color
		m_wPlayerName.SetText(playerManager.GetPlayerName(m_iPlayer));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayer);
		if (playerRole == EPlayerRole.ADMINISTRATOR) m_wPlayerName.SetColor(Color.FromInt(0xfff2a34b));
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
		
		
		PlayerController playerController = GetGame().GetPlayerController();
		
		
		// Check OUR VoN is WE listen this player?
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		PermissionState mute = playerController.GetPlayerMutedState(m_iPlayer);
		PS_LobbyVoNComponent von;
		if (entity) von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		if (von && mute != PermissionState.DISALLOWED)
		{
			if (von.IsPlayerSpeech(m_iPlayer)) m_wVoiceImage.LoadImageFromSet(0, m_sImageSet, "sound-on");
			else m_wVoiceImage.LoadImageFromSet(0, m_sImageSet, "VON_directspeech");
		} else m_wVoiceImage.LoadImageFromSet(0, m_sImageSet, "sound-off");
		
		
		// Check is VoN the same.
		// faction
		FactionKey currentFactionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
		
		// group
		string groupName = "";
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		if (playableId != RplId.Invalid()) groupName = playableManager.GetGroupNameByPlayable(playableId);
		string currentGroupName = "";
		playableId = playableManager.GetPlayableByPlayer(playerController.GetPlayerId());
		if (playableId != RplId.Invalid()) currentGroupName = playableManager.GetGroupNameByPlayable(playableId);
		
		string VoNRoom = factionKey + groupName;
		string currentVoNRoom = currentFactionKey + currentGroupName;
		
		if (VoNRoom != currentVoNRoom) m_wVoiceImage.SetColor(Color.FromInt(0xff595959));
		else {
			m_wVoiceImage.SetColor(Color.FromInt(0xffffffff));
		}
		
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
	void KickButtonClicked(SCR_ButtonBaseComponent kickButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.KickPlayer(m_iPlayer);
	}
	
	// Admin may unpin player
	void PinButtonClicked(SCR_ButtonBaseComponent pinButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.UnpinPlayer(m_iPlayer);
	}
	
	void VoiceButtonClicked(SCR_ButtonBaseComponent pinButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController.GetPlayerId() == m_iPlayer) return;
		PermissionState mute = playerController.GetPlayerMutedState(m_iPlayer);
		if (mute == PermissionState.ALLOWED) playerController.SetPlayerMutedState(m_iPlayer, PermissionState.DISALLOWED);
		else playerController.SetPlayerMutedState(m_iPlayer, PermissionState.ALLOWED);
	}
	
}