// Widget displays info about player in voice chat.
// Path: {086F282C8CE692F1}UI/VoiceChat/VoicePlayerSelector.layout
// Part of voice chat list PS_VoiceChatList ({35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout)

class PS_PlayerVoiceSelector : SCR_ButtonComponent
{
	protected int m_iPlayerId;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wUnitIcon;
	ImageWidget m_wLeaderIcon;
	RichTextWidget m_wPlayerName;
	PS_VoiceButton m_wVoiceHideableButton;
	TextWidget m_wGroupName;
	ImageWidget m_wCharacterFactionColor;
	ButtonWidget m_wKickButton;
	ImageWidget m_wImageCurrent;
	
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
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayerId = playerId;
		m_wVoiceHideableButton.SetPlayer(playerId);
		UpdateInfo();
	}
	
	int GetPlayerId()
	{
		return m_iPlayerId;
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
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayerId);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayerId);
		string playerName = playableManager.GetPlayerName(m_iPlayerId);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayerId);
		int playerRoomId = VoNRoomsManager.GetPlayerRoom(m_iPlayerId);
		string playerRoom = VoNRoomsManager.GetRoomName(playerRoomId);
		int groupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
		 
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		int currentPlayerRoomId = VoNRoomsManager.GetPlayerRoom(currentPlayerId);
		string currentPlayerRoom = VoNRoomsManager.GetRoomName(currentPlayerRoomId);
		int currentGroupCallSign = playableManager.GetGroupCallsignByPlayable(m_iPlayerId);
				
		// update
		if (playerName != "") m_wPlayerName.SetText(playerName);
		//m_wVoiceHideableButton.Update();
		m_wLeaderIcon.SetVisible(playableManager.IsPlayerGroupLeader(m_iPlayerId));
		if (faction) m_wCharacterFactionColor.SetColor(faction.GetFactionColor());
		else m_wCharacterFactionColor.SetColor(Color.FromInt(0xFF2c2c2c));
		
		bool showKick = PS_PlayersHelper.IsAdminOrServer();
		if (playerRoomId == currentPlayerRoomId) {
			if (currentPlayerRoom.Contains(currentGroupCallSign.ToString())) {
				if (!showKick) showKick = groupCallSign != currentGroupCallSign;
			}
			if (!showKick && currentPlayerRoom.Contains("#PS-VoNRoom_Command")) showKick = !playableManager.IsPlayerGroupLeader(m_iPlayerId) && playableManager.IsPlayerGroupLeader(currentPlayerId);
		}
		//m_wKickButton.SetVisible(showKick);
		int currentPlayerIdInt = currentPlayerId;
		PS_CoopLobby coopLobby = PS_CoopLobby.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if (coopLobby)
			currentPlayerIdInt = coopLobby.GetSelectedPlayer();
		m_wImageCurrent.SetVisible(currentPlayerIdInt == m_iPlayerId);
		
		if (SCR_Global.IsAdmin(m_iPlayerId)) m_wPlayerName.SetColor(Color.FromInt(0xfff2a34b));
		else if (playerName == "") m_wPlayerName.SetColor(Color.FromInt(0xff999999));
		else m_wPlayerName.SetColor(Color.FromInt(0xffffffff));
		
		if (playableId != RplId.Invalid()) {
			PS_PlayableContainer playableComponent = playableManager.GetPlayableById(playableId);
			string groupName = PS_GroupHelper.GroupCallsignToGroupName(faction, groupCallSign);
			
			m_wUnitIcon.SetVisible(true);
			m_wGroupName.SetVisible(true);
			if (playableComponent)
				m_wUnitIcon.LoadImageTexture(0, playableComponent.GetRoleIconPath());
			m_wGroupName.SetText(groupName);
		}else{
			m_wUnitIcon.SetVisible(false);
			m_wGroupName.SetVisible(false);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Context menu
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
		
		return false;
	}
	
	void OpenContext()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return;
		
		string playerName = PS_PlayableManager.GetInstance().GetPlayerName(m_iPlayerId);
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(menu.GetRootWidget(), playerName);
		// Well that suck...
		if (menu.IsInherited(PS_CoopLobby) && PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionPlayerSelect(m_iPlayerId);
			if (PS_PlayableManager.GetInstance().GetPlayerPin(m_iPlayerId))
				contextMenu.ActionUnpin(m_iPlayerId);
		}
		
		if (m_iPlayerId == GetGame().GetPlayerController().GetPlayerId())
		{
			if (PS_PlayersHelper.IsAdminOrServer())
			{
				contextMenu.ActionGetArmaId(m_iPlayerId);
			}
			return;
		}
		
		contextMenu.ActionDirectMessage(m_iPlayerId);
		
		// TODO: simplify and protect
		
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		
		PermissionState mute = PermissionState.DISALLOWED;
		SocialComponent socialComp = SocialComponent.Cast(GetGame().GetPlayerController().FindComponent(SocialComponent));
		if (socialComp.IsMuted(m_iPlayerId))
			contextMenu.ActionUnmute(m_iPlayerId);
		else
			contextMenu.ActionMute(m_iPlayerId);
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionKick(m_iPlayerId);
			contextMenu.ActionPlayerSelect(m_iPlayerId);
			
			PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			RplId playableId = playableManager.GetPlayableByPlayerRemembered(m_iPlayerId);
			if (playableId != RplId.Invalid() && gameMode.GetState() == SCR_EGameModeState.GAME)
			{
				contextMenu.ActionRespawnInPlace(playableId, m_iPlayerId);
			}
		}
		
		// player data
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayerId);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayerId);
		//string playerName = playableManager.GetPlayerName(m_iPlayerId);
		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(m_iPlayerId);
		int playerRoomId = VoNRoomsManager.GetPlayerRoom(m_iPlayerId);
		string playerRoom = VoNRoomsManager.GetRoomName(playerRoomId);
		int groupCallSign = playableManager.GetGroupCallsignByPlayable(playableId);
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		int currentPlayerRoomId = VoNRoomsManager.GetPlayerRoom(currentPlayerId);
		string currentPlayerRoom = VoNRoomsManager.GetRoomName(currentPlayerRoomId);
		int currentGroupCallSign = playableManager.GetGroupCallsignByPlayable(m_iPlayerId);
		
		bool showKick = PS_PlayersHelper.IsAdminOrServer();
		if (playerRoomId == currentPlayerRoomId) {
			if (currentPlayerRoom.Contains(currentGroupCallSign.ToString())) {
				if (!showKick) showKick = groupCallSign != currentGroupCallSign;
			}
			if (!showKick && currentPlayerRoom.Contains("#PS-VoNRoom_Command")) showKick = !playableManager.IsPlayerGroupLeader(m_iPlayerId) && playableManager.IsPlayerGroupLeader(currentPlayerId);
		}
		if (showKick)
			contextMenu.ActionVoiceKick(m_iPlayerId).Insert(OnActionVoiceKick);
		
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			contextMenu.ActionGetArmaId(m_iPlayerId);
		}
	}
	
	// -------------------- Buttons events --------------------
	void OnActionVoiceKick(PS_ContextAction contextAction, PS_ContextActionDataPlayer contextActionDataPlayer)
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
			RplId playableId = playableManager.GetPlayableByPlayer(contextActionDataPlayer.GetPlayerId());
			string groupName = playableManager.GetGroupCallsignByPlayable(playableId).ToString();
			currentPlayableController.MoveToVoNRoom(contextActionDataPlayer.GetPlayerId(), playableManager.GetPlayerFactionKey(contextActionDataPlayer.GetPlayerId()), groupName);
		} else {
			currentPlayableController.MoveToVoNRoom(contextActionDataPlayer.GetPlayerId(), playableManager.GetPlayerFactionKey(contextActionDataPlayer.GetPlayerId()), "#PS-VoNRoom_Faction");
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void Deselect()
	{
		m_wImageCurrent.SetVisible(false);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void Select()
	{
		m_wImageCurrent.SetVisible(true);
	}
}
































