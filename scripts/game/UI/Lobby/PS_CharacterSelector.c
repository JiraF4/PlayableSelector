// Widget displays info about playable character.
// Path: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)
// Lobby insert it into PS_RolesGroup widget

// State states...
enum PS_ECharacterState
{
	Empty,
	Player,
	Disconnected,
	Pin,
	Kick,
	Lock,
	Dead,
}

class PS_CharacterSelector : SCR_ButtonComponent
{
	// Const
	protected ResourceName m_sUIWrapper = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	const static ResourceName IMAGESET_PS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	protected ref Color m_DefaultColor = Color.White;
	protected ref Color m_AdminColor = Color.FromInt(0xfff2a34b);
	protected ref Color m_DeathColor = Color.FromInt(0xFF2c2c2c);
	protected ref Color m_ReadyColor = Color.Green;
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected PlayerManager m_PlayerManager;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iCurrentPlayerId;
	
	// Widgets
	protected ImageWidget m_wCharacterFactionColor;
	protected ImageWidget m_wUnitIcon;
	protected TextWidget m_wCharacterClassName;
	protected ImageWidget m_wStateIcon;
	protected ButtonWidget m_wStateButton;
	protected RichTextWidget m_wCharacterStatus;
	protected OverlayWidget m_wVoiceHideableButton;
	
	// Handlers
	protected SCR_ButtonBaseComponent m_StateButtonBaseComponent;
	protected PS_VoiceButton m_VoiceHideableButton;
	
	// Parameters
	protected PS_ECharacterState m_state;
	protected PS_CoopLobby m_CoopLobby;
	protected PS_RolesGroup m_RolesGroup;
	protected PS_PlayableContainer m_PlayableContainer;
	protected RplId m_iPlayableId;
	protected int m_iPlayableCallsign;
	protected string m_sPlayableCallsign;
	
	protected bool m_bDead;
	protected bool m_bPined;
	protected bool m_bDisconnected;
	protected EDamageState m_iDamageState;
	protected int m_iPlayerId;
	protected bool m_bAdmin;
	protected bool m_bReady;
	protected bool m_bCanKick;
	
	protected bool m_bStateClickSkip;
	
	// Cache parameters
	protected SCR_CharacterDamageManagerComponent m_CharacterDamageManagerComponent;
	protected SCR_Faction m_Faction;
	protected FactionKey m_sFactionKey;
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		if (!m_PlayerController)
			return;
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		
		// Widgets
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wCharacterClassName = TextWidget.Cast(w.FindAnyWidget("CharacterClassName"));
		m_wStateIcon = ImageWidget.Cast(w.FindAnyWidget("StateIcon"));
		m_wStateButton = ButtonWidget.Cast(w.FindAnyWidget("StateButton"));
		m_wCharacterStatus = RichTextWidget.Cast(w.FindAnyWidget("CharacterStatus"));
		m_wVoiceHideableButton = OverlayWidget.Cast(w.FindAnyWidget("VoiceHideableButton"));
		
		// Handlers
		m_StateButtonBaseComponent = SCR_ButtonBaseComponent.Cast(m_wStateButton.FindHandler(SCR_ButtonBaseComponent));
		m_VoiceHideableButton = PS_VoiceButton.Cast(m_wVoiceHideableButton.FindHandler(PS_VoiceButton));
		
		// Buttons
		//m_OnClicked.Insert(OnClicked);
		m_OnHover.Insert(OnHover);
		m_OnHoverLeave.Insert(OnHoverLeave);
		m_StateButtonBaseComponent.m_OnClicked.Insert(OnStateClicked);
		
