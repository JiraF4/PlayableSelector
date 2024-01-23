// Widget displays info about faction witch contains any playable character.
// Path: {DA22ED7112FA8028}UI/Lobby/FactionSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)


class PS_FactionSelector : SCR_ButtonBaseComponent
{
	protected int m_maxCount;
	protected int m_currentCount;
	protected Faction m_faction;
	
	ImageWidget m_wFactionFlag;
	TextWidget m_wFactionName;
	ImageWidget m_wFactionColor;
	TextWidget m_wFactionCounter;
	ImageWidget m_wLockImage;
	
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("FactionFlag"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wFactionCounter = TextWidget.Cast(w.FindAnyWidget("FactionCounter"));
		m_wLockImage = ImageWidget.Cast(w.FindAnyWidget("LockImage"));
	}
	
	void SetFaction(Faction faction)
	{
		m_faction = faction;
		
		UIInfo uiInfo = faction.GetUIInfo();
		
		m_wFactionName.SetText(m_faction.GetFactionName());
		m_wFactionColor.SetColor(SCR_Faction.Cast(m_faction).GetOutlineFactionColor());
		m_wFactionFlag.LoadImageTexture(0, uiInfo.GetIconPath());
	}
	
	void SetCount(int readyPlayers, int current, int max)
	{
		m_wFactionCounter.SetText(current.ToString() + " / " + max.ToString());
		
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_wLockImage.SetVisible(false);
		if (gameMode.IsFactionLockMode())
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			PlayerController currentPlayerController = GetGame().GetPlayerController();
			EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
			if (currentPlayerRole != EPlayerRole.ADMINISTRATOR)
			{
				PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
				FactionKey factionKey = playableManager.GetPlayerFactionKey(currentPlayerController.GetPlayerId());
				if (factionKey != "") m_wLockImage.SetVisible(true);
			}
		}
	}
	
	Faction GetFaction()
	{
		return m_faction;
	}
	
}