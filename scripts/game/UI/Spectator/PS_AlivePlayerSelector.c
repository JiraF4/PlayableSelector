// Widget displays info about player in list of alive players.
// Path: {20DCB7288210C151}UI/Spectator/AlivePlayerSelector.layout
// Part of alive players list PS_AlivePlayerList ({18D3CF175C9AA974}UI/Spectator/AlivePlayersList.layout)

class PS_AlivePlayerSelector : SCR_ButtonBaseComponent
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wUnitIcon;
	ImageWidget m_wLeaderIcon;
	TextWidget m_wPlayerName;
	TextWidget m_wGroupName;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wLeaderIcon = ImageWidget.Cast(w.FindAnyWidget("LeaderIcon"));
		m_wPlayerName = TextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wGroupName = TextWidget.Cast(w.FindAnyWidget("GroupName"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		m_OnClicked.Insert(AlivePlayerButtonClicked);
	}
	
	void SetPlayer(int playerId)
	{
		m_iPlayer = playerId;
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
		m_wLeaderIcon.SetVisible(playableManager.IsPlayerGroupLeader(m_iPlayer));
		
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
	void AlivePlayerButtonClicked(SCR_ButtonBaseComponent playerButton)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		SCR_CameraManager cameraManager = SCR_CameraManager.Cast(GetGame().GetCameraManager());
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(cameraManager.CurrentCamera());
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
		camera.SetOrigin(playableComponent.GetOwner().GetOrigin());
	}
	
}