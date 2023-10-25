class PS_PlayerVoiceSelector : SCR_ButtonBaseComponent
{
	protected int m_iPlayer;
	
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	ImageWidget m_wUnitIcon;
	TextWidget m_wPlayerName;
	PS_VoiceButton m_wVoiceHideableButton;
	TextWidget m_wGroupName;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
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
		
		// player data
		RplId playableId = playableManager.GetPlayableByPlayer(m_iPlayer);
		FactionKey factionKey = playableManager.GetPlayerFactionKey(m_iPlayer);
		string playerName = playerManager.GetPlayerName(m_iPlayer);
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		
		// update
		m_wPlayerName.SetText(playerName);
		m_wVoiceHideableButton.Update();
		
		
		if (playableId != RplId.Invalid()) {
			PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
			SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
			string groupName = playableManager.GetGroupNameByPlayable(playableId);
			
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
	
	
}
































