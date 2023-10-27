// Widget displays info about group witch contains any playable character.
// Contains PS_CharacterSelector widgets for each playable members of group.
// Path: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_RolesGroup : SCR_WLibComponentBase
{
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	protected ref array<Widget> m_aCharactersListWidgets = {};
	Widget m_wCharactersList;
	TextWidget m_wRolesGroupName;
	
	int n_sGroupCallSign;
	
	ButtonWidget m_wVoiceJoinButton;
	ImageWidget m_wVoiceJoinImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharactersList = w.FindAnyWidget("CharactersList");
		m_wRolesGroupName = TextWidget.Cast(w.FindAnyWidget("RolesGroupName"));
		
		m_wVoiceJoinButton = ButtonWidget.Cast(w.FindAnyWidget("VoiceJoinButton"));
		m_wVoiceJoinImage = ImageWidget.Cast(w.FindAnyWidget("VoiceJoinImage"));
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
		Update();
	}
	
	void AddOnClick()
	{
		SCR_ButtonBaseComponent voiceJoinButtonHandler = SCR_ButtonBaseComponent.Cast(m_wVoiceJoinButton.FindHandler(SCR_ButtonBaseComponent));
		voiceJoinButtonHandler.m_OnClicked.Insert(VoiceJoinButtonHandlerClicked);
	}
	
	Widget AddPlayable(PS_PlayableComponent playable)
	{
		Widget CharacterSelector = GetGame().GetWorkspace().CreateWidgets(m_sCharacterSelectorPrefab);
		PS_CharacterSelector handler = PS_CharacterSelector.Cast(CharacterSelector.FindHandler(PS_CharacterSelector));
		handler.SetPlayable(playable);
		m_aCharactersListWidgets.Insert(CharacterSelector);
		m_wCharactersList.AddChild(CharacterSelector);
		return CharacterSelector;
	}
	
	void SetName(SCR_Faction faction, int CallSign)
	{
		n_sGroupCallSign = CallSign;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		string name = playableManager.GroupCallsignToGroupName(faction, CallSign);
		
		m_wRolesGroupName.SetText(name);
	}
	
	void GetWidgets(out array<Widget> charactersListWidgets)
	{
		charactersListWidgets.Copy(m_aCharactersListWidgets);
	}
	
	void Update()
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		int currentRoomId = VoNRoomsManager.GetPlayerRoom(playerId);
		int roomId = VoNRoomsManager.GetRoomWithFaction(playableManager.GetPlayerFactionKey(playerId), n_sGroupCallSign.ToString());
		
		if (currentRoomId == roomId) m_wVoiceJoinImage.LoadImageFromSet(0, m_sImageSetPS, "RoomExit");
		else m_wVoiceJoinImage.LoadImageFromSet(0, m_sImageSetPS, "RoomEnter");
	}
	
	// -------------------- Buttons events --------------------
	void VoiceJoinButtonHandlerClicked(SCR_ButtonBaseComponent VoiceJoinButton)
	{
		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = playerController.GetPlayerId();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		int currentRoomId = VoNRoomsManager.GetPlayerRoom(playerId);
		int roomId = VoNRoomsManager.GetRoomWithFaction(playableManager.GetPlayerFactionKey(playerId), n_sGroupCallSign.ToString());
		
		if (currentRoomId == roomId) playableController.MoveToVoNRoom(playerId, playableManager.GetPlayerFactionKey(playerId), "#PS-VoNRoom_Faction");
		else playableController.MoveToVoNRoom(playerId, playableManager.GetPlayerFactionKey(playerId), n_sGroupCallSign.ToString());
	}
}




























