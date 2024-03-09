class PS_GameModeHeaderButton : SCR_ButtonBaseComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Game state", enums: ParamEnumArray.FromEnum(SCR_EGameModeState))]
	SCR_EGameModeState m_eState;
	
	TextWidget m_wStateText;
	
	ImageWidget m_wActiveImage;
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wActiveImage = ImageWidget.Cast(w.FindAnyWidget("ActiveImage"));
		m_wStateText = TextWidget.Cast(w.FindAnyWidget("StateText"));
	}
	
	void SetStateAvailable(bool avaliable)
	{
		SetToggleable(true);
		if (avaliable)
			m_wStateText.SetColor(Color.White);
		else
			m_wStateText.SetColor(Color.Gray);
	}
	
	void Update()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		
		if (m_eState == gameMode.GetState()) 
		{
			SetStateAvailable(true);
			m_wActiveImage.SetVisible(true);
		}
		
		// TODO: fix me please
		if (m_eState == SCR_EGameModeState.BRIEFING && gameMode.GetState() == SCR_EGameModeState.SLOTSELECTION) SetStateAvailable(true);
		if (m_eState == SCR_EGameModeState.SLOTSELECTION && gameMode.GetState() == SCR_EGameModeState.BRIEFING) SetStateAvailable(true);
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		bool toggle = m_eState == playableController.GetMenuState();
		if (IsToggled() != toggle) SetToggled(toggle);
	}
}