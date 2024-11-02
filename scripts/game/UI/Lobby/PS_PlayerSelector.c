// Widget displays info about connected player.
// Path: {B55DD7054C5892AE}UI/Lobby/PlayerSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_PlayerSelector : SCR_ButtonBaseComponent
{
	// Const
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	protected ref Color m_DefaultColor = Color.White;
	protected ref Color m_AdminColor = Color.FromInt(0xfff2a34b);
	protected ref Color m_DeathColor = Color.FromInt(0xFF2c2c2c);
	protected ref Color m_ReadyColor = Color.Green;
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PlayerManager m_PlayerManager;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iCurrentPlayerId;
	
	// Vars
	protected int m_iPlayerId;
	protected PS_PlayersList m_PlayersList;
	protected PS_CoopLobby m_CoopLobby;
	protected PS_ECharacterState m_iState;
	
	// Widgets
	protected ImageWidget m_wPlayerFactionColor;
	protected RichTextWidget m_wPlayerName;
	protected OverlayWidget m_wVoiceHideableButton;
	protected ButtonWidget m_wPinButton;
	protected ImageWidget m_wPinImage;
	protected ButtonWidget m_wKickButton;
	protected TextWidget m_wPlayerGroupName;
	protected ImageWidget m_wReadyImage;
	protected ImageWidget m_wImageCurrent;
	
	// Handlers
	protected PS_VoiceButton m_VoiceHideableButton;
	protected SCR_ButtonBaseComponent m_PinButton;
	protected SCR_ButtonBaseComponent m_KickButton;
	
	protected bool m_bButtonClickSkip;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_PlayerManager = GetGame().GetPlayerManager();
		m_iPlayerId = m_PlayerController.GetPlayerId();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		
		// Widgets
		m_wPlayerFactionColor = ImageWidget.Cast(w.FindAnyWidget("PlayerFactionColor"));
		m_wPlayerName = RichTextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wVoiceHideableButton = OverlayWidget.Cast(w.FindAnyWidget("VoiceHideableButton"));
		m_wPinButton = ButtonWidget.Cast(w.FindAnyWidget("PinButton"));
		m_wPinImage = ImageWidget.Cast(w.FindAnyWidget("PinImage"));
		m_wKickButton = ButtonWidget.Cast(w.FindAnyWidget("KickButton"));
		m_wPlayerGroupName = TextWidget.Cast(w.FindAnyWidget("PlayerGroupName"));
		m_wReadyImage = ImageWidget.Cast(w.FindAnyWidget("ReadyImage"));
		m_wImageCurrent = ImageWidget.Cast(w.FindAnyWidget("ImageCurrent"));
		
		// Handlers
		m_VoiceHideableButton = PS_VoiceButton.Cast(m_wVoiceHideableButton.FindHandler(PS_VoiceButton));
		m_KickButton = SCR_ButtonBaseComponent.Cast(m_wKickButton.FindHandler(SCR_ButtonBaseComponent));
		m_PinButton = SCR_ButtonBaseComponent.Cast(m_wPinButton.FindHandler(SCR_ButtonBaseComponent));
		
		// Buttons
		m_OnClicked.Insert(OnClicked);
		m_KickButton.m_OnClicked.Insert(OnClickedKick);
		m_PinButton.m_OnClicked.Insert(OnClickedPin);
		
		// Events
		m_GameModeCoop.GetOnPlayerConnected().Insert(UpdatePlayerName);
		//m_GameModeCoop.GetOnPlayerDisconnected().Insert(RemovePlayer);
		m_GameModeCoop.GetOnPlayerRoleChange().Insert(UpdatePlayerRole);
		m_PlayableManager.GetOnFactionChange().Insert(UpdatePlayerFaction);
		m_PlayableManager.GetOnPlayerPinChange().Insert(UpdatePlayerPin);
		m_PlayableManager.GetOnPlayerStateChange().Insert(UpdatePlayerState);
		m_PlayableManager.GetOnPlayerPlayableChange().Insert(UpdatePlayerPlayable);
	}
	
	override void HandlerDeattached(Widget w)
	{
		if (m_GameModeCoop)
		{
			m_GameModeCoop.GetOnPlayerConnected().Remove(UpdatePlayerName);
			//m_GameModeCoop.GetOnPlayerDisconnected().Remove(RemovePlayer);
			m_GameModeCoop.GetOnPlayerRoleChange().Remove(UpdatePlayerRole);
		}
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnFactionChange().Remove(UpdatePlayerFaction);
			m_PlayableManager.GetOnPlayerPinChange().Remove(UpdatePlayerPin);
			m_PlayableManager.GetOnPlayerStateChange().Remove(UpdatePlayerState);
			m_PlayableManager.GetOnPlayerPlayableChange().Remove(UpdatePlayerPlayable);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetPlayer(int playerId)
	{
		m_iPlayerId = playerId;
		
		// Init
		bool pin = m_PlayableManager.GetPlayerPin(m_iPlayerId);
		FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(m_iPlayerId);
		PS_EPlayableControllerState state = m_PlayableManager.GetPlayerState(m_iPlayerId);
		RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iPlayerId);
		m_VoiceHideableButton.SetPlayer(playerId);
		
		m_wImageCurrent.SetVisible(playerId == m_CoopLobby.GetSelectedPlayer());
		m_wKickButton.SetVisible(playerId != m_iCurrentPlayerId && PS_PlayersHelper.IsAdminOrServer());
		UpdatePlayerName(playerId);
		UpdatePlayerFaction(playerId, factionKey, factionKey);
		UpdatePlayerPin(playerId, pin);
		UpdatePlayerState(playerId, state);
		UpdatePlayerPlayable(playerId, playableId);
	}
	
	void SetPlayersList(PS_PlayersList playersList)
	{
		m_PlayersList = playersList;
	}
	
	void SetCoopLobby(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update
	void UpdatePlayerName(int playerId)
	{
		if (playerId != m_iPlayerId)
			return;
		
		string playerName = m_PlayableManager.GetPlayerName(m_iPlayerId);
		m_wPlayerName.SetText(playerName);
	}
	
	void UpdatePlayerFaction(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		if (playerId != m_iPlayerId)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));
		if (!faction)
			m_wPlayerFactionColor.SetColor(Color.Gray);
		else
			m_wPlayerFactionColor.SetColor(faction.GetFactionColor());
		
		int index = -1;
		if (playerId == m_iCurrentPlayerId)
			index = -100;
		else
		{
			if (faction)
				index = m_FactionManager.GetFactionIndex(faction);
		}
		m_wRoot.SetZOrder(index);
	}
	
	void UpdatePlayerPin(int playerId, bool pin)
	{
		if (playerId != m_iPlayerId)
			return;
		
		if (pin)
		{
			if (PS_PlayersHelper.IsAdminOrServer())
				m_wPinButton.SetVisible(pin);
			else
				m_wPinImage.SetVisible(pin);
		} else {
			m_wPinImage.SetVisible(false);
			m_wPinButton.SetVisible(false);
		}
	}
	
	void UpdatePlayerState(int playerId, PS_EPlayableControllerState state)
	{
		if (playerId != m_iPlayerId)
			return;
		
		m_iState = state;
		
		UpdateColor();
		
		if (state == PS_EPlayableControllerState.Disconected)
			RemovePlayer(playerId);
	}
	
	void UpdatePlayerPlayable(int playerId, RplId playableId)
	{
		if (playerId != m_iPlayerId)
			return;
		
		PS_PlayableComponent playableComponent = m_PlayableManager.GetPlayableById(playableId);
		SCR_AIGroup group = m_PlayableManager.GetPlayerGroupByPlayable(playableId);
		string groupName = PS_GroupHelper.GetGroupFullName(group);
		
		m_wPlayerGroupName.SetText(groupName);
	}
	
	void UpdatePlayerRole(int playerId, EPlayerRole roleFlags)
	{
		if (playerId == m_iCurrentPlayerId)
			m_wKickButton.SetVisible(m_iCurrentPlayerId != m_iPlayerId && PS_PlayersHelper.IsAdminOrServer());
		
		if (playerId != m_iPlayerId)
			return;
		
		UpdateColor();
	}
	
	void RemovePlayer(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		if (playerId != m_iPlayerId)
			return;
		
		m_wRoot.RemoveFromHierarchy();
		m_PlayersList.OnPlayerRemoved(m_iPlayerId);
		m_CoopLobby.OnPlayerRemoved(m_iPlayerId);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Etc
	void UpdateColor()
	{
		if (m_iState == PS_EPlayableControllerState.Disconected)
			m_wPlayerName.SetColor(m_DeathColor);
		else if (m_iState == PS_EPlayableControllerState.Ready)
			m_wPlayerName.SetColor(m_ReadyColor);
		else
			if (SCR_Global.IsAdmin(m_iPlayerId))
				m_wPlayerName.SetColor(m_AdminColor);
			else
				m_wPlayerName.SetColor(m_DefaultColor);
	}
	
	void Deselect()
	{
		m_wImageCurrent.SetVisible(false);
	}
	void Select()
	{
		m_wImageCurrent.SetVisible(true);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClicked(SCR_ButtonBaseComponent button)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		if (m_iPlayerId != m_CoopLobby.GetSelectedPlayer())
			m_CoopLobby.SetSelectedPlayer(m_iPlayerId);
	}
	
	void OnClickedKick(SCR_ButtonBaseComponent button)
	{
		AudioSystem.PlaySound("{5EF75EB4A836831F}Sounds/Explosions/_SharedData/Bodies/Explosion_Body_TNT_Far_01.wav");
		m_PlayableControllerComponent.KickPlayer(m_iPlayerId);
	}
	
	void OnClickedPin(SCR_ButtonBaseComponent button)
	{
		AudioSystem.PlaySound("{63DB9ACDD10DB801}Sounds/UI/Samples/Editor/UI_E_Layer_Edit_Back.wav");
		m_PlayableControllerComponent.UnpinPlayer(m_iPlayerId);
	}
}