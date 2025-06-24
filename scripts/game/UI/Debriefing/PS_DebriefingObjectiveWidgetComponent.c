class PS_DebriefingObjectiveWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	PS_Objective m_Objective;
	
	TextWidget m_wObjectiveName;
	TextWidget m_wObjectiveScore;
	ImageWidget m_wObjectiveCompletion;
	ButtonWidget m_wCompleteButton;
	
	SCR_ButtonBaseComponent m_wCompleteButtonHandler;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wObjectiveName = TextWidget.Cast(w.FindAnyWidget("ObjectiveName"));
		m_wObjectiveScore = TextWidget.Cast(w.FindAnyWidget("ObjectiveScore"));
		m_wObjectiveCompletion = ImageWidget.Cast(w.FindAnyWidget("ObjectiveCompletion"));
		m_wCompleteButton = ButtonWidget.Cast(w.FindAnyWidget("CompleteButton"));
		
		m_wCompleteButtonHandler = SCR_ButtonBaseComponent.Cast(m_wCompleteButton.FindHandler(SCR_ButtonBaseComponent));
		
		m_wCompleteButtonHandler.m_OnClicked.Insert(CompleteButtonClicked);
	}
	
	void SetObjective(PS_Objective objective)
	{
		m_Objective = objective;
		m_Objective.GetOnObjectiveUpdate().Insert(UpdateInfo);
		UpdateInfo();
	}
	
	void UpdateInfo()
	{
		m_wObjectiveName.SetText(m_Objective.GetTitle());
		m_wObjectiveScore.SetText(m_Objective.GetScore().ToString());
		
		if (m_Objective.GetCompleted())
			m_wObjectiveCompletion.LoadImageFromSet(0, m_sImageSet, "check");
		else
			m_wObjectiveCompletion.LoadImageFromSet(0, m_sImageSet, "cancel");
	}
	
	void CompleteButtonClicked(SCR_ButtonBaseComponent completeButton)
	{
		// global
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		// current player
		PlayerController currentPlayerController = GetGame().GetPlayerController();
		int currentPlayerId = currentPlayerController.GetPlayerId();
		EPlayerRole currentPlayerRole = playerManager.GetPlayerRoles(currentPlayerController.GetPlayerId());
		PS_PlayableControllerComponent currentPlayableController = PS_PlayableControllerComponent.Cast(currentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		currentPlayableController.SetObjectiveCompleteState(m_Objective, !m_Objective.GetCompleted());
	}
}