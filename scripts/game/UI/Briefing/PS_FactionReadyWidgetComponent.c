class PS_FactionReadyWidgetComponent : PS_HideableButton
{
	ResourceName m_sImageSet = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	FactionManager m_FactionManager;
	PS_PlayableManager m_PlayableManager;
	
	PlayerController m_CurrentPlayerController;
	PS_PlayableControllerComponent m_CurrentPlayableControllerComponent;
	int m_iCurrentPlayerId;
	FactionKey m_sCurrentFactionKey;
	
	PS_FactionReadyListWidgetComponent m_FactionReadyListWidgetComponent;
	FactionKey m_sFactionKey;
	SCR_Faction m_faction;
	
	ImageWidget m_wBackgroundFaction;
	int m_iFactionReady;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_FactionManager = GetGame().GetFactionManager();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		
		m_wBackgroundFaction = ImageWidget.Cast(w.FindAnyWidget("BackgroundFaction"));
		
		m_wButtonHandler.m_OnClicked.Insert(OnFactionClicked);
		
		m_CurrentPlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_CurrentPlayerController.GetPlayerId();
		m_sCurrentFactionKey = m_PlayableManager.GetPlayerFactionKey(m_iCurrentPlayerId);
		m_CurrentPlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_CurrentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		m_PlayableManager.m_eFactionReadyChanged.Insert(UpdateFaction);
		
		m_wButton.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	void Init(PS_FactionReadyListWidgetComponent factionReadyListWidgetComponent, FactionKey factionKey)
	{
		m_FactionReadyListWidgetComponent = factionReadyListWidgetComponent;
		m_sFactionKey = factionKey;
		
		m_faction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));
		m_wBackgroundFaction.SetColor(m_faction.GetOutlineFactionColor());
		
		m_wButton.SetVisible(m_sFactionKey == m_sCurrentFactionKey);
		UpdateFaction(m_sFactionKey, m_PlayableManager.GetFactionReady(m_sFactionKey));
	}
	
	//------------------------------------------------------------------------------------------------
	void OnFactionClicked(SCR_ButtonBaseComponent button)
	{
		if (m_sFactionKey != m_sCurrentFactionKey)
			return;
		
		array<PS_PlayableComponent> playables = m_PlayableManager.GetPlayablesSorted();
		foreach (PS_PlayableComponent playableComponent: playables)
		{
			FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
			if (factionAffiliationComponent.GetDefaultFactionKey() != m_sFactionKey)
				continue;
			
			int playerId = m_PlayableManager.GetPlayerByPlayable(playableComponent.GetId());
			if (playerId <= 0)
				continue;
			
			if (playerId != m_iCurrentPlayerId)
				return;
			else
				break;
		}
		
		if (m_iFactionReady == 0)
			m_CurrentPlayableControllerComponent.SetFactionReady(m_sFactionKey, 1);
		else
			m_CurrentPlayableControllerComponent.SetFactionReady(m_sFactionKey, 0);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateFaction(FactionKey factionKey, int readyValue)
	{
		if (factionKey != m_sFactionKey)
			return;
		
		m_iFactionReady = readyValue;
		if (m_iFactionReady == 1)
			m_wImage.LoadImageFromSet(0, m_sImageSet, "check");
		else
			m_wImage.LoadImageFromSet(0, m_sImageSet, "disable");
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_PlayableManager)
			m_PlayableManager.m_eFactionReadyChanged.Remove(UpdateFaction);
	}
}