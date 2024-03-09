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
	protected PS_PlayableComponent m_playable;
	
	ImageWidget m_wCharacterFactionColor;
	ImageWidget m_wCharacterStatusIcon;
	ImageWidget m_wUnitIcon;
	ImageWidget m_wStateIcon;
	TextWidget m_wCharacterClassName;
	TextWidget m_wCharacterStatus;
	ButtonWidget m_wDisconnectionButton;
	
	ButtonWidget m_wStateButton;
	
	PS_VoiceButton m_wVoiceHideableButton;
	
	PS_ECharacterState characterState = PS_ECharacterState.Empty;
	
	protected ResourceName m_sUIWrapper = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wCharacterStatusIcon = ImageWidget.Cast(w.FindAnyWidget("CharacterStatusIcon"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wCharacterClassName = TextWidget.Cast(w.FindAnyWidget("CharacterClassName"));
		m_wCharacterStatus = TextWidget.Cast(w.FindAnyWidget("CharacterStatus"));
		m_wStateIcon = ImageWidget.Cast(w.FindAnyWidget("StateIcon"));
		
		Widget voiceHideableButtonWidget = Widget.Cast(w.FindAnyWidget("VoiceHideableButton"));
		m_wVoiceHideableButton = PS_VoiceButton.Cast(voiceHideableButtonWidget.FindHandler(PS_VoiceButton));
		
		m_wStateButton = ButtonWidget.Cast(w.FindAnyWidget("StateButton"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		SCR_ButtonBaseComponent stateButtonHandler = SCR_ButtonBaseComponent.Cast(m_wStateButton.FindHandler(SCR_ButtonBaseComponent));
		stateButtonHandler.m_OnClicked.Insert(StateButtonClicked);
	}
	
	void SetPlayable(PS_PlayableComponent playable)
	{
		m_playable = playable;
		UpdatePlayableInfo();
	}
	
	bool IsLocked()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		int playerId = playableManager.GetPlayerByPlayable(m_playable.GetId());
		return playerId == -2;
	}
	
	void UpdatePlayableInfo()
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// player data
		int playerId = playableManager.GetPlayerByPlayable(m_playable.GetId());
		FactionKey factionKey = playableManager.GetPlayerFactionKey(playerId);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		string playerName = playerManager.GetPlayerName(playerId);
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerId);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		RplId currentPlayableId = playableManager.GetPlayableByPlayer(currentPlayerId);
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		// Set base playable info role icon, faction color, custome/role name
		m_wUnitIcon.LoadImageTexture(0, uiInfo.GetIconPath());
		m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "server-unlocked");
		m_wCharacterStatus.SetText("-");
		if (faction) m_wCharacterFactionColor.SetColor(faction.GetOutlineFactionColor());
		else m_wCharacterFactionColor.SetColor(Color.FromInt(0xFF2c2c2c));
		m_wCharacterClassName.SetText(m_playable.GetName());
		m_wCharacterStatus.SetColor(Color.FromInt(0xFFFFFFFF));
		m_wStateButton.SetVisible(PS_PlayersHelper.IsAdminOrServer());
		characterState = PS_ECharacterState.Empty;
		
		// Voice
		m_wVoiceHideableButton.SetPlayer(playerId);
		m_wVoiceHideableButton.Update();
		
		// Dead
		if (character.GetDamageManager().IsDestroyed()) 
		{
			m_wCharacterStatus.SetText("Dead");
			m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "death");
			
			m_wStateIcon.SetColor(Color.FromInt(0xFF2c2c2c));
			m_wUnitIcon.SetColor(Color.FromInt(0xFF2c2c2c));
			m_wCharacterClassName.SetColor(Color.FromInt(0xFF2c2c2c));
			m_wCharacterStatus.SetColor(Color.FromInt(0xFF2c2c2c));
			
			characterState = PS_ECharacterState.Dead;
			
			return;
		}
		// Disconnected
		if (playerId > 0 && playerName == "")
		{
			m_wCharacterStatus.SetText("Disconnected");
			m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "disconnection");
			
			characterState = PS_ECharacterState.Disconnected;
			
			return;
		}
		// Player
		if (playerId > 0) 
		{
			m_wCharacterStatus.SetText(playerName);
			
			bool showKick = PS_PlayersHelper.IsAdminOrServer();
			if (playableManager.IsPlayerGroupLeader(currentPlayerController.GetPlayerId()))
			{
				if (!showKick)
					if (playableManager.GetGroupCallsignByPlayable(m_playable.GetId()) == playableManager.GetGroupCallsignByPlayable(currentPlayableId))
						if (playableManager.GetPlayerFactionKey(playerId) == playableManager.GetPlayerFactionKey(currentPlayerController.GetPlayerId()))
							showKick = true;
			}
			if (m_playable.GetId() == currentPlayableId)
				showKick = false;
			
			if (playableManager.GetPlayerState(playerId) == PS_EPlayableControllerState.Ready)
				m_wCharacterStatus.SetColor(Color.FromInt(0xFF2eee41));
			
			if (playableManager.GetPlayerPin(playerId)) {
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "pinPlay");
				m_wStateButton.SetVisible(PS_PlayersHelper.IsAdminOrServer());
				characterState = PS_ECharacterState.Pin;
			} else if (showKick) {
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "kickCommandAlt");
				m_wStateButton.SetVisible(true);
				characterState = PS_ECharacterState.Kick;
			} else {
				m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "player");
				characterState = PS_ECharacterState.Player;
			}
			
			return;
		}
		// Lock
		if (playerId == -2)
		{
			m_wStateIcon.LoadImageFromSet(0, m_sUIWrapper, "server-locked");
			characterState = PS_ECharacterState.Lock;
		}
	}
	
	int GetPlayableId()
	{
		return m_playable.GetId();
	}
	
	
	// -------------------- Buttons events --------------------
	//
	void StateButtonClicked(SCR_ButtonBaseComponent stateButton)
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// player data
		int playerId = playableManager.GetPlayerByPlayable(m_playable.GetId());
		FactionKey factionKey = playableManager.GetPlayerFactionKey(playerId);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		string playerName = playerManager.GetPlayerName(playerId);
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerId);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		RplId currentPlayableId = playableManager.GetPlayableByPlayer(currentPlayerId);
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		switch (characterState)
		{
			case PS_ECharacterState.Pin:
				currentPlayableController.UnpinPlayer(playableManager.GetPlayerByPlayable(m_playable.GetId()));
				break;
			case PS_ECharacterState.Dead:
				
				break;
			case PS_ECharacterState.Lock:
				currentPlayableController.SetPlayablePlayer(m_playable.GetId(), -1);
				break;
			case PS_ECharacterState.Kick:
				currentPlayableController.SetPlayerPlayable(playerId, -1);
				currentPlayableController.MoveToVoNRoom(playerId, playableManager.GetPlayerFactionKey(playerId), "#PS-VoNRoom_Faction");
				break;
			case PS_ECharacterState.Empty:
				currentPlayableController.SetPlayablePlayer(m_playable.GetId(), -2);
				break;
			case PS_ECharacterState.Player:
				
				break;
			case PS_ECharacterState.Disconnected:
				currentPlayableController.SetPlayerPlayable(playableManager.GetPlayerByPlayable(m_playable.GetId()), -1);
				break;
		}
	}
	
}