		// Events
		m_PlayableControllerComponent.GetOnPlayerRoleChange().Insert(OnRoleChangeCurrent);
		m_PlayableManager.GetOnPlayerPlayableChange().Insert(OnPlayerPlayableChange);
	}
	
	override void HandlerDeattached(Widget w)
	{
		if (m_PlayableControllerComponent)
			m_PlayableControllerComponent.GetOnPlayerRoleChange().Remove(OnRoleChangeCurrent);
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnPlayerPlayableChange().Remove(OnPlayerPlayableChange);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetLobbyMenu(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	void SetRolesGroup(PS_RolesGroup rolesGroup)
	{
		m_RolesGroup = rolesGroup;
	}
	
	void SetPlayable(PS_PlayableContainer playableContainer)
	{
		m_PlayableContainer = playableContainer;
		m_iPlayableId = playableContainer.GetRplId();
		
		// Temp
		m_Faction = m_PlayableContainer.GetFaction();
		m_sFactionKey = m_Faction.GetFactionKey();
		
		// Initial setup
		m_PlayableContainer.SetIconTo(m_wUnitIcon);
		m_wCharacterFactionColor.SetColor(m_Faction.GetFactionColor());
		m_iPlayerId = m_PlayableManager.GetPlayerByPlayable(m_iPlayableId);
		if (m_iPlayerId > 0)
			m_bDisconnected = !m_PlayerManager.IsPlayerConnected(m_iPlayerId);
		m_iDamageState = playableContainer.GetDamageState();
		m_wCharacterClassName.SetText(m_PlayableContainer.GetName());
		m_iPlayableCallsign = m_PlayableManager.GetGroupCallsignByPlayable(m_iPlayableId);
		m_sPlayableCallsign = m_iPlayableCallsign.ToString();
		m_bDead = m_iDamageState == EDamageState.DESTROYED;
		
		UpdatePlayer(0, m_iPlayerId);
		UpdateStateIcon();
		
		// Events
		m_PlayableContainer.GetOnPlayerChange().Insert(UpdatePlayer);
		m_PlayableContainer.GetOnDamageStateChanged().Insert(UpdateDammage);
		m_PlayableContainer.GetOnUnregister().Insert(RemoveSelf);
		//m_PlayableContainer.GetOnPlayerDisconnected().Insert(OnDisconnected);
		m_PlayableContainer.GetOnPlayerConnected().Insert(OnConnected);
		m_PlayableContainer.GetOnPlayerStateChange().Insert(OnStateChange);
		m_PlayableContainer.GetOnPlayerPinChange().Insert(UpdatePined);
		m_PlayableContainer.GetOnPlayerRoleChange().Insert(OnRoleChange);
	}
	
	PS_PlayableContainer GetPlayable()
	{
		return m_PlayableContainer;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update character
	void UpdateDammage(EDamageState state)
	{
		m_iDamageState = state;
		m_bDead = m_iDamageState == EDamageState.DESTROYED;
		UpdateState();
	}
	
	void UpdatePined(bool pined)
	{
		m_bPined = pined;
		UpdateState();
	}
	
	void UpdatePlayer(int oldPlayerId, int playerId)
	{
		m_VoiceHideableButton.SetPlayer(playerId);
		
		if (m_iPlayerId != -2 && playerId == -2)
		{
			m_RolesGroup.UpdateLockedState(true);
			m_CoopLobby.AddFactionCount(m_Faction, 0, 0, 1);
		}
		if (m_iPlayerId == -2 && playerId != -2)
		{
			m_RolesGroup.UpdateLockedState(false);
			m_CoopLobby.AddFactionCount(m_Faction, 0, 0, -1);
		}
		
		PS_EPlayableControllerState state = m_PlayableManager.GetPlayerState(m_iPlayerId);
		m_bReady = state == PS_EPlayableControllerState.Ready;
		
		if (m_iPlayerId > 0 && playerId <= 0)
		{
			m_CoopLobby.AddFactionCount(m_Faction, -1, 0);
		}
		if (m_iPlayerId <= 0 && playerId > 0)
			m_CoopLobby.AddFactionCount(m_Faction, 1, 0);
		
		m_iPlayerId = playerId;
		
		m_bPined = m_PlayableManager.GetPlayerPin(m_iPlayerId);
		if (m_iPlayerId > 0)
			m_bDisconnected = !m_PlayerManager.IsPlayerConnected(m_iPlayerId);
		else
			m_bDisconnected = false;
		
		string playerName = m_PlayableManager.GetPlayerName(playerId);
		m_wCharacterStatus.SetText(playerName);
		
		m_bAdmin = SCR_Global.IsAdmin(playerId);
		
		if (oldPlayerId == m_iCurrentPlayerId || playerId == m_iCurrentPlayerId)
			m_CoopLobby.SetPreviewPlayable(m_iPlayableId, false);

		OnPlayerPlayableChange(playerId, m_iPlayableId);
		UpdateColor();
	}
	
	void RemoveSelf()
	{
		m_wRoot.RemoveFromHierarchy();
		m_RolesGroup.OnPlayableRemoved(m_PlayableContainer);
		m_CoopLobby.OnPlayableRemoved(m_PlayableContainer);
		if (m_iPlayerId == -2)
			m_RolesGroup.UpdateLockedState(false);
	}
	
	void OnPlayerPlayableChange(int playerId, RplId playbleId)
	{
		// Self kick
		if (m_iPlayerId == m_iCurrentPlayerId)
		{
			m_bCanKick = false;
			UpdateState();
			return;
		}
		
		// Admin can kick any
		m_bCanKick = PS_PlayersHelper.IsAdminOrServer();
		if (m_bCanKick)
		{
			UpdateState();
			return;
		}
		
		// get CURRENT PLAYER playable
		RplId currentPlayableId = m_PlayableManager.GetPlayableByPlayer(m_iCurrentPlayerId);
		if (currentPlayableId == RplId.Invalid())
		{
			m_bCanKick = false;
			UpdateState();
			return;
		}
		
		// Only group leader can kick (Longest event?)
		FactionKey factionKeyCurrent = m_PlayableManager.GetPlayerFactionKey(m_iCurrentPlayerId);
		SCR_AIGroup groupCurrent = m_PlayableManager.GetPlayerGroupByPlayable(currentPlayableId);
		FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(m_iPlayerId);
		SCR_AIGroup group = m_PlayableManager.GetPlayerGroupByPlayable(m_iPlayableId);
		m_bCanKick = m_PlayableManager.IsPlayerGroupLeader(m_iCurrentPlayerId)
			&& factionKeyCurrent == factionKey
			&& groupCurrent == group;
		
		UpdateState();
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Get
	int GetPlayerId()
	{
		return m_iPlayerId;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update player
	void OnDisconnected(int playerId, KickCauseCode cause = KickCauseCode.NONE, int timeout = -1)
	{
		m_bDisconnected = true;
		
		m_wCharacterStatus.SetColor(m_DeathColor);
		
		UpdateColor();
		UpdateState();
	}
	
	void OnConnected(int playerId)
	{
		m_bDisconnected = false;
		
		UpdateColor();
		UpdateState();
	}
	
	void OnStateChange(PS_EPlayableControllerState state)
	{
		m_bReady = state == PS_EPlayableControllerState.Ready;
		m_bDisconnected = state == PS_EPlayableControllerState.Disconected;
		
		UpdateColor();
	}
	
	void OnRoleChangeCurrent(int playerId, EPlayerRole roleFlags)
	{
		UpdateState(true);
	}
	
	void OnRoleChange(int playerId, EPlayerRole roleFlags)
	{
		m_bAdmin = roleFlags & EPlayerRole.ADMINISTRATOR ||
			roleFlags & EPlayerRole.SESSION_ADMINISTRATOR;
		
		if (m_bAdmin)
			m_wCharacterStatus.SetColor(m_AdminColor);
		else
			m_wCharacterStatus.SetColor(m_DefaultColor);
		
		UpdateState(true);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// State
	void UpdateColor()
	{
		if (m_bDisconnected)
			m_wCharacterStatus.SetColor(m_DeathColor);
		else if (m_bReady)
			m_wCharacterStatus.SetColor(m_ReadyColor);
		else if (m_bAdmin)
				m_wCharacterStatus.SetColor(m_AdminColor);
			else
				m_wCharacterStatus.SetColor(m_DefaultColor);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void UpdateState(bool forceIcons = false)
	{
		PS_ECharacterState state = PS_ECharacterState.Empty;
		if (m_bDead)
			state = PS_ECharacterState.Dead;
		else if (m_bDisconnected)
			state = PS_ECharacterState.Disconnected;
		else if (m_bPined)
			state = PS_ECharacterState.Pin;
		else if (m_iPlayerId == -2)
			state = PS_ECharacterState.Lock;
		//else if (m_bCanKick && m_iPlayerId >= 0 && m_iPlayerId != m_iCurrentPlayerId)
		//	state = PS_ECharacterState.Kick;
		else if (m_iPlayerId >= 0)
			state = PS_ECharacterState.Player;
		
		if (forceIcons || m_state != state)
		{
			m_state = state;
			UpdateStateIcon();
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void UpdateStateIcon()
	{
		m_wStateIcon.SetVisible(true);
		m_wStateButton.SetVisible(false);
		switch (m_state)
		{
			case PS_ECharacterState.Pin:
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "pinPlay");
				break;
			case PS_ECharacterState.Dead:
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "death");
				break;
			case PS_ECharacterState.Lock:
				m_wStateIcon.LoadImageFromSet(0, IMAGESET_PS, "Locked");
				break;
			case PS_ECharacterState.Kick:
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "kickCommandAlt");
				break;
			case PS_ECharacterState.Empty:
				m_wStateIcon.LoadImageFromSet(0, IMAGESET_PS, "Unlocked");
				m_wStateIcon.SetVisible(false);
				break;
			case PS_ECharacterState.Player:
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "player");
				break;
			case PS_ECharacterState.Disconnected:
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "disconnection");
				break;
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (button == 1)
		{
			OpenContext();
			return false;
		}
		if (button != 0)
			return false;
		
		OnClicked(this);
		return false;
	}
	void OnClicked(SCR_ButtonBaseComponent button)
	{
		if (m_bStateClickSkip)
		{
			m_bStateClickSkip = false;
			return;
		}
		
		int playerId = m_CoopLobby.GetSelectedPlayer();
		if (m_iPlayerId == -2)
		{
			m_CoopLobby.SetPreviewPlayable(m_iPlayableId, true);
			SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FAIL");
			return;
		}
		if (m_iPlayerId > 0 && playerId != m_iPlayerId)
		{
			m_CoopLobby.SetPreviewPlayable(m_iPlayableId, true);
			SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FAIL");
			return;
		}
		PS_PlayableContainer playableContainer = m_PlayableManager.GetPlayableById(m_iPlayableId);
		if (playableContainer.GetDamageState() == EDamageState.DESTROYED)
		{
			m_CoopLobby.SetPreviewPlayable(m_iPlayableId, true);
			SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FAIL");
			return;
		}
	
		SCR_EGameModeState gameState = m_GameModeCoop.GetState();
		if (!PS_PlayersHelper.IsAdminOrServer())
		{
			RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iCurrentPlayerId);
			if (gameState == SCR_EGameModeState.BRIEFING && playableId != RplId.Invalid())
			{
				m_CoopLobby.SetPreviewPlayable(m_iPlayableId, true);
				return;
			}
		}
		
		if (playerId != m_iPlayerId)
		{
			if (!CanJoinFaction())
			{
				SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
				ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("lmsg");
				invoker.Invoke(null, "Где баланс?");
				m_CoopLobby.SetPreviewPlayable(m_iPlayableId, true);
				return;
			}
			
			SCR_UISoundEntity.SoundEvent("SOUND_HUD_GADGET_SELECT");
			m_PlayableControllerComponent.MoveToVoNRoom(playerId, m_sFactionKey, m_sPlayableCallsign);
			m_PlayableControllerComponent.MoveToVoNRoom(playerId, m_sFactionKey, "#PS-VoNRoom_Faction");
			m_PlayableControllerComponent.ChangeFactionKey(playerId, m_sFactionKey);
			m_PlayableControllerComponent.SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);	
			m_PlayableControllerComponent.SetPlayerPlayable(playerId, m_iPlayableId);
		} else {
			SCR_UISoundEntity.SoundEvent("SOUND_HUD_GADGET_SELECT");
			m_PlayableControllerComponent.MoveToVoNRoom(playerId, m_sFactionKey, "#PS-VoNRoom_Faction");
			m_PlayableControllerComponent.ChangeFactionKey(playerId, "");
			m_PlayableControllerComponent.SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
			m_PlayableControllerComponent.SetPlayerPlayable(playerId, RplId.Invalid());
			if (PS_PlayersHelper.IsAdminOrServer())
				m_PlayableControllerComponent.UnpinPlayer(playerId);
		}
		
		if (PS_PlayersHelper.IsAdminOrServer() && playerId != m_iCurrentPlayerId && gameState == SCR_EGameModeState.GAME)
			m_PlayableControllerComponent.ForceSwitch(playerId);
		if (!PS_PlayersHelper.IsAdminOrServer() && playerId == m_iCurrentPlayerId && gameState == SCR_EGameModeState.BRIEFING)
			m_PlayableControllerComponent.SwitchToMenuServer(SCR_EGameModeState.BRIEFING);
	}
	
	void OnHover()
	{
		m_CoopLobby.SetPreviewPlayable(m_PlayableContainer.GetRplId(), false);
	}
	
	void OnHoverLeave()
	{
		m_CoopLobby.SetPreviewPlayable(RplId.Invalid(), false);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Context menu
	void OpenContext()
	{
		string playerName = PS_PlayableManager.GetInstance().GetPlayerName(m_iPlayerId);
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(m_CoopLobby.GetRootWidget(), playerName);
		contextMenu.ActionOpenInventory(m_iPlayableId).Insert(OnActionOpenInventory);
		
		if (m_iPlayerId > 0)
		{
			if (PS_PlayersHelper.IsAdminOrServer())
			{
				contextMenu.ActionGetArmaId(m_iPlayerId);
			}
			if (m_iPlayerId != m_iCurrentPlayerId)
			{
				PermissionState mute = PermissionState.DISALLOWED;
				SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
				if (socialComp.IsMuted(m_iPlayerId))
					contextMenu.ActionUnmute(m_iPlayerId);
				else
					contextMenu.ActionMute(m_iPlayerId);
				
				if (m_bCanKick && m_iPlayerId >= 0 && m_iPlayerId != m_iCurrentPlayerId)
					contextMenu.ActionFreeSlot(m_iPlayableId).Insert(OnActionFreeSlot);
				
				if (PS_PlayersHelper.IsAdminOrServer())
				{
					contextMenu.ActionDirectMessage(m_iPlayerId);
					contextMenu.ActionKick(m_iPlayerId);
					
					if (m_PlayableManager.GetPlayerPin(m_iPlayerId))
						contextMenu.ActionUnpin(m_iPlayerId);
					else
						contextMenu.ActionPin(m_iPlayerId);
				}
			}
			if (m_CoopLobby.GetSelectedPlayer() != m_iPlayerId && PS_PlayersHelper.IsAdminOrServer())
			{
				contextMenu.ActionPlayerSelect(m_iPlayerId);
			}
		}
		
		if (PS_PlayersHelper.IsAdminOrServer())
			if (m_iPlayerId != -2)
				contextMenu.ActionLock(m_iPlayableId).Insert(OnActionLock);
			else
				contextMenu.ActionUnlock(m_iPlayableId).Insert(OnActionUnlock);
	}
	void OnActionOpenInventory(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		m_CoopLobby.SetPreviewPlayable(contextActionDataPlayable.GetPlayableId(), true);
	}
	void OnActionLock(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_ON");
		if (m_iPlayerId > 0)
			OnActionFreeSlot(contextAction, contextActionDataPlayable);
		m_PlayableControllerComponent.SetPlayablePlayer(contextActionDataPlayable.GetPlayableId(), -2);
	}
	void OnActionUnlock(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_OFF");
		m_PlayableControllerComponent.SetPlayablePlayer(contextActionDataPlayable.GetPlayableId(), -1);
	}
	void OnActionFreeSlot(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		if (m_iPlayerId <= 0)
			return;
		
		SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
		m_PlayableControllerComponent.MoveToVoNRoom(m_iPlayerId, m_sFactionKey, "#PS-VoNRoom_Faction");
		m_PlayableControllerComponent.ChangeFactionKey(m_iPlayerId, "");
		m_PlayableControllerComponent.SetPlayerState(m_iPlayerId, PS_EPlayableControllerState.NotReady);
		m_PlayableControllerComponent.SetPlayerPlayable(m_iPlayerId, -1);
		if (PS_PlayersHelper.IsAdminOrServer())
			m_PlayableControllerComponent.UnpinPlayer(m_iPlayerId);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void OnStateClicked(SCR_ButtonBaseComponent button)
	{
		m_bStateClickSkip = true;
		switch (m_state)
		{
			case PS_ECharacterState.Pin:
				SCR_UISoundEntity.SoundEvent("SOUND_E_LAYER_BACK");
				m_PlayableControllerComponent.UnpinPlayer(m_iPlayerId);
				break;
			case PS_ECharacterState.Dead:
				break;
			case PS_ECharacterState.Lock:
				SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_OFF");
				m_PlayableControllerComponent.SetPlayablePlayer(m_iPlayableId, -1);
				break;
			case PS_ECharacterState.Kick:
				SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
				m_PlayableControllerComponent.MoveToVoNRoom(m_iPlayerId, m_sFactionKey, "#PS-VoNRoom_Faction");
				m_PlayableControllerComponent.ChangeFactionKey(m_iPlayerId, "");
				m_PlayableControllerComponent.SetPlayerState(m_iPlayerId, PS_EPlayableControllerState.NotReady);
				m_PlayableControllerComponent.SetPlayerPlayable(m_iPlayerId, -1);
				if (PS_PlayersHelper.IsAdminOrServer())
					m_PlayableControllerComponent.UnpinPlayer(m_iPlayerId);
				break;
			case PS_ECharacterState.Empty:
				SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_ON");
				if (m_iPlayerId > 0)
					m_PlayableControllerComponent.SetPlayerState(m_iPlayerId, PS_EPlayableControllerState.NotReady);
				m_PlayableControllerComponent.SetPlayablePlayer(m_iPlayableId, -2);
			   break;
			case PS_ECharacterState.Player:
				break;
			case PS_ECharacterState.Disconnected:
				SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_KICK");
				m_PlayableControllerComponent.MoveToVoNRoom(m_iPlayerId, m_PlayableManager.GetPlayerFactionKey(m_iPlayerId), "#PS-VoNRoom_Faction");
				m_PlayableControllerComponent.ChangeFactionKey(m_iPlayerId, "");
				m_PlayableControllerComponent.SetPlayerPlayable(m_iPlayerId, RplId.Invalid());
				break;
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	bool CanJoinFaction()
	{
		// Check faction balance
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (m_PlayableContainer)
		{
			if (!PS_PlayersHelper.IsAdminOrServer() && !gameModeCoop.CanJoinFaction(m_sFactionKey, m_PlayableManager.GetPlayerFactionKey(m_iCurrentPlayerId)))
				return false;
		}
		return true;
	}
}




















