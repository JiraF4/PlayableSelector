// Widget displays info about player in voice chat.
// Path: {086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout
// Part of voice chat list PS_VoiceChatList ({35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout)

class PS_PlayerVoiceSelector : SCR_ButtonBaseComponent
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wUnitIcon;
	ImageWidget m_wLeaderIcon;
	TextWidget m_wPlayerName;
	PS_VoiceButton m_wVoiceHideableButton;
	TextWidget m_wGroupName;
	ImageWidget m_wCharacterFactionColor;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wLeaderIcon = ImageWidget.Cast(w.FindAnyWidget("LeaderIcon"));
		m_wPlayerName = TextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wVoiceHideableButton = PS_VoiceButton.Cast(w.FindAnyWidget("VoiceHideableButton").FindHandler(PS_VoiceButton));
		m_wGroupName = TextWidget.Cast(w.FindAnyWidget("GroupName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayer = playerId;
		m_wVoiceHideableButton.SetPlayer(playerId);
		UpdateInfo();
	}
	
	int GetPlayerId()
	{
		return m_iPlayer;
	}
	
	void UpdateInfo()
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// player data
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayer);
		string playerName = playerManager.GetPlayerName(m_iPlayer);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayer);
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		// update
		m_wPlayerName.SetText(playerName);
		m_wVoiceHideableButton.Update();
		m_wLeaderIcon.SetVisible(playableManager.IsPlayerGroupLeader(m_iPlayer));
		if (faction) m_wCharacterFactionColor.SetColor(faction.GetOutlineFactionColor());
		else m_wCharacterFactionColor.SetColor(Color.FromInt(0xFF2c2c2c));
		
		if (playerRole == EPlayerRole.ADMINISTRATOR) m_wPlayerName.SetColor(Color.FromInt(0xfff2a34b));
		else m_wPlayerName.SetColor(Color.FromInt(0xffffffff));
		
		if (playableId != RplId.Invalid()) {
			PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
			SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
			int groupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
			string groupName = playableManager.GroupCallsignToGroupName(faction, groupCallSign);
			
			
			m_wUnitIcon.SetVisible(true);
			m_wGroupName.SetVisible(true);
			m_wUnitIcon.LoadImageTexture(0, uiInfo.GetIconPath());
			m_wGroupName.SetText(groupName);
		}else{
			m_wUnitIcon.SetVisible(false);
			m_wGroupName.SetVisible(false);
		}
		
	}
	
	// -------------------- Buttons events --------------------
	
	void MoveToRoom(SCR_ButtonBaseComponent roomButton)
	{
		
	}
}
































