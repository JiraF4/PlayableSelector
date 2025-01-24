// Widget displays info about faction witch contains any playable character.
// Path: {DA22ED7112FA8028}UI/Lobby/FactionSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)


class PS_FactionSelector : SCR_ButtonBaseComponent
{
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected int m_iCurrentPlayerId;
	
	// Vars
	protected int m_iCount;
	protected int m_iMaxCount;
	protected int m_iLockedCount;
	protected SCR_Faction m_faction;
	protected PS_CoopLobby m_CoopLobby;
	
	// Widgets
	protected ImageWidget m_wFactionFlag;
	protected TextWidget m_wFactionName;
	protected ImageWidget m_wFactionColor;
	protected TextWidget m_wFactionCounter;
	protected ImageWidget m_wLockImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		
		// Widgets
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("FactionFlag"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wFactionCounter = TextWidget.Cast(w.FindAnyWidget("FactionCounter"));
		m_wLockImage = ImageWidget.Cast(w.FindAnyWidget("LockImage"));
		
		// Buttons
		m_OnClicked.Insert(OnClicked);
	}
	
	void SetFaction(SCR_Faction faction)
	{
		m_faction = faction;
		
		UIInfo uiInfo = faction.GetUIInfo();
		
		m_wFactionName.SetText(m_faction.GetFactionName());
		m_wFactionColor.SetColor(m_faction.GetOutlineFactionColor());
		m_wFactionFlag.LoadImageTexture(0, uiInfo.GetIconPath());
	}
	
	void SetCount(int count)
	{
		m_iCount = count;
		UpdateCounter();
	}
	
	void SetMaxCount(int maxCount)
	{
		m_iMaxCount = maxCount;
		
		UpdateCounter();
	}
	
	void SetLockedCount(int lockedCount)
	{
		m_iLockedCount = lockedCount;
		
		UpdateCounter();
	}
	
	void SetCoopLobby(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	int GetCount()
	{
		return m_iCount;
	}
	
	int GetMaxCount()
	{
		return m_iMaxCount;
	}
	
	int GetLockedCount()
	{
		return m_iLockedCount;
	}
	
	void UpdateCounter()
	{
		if (m_iMaxCount - m_iLockedCount == 0)
		{
			m_wFactionCounter.SetText("#PS_Lobby_Locked");
			return;
		}
		m_wFactionCounter.SetText(m_iCount.ToString() + " / " + (m_iMaxCount - m_iLockedCount).ToString());
	}
	
	Faction GetFaction()
	{
		return m_faction;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClicked()
	{
		m_CoopLobby.SwitchCurrentFaction(m_faction);
		
		SCR_EGameModeState gameState = m_GameModeCoop.GetState();
		if (gameState == SCR_EGameModeState.SLOTSELECTION)
			m_CoopLobby.SwitchVoiceChatFaction(m_faction.GetFactionKey());
	}
}