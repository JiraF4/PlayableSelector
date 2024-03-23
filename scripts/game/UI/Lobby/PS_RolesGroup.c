// Widget displays info about group witch contains any playable character.
// Contains PS_CharacterSelector widgets for each playable members of group.
// Path: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_RolesGroup : SCR_ScriptedWidgetComponent
{
	// Const
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PlayerController m_PlayerController;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iCurrentPlayerId;
	
	// Widgets
	protected VerticalLayoutWidget m_wCharactersList;
	protected ButtonWidget m_wLockButton;
	protected ImageWidget m_wLockImage;
	protected ButtonWidget m_wVoiceJoinButton;
	protected TextWidget m_wRolesGroupName;
	
	// Handlers
	SCR_ButtonBaseComponent m_LockButtonBaseComponent;
	SCR_ButtonBaseComponent m_VoiceJoinButton;
	
	// Vars
	protected PS_CoopLobby m_CoopLobby;
	protected SCR_AIGroup m_AIGroup;
	protected int m_iLockedCount;
	protected int m_iGroupCallsign;
	protected string m_sGroupCallsign;
	protected FactionKey m_sFactionKey;
	
	ref map<PS_PlayableComponent, PS_CharacterSelector> m_mCharacters = new map<PS_PlayableComponent, PS_CharacterSelector>();
		
	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_PlayerController = GetGame().GetPlayerController();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		
		// Widgets
		m_wCharactersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("CharactersList"));
		m_wLockButton = ButtonWidget.Cast(w.FindAnyWidget("LockButton"));
		m_wLockImage = ImageWidget.Cast(w.FindAnyWidget("LockImage"));
		m_wVoiceJoinButton = ButtonWidget.Cast(w.FindAnyWidget("VoiceJoinButton"));
		m_wRolesGroupName = TextWidget.Cast(w.FindAnyWidget("RolesGroupName"));
		
		// Handlers
		m_LockButtonBaseComponent = SCR_ButtonBaseComponent.Cast(m_wLockButton.FindHandler(SCR_ButtonBaseComponent));
		m_VoiceJoinButton = SCR_ButtonBaseComponent.Cast(m_wVoiceJoinButton.FindHandler(SCR_ButtonBaseComponent));
		
		// Buttons
		m_LockButtonBaseComponent.m_OnClicked.Insert(OnClickedLock);
		m_VoiceJoinButton.m_OnClicked.Insert(OnClickedVoiceJoin);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetAIGroup(SCR_AIGroup aiGroup)
	{
		m_AIGroup = aiGroup;
		m_iGroupCallsign = aiGroup.GetCallsignNum();
		m_sGroupCallsign = m_iGroupCallsign.ToString();
		m_sFactionKey = m_AIGroup.GetFaction().GetFactionKey();
		
		// Init
		string groupName = PS_GroupHelper.GetGroupFullName(m_AIGroup);
		m_wRolesGroupName.SetText(groupName);
	}
	
	void SetLobbyMenu(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update
	void UpdateLockedState(bool add)
	{
		if (add)
			m_iLockedCount++;
		else
			m_iLockedCount--;
		
		if (m_mCharacters.Count() == m_iLockedCount)
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
		else
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Add
	void InsertPlayable(PS_PlayableComponent playable)
	{
		Widget characterSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sCharacterSelectorPrefab, m_wCharactersList);
		PS_CharacterSelector characterSelector = PS_CharacterSelector.Cast(characterSelectorRoot.FindHandler(PS_CharacterSelector));
		characterSelector.SetLobbyMenu(m_CoopLobby);
		characterSelector.SetRolesGroup(this);
		characterSelector.SetPlayable(playable);
		
		m_mCharacters.Insert(playable, characterSelector);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClickedLock(SCR_ButtonBaseComponent button)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		bool unlock = m_mCharacters.Count() == m_iLockedCount;
		
		if (unlock) AudioSystem.PlaySound("{E495F2DA6A44D0BB}Sounds/UI/Samples/Menu/UI_Button_Filter_Off.wav");
		else AudioSystem.PlaySound("{B6008DBCA565E5E1}Sounds/UI/Samples/Menu/UI_Button_Filter_On.wav");
		
		foreach (PS_PlayableComponent playable, PS_CharacterSelector characterSelector : m_mCharacters)
		{
			int playerId = characterSelector.GetPlayerId();
			if (unlock)
			{
				if (playerId == -2)
				{
					m_PlayableControllerComponent.SetPlayablePlayer(playable.GetId(), -1);
				}
			}
			else
			{
				if (playerId != -2)
				{
					PS_EPlayableControllerState state = m_PlayableManager.GetPlayerState(playerId);
					if (state == PS_EPlayableControllerState.Ready)
						m_PlayableControllerComponent.SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
					m_PlayableControllerComponent.SetPlayablePlayer(playable.GetId(), -2);
				}
			}
		}
	}
	
	void OnClickedVoiceJoin(SCR_ButtonBaseComponent button)
	{
		SCR_EGameModeState gameState = m_GameModeCoop.GetState();
		if (gameState == SCR_EGameModeState.SLOTSELECTION)
			m_PlayableControllerComponent.MoveToVoNRoom(m_iCurrentPlayerId, m_sFactionKey, m_sGroupCallsign);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Removed
	void OnPlayableRemoved(PS_PlayableComponent playable)
	{
		m_mCharacters.Remove(playable);
		if (!m_wCharactersList.GetChildren())
		{
			m_wRoot.RemoveFromHierarchy();
			m_CoopLobby.OnRolesGroupRemoved(this);
		}
	}
}




























