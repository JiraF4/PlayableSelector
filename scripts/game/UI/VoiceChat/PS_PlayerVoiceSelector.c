// Widget displays info about player in voice chat.
// Path: {086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout
// Part of voice chat list PS_VoiceChatList ({35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout)

class PS_PlayerVoiceSelector : SCR_ButtonBaseComponent
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wUnitIcon;
	ImageWidget m_wLeaderIcon;
	RichTextWidget m_wPlayerName;
	PS_VoiceButton m_wVoiceHideableButton;
	TextWidget m_wGroupName;
	ImageWidget m_wCharacterFactionColor;
	ButtonWidget m_wKickButton;
	ImageWidget m_wImageCurrent
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharacterFactionColor = ImageWidget.Cast(w.FindAnyWidget("CharacterFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wLeaderIcon = ImageWidget.Cast(w.FindAnyWidget("LeaderIcon"));
		m_wPlayerName = RichTextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wVoiceHideableButton = PS_VoiceButton.Cast(w.FindAnyWidget("VoiceHideableButton").FindHandler(PS_VoiceButton));
		m_wGroupName = TextWidget.Cast(w.FindAnyWidget("GroupName"));
		m_wKickButton = ButtonWidget.Cast(w.FindAnyWidget("KickButton"));
		m_wImageCurrent = ImageWidget.Cast(w.FindAnyWidget("ImageCurrent"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		SCR_ButtonBaseComponent kickButtonHandler = SCR_ButtonBaseComponent.Cast(m_wKickButton.FindHandler(SCR_ButtonBaseComponent));
		kickButtonHandler.m_OnClicked.Insert(KickButtonClicked);
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
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		if (!playableManager)
			return;
		
		// player data
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayer);
		string playerName = playableManager.GetPlayerName(m_iPlayer);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayer);
		int playerRoomId = VoNRoomsManager.GetPlayerRoom(m_iPlayer);
		string playerRoom = VoNRoomsManager.GetRoomName(playerRoomId);
		int groupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
		 
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		int currentPlayerRoomId = VoNRoomsManager.GetPlayerRoom(currentPlayerId);
		string currentPlayerRoom = VoNRoomsManager.GetRoomName(currentPlayerRoomId);
		int currentGroupCallSign = playableManager.GetGroupCallsignByPlayable(m_iPlayer);
				
		// update
		if (playerName != "") m_wPlayerName.SetText(playerName);
		//m_wVoiceHideableButton.Update();
		m_wLeaderIcon.SetVisible(playableManager.IsPlayerGroupLeader(m_iPlayer));
		if (faction) m_wCharacterFactionColor.SetColor(faction.GetFactionColor());
		else m_wCharacterFactionColor.SetColor(Color.FromInt(0xFF2c2c2c));
		
		bool showKick = PS_PlayersHelper.IsAdminOrServer();
		if (playerRoomId == currentPlayerRoomId) {
			if (currentPlayerRoom.Contains(currentGroupCallSign.ToString())) {
				if (!showKick) showKick = groupCallSign != currentGroupCallSign;
			}
			if (!showKick && currentPlayerRoom.Contains("#PS-VoNRoom_Command")) showKick = !playableManager.IsPlayerGroupLeader(m_iPlayer) && playableManager.IsPlayerGroupLeader(currentPlayerId);
		}
		m_wKickButton.SetVisible(showKick);
		m_wImageCurrent.SetVisible(currentPlayerId == m_iPlayer);
		
		if (SCR_Global.IsAdmin(m_iPlayer)) m_wPlayerName.SetColor(Color.FromInt(0xfff2a34b));
		else if (playerName == "") m_wPlayerName.SetColor(Color.FromInt(0xff999999));
		else m_wPlayerName.SetColor(Color.FromInt(0xffffffff));
		
		if (playableId != RplId.Invalid()) {
			PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
			SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
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
	
	void KickButtonClicked(SCR_ButtonBaseComponent roomButton)
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		SCR_EGameModeState gameState = gameMode.GetState();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		if (gameState == SCR_EGameModeState.BRIEFING)
		{
			RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
			string groupName = playableManager.GetGroupCallsignByPlayable(playableId).ToString();
			currentPlayableController.MoveToVoNRoom(m_iPlayer, playableManager.GetPlayerFactionKey(m_iPlayer), groupName);
		} else {
			currentPlayableController.MoveToVoNRoom(m_iPlayer, playableManager.GetPlayerFactionKey(m_iPlayer), "#PS-VoNRoom_Faction");
		}
	}
}
































