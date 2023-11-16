class PS_GameModeHeaderButton : SCR_ButtonBaseComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Game state", enums: ParamEnumArray.FromEnum(SCR_EGameModeState))]
	SCR_EGameModeState m_eState;
	
	ImageWidget m_wAdvanceImage;
	protected ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wAdvanceImage = ImageWidget.Cast(w.FindAnyWidget("AdvanceImage"));
	}
	
	void Update()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		
		if (m_eState == gameMode.GetState()) m_wAdvanceImage.LoadImageFromSet(0, m_sImageSet, "moveAll");
		else m_wAdvanceImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
		
		if (m_eState == SCR_EGameModeState.BRIEFING && gameMode.GetState() == SCR_EGameModeState.SLOTSELECTION) m_wAdvanceImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		bool toggle = m_eState == playableController.GetMenuState();
		if (IsToggled() != toggle) SetToggled(toggle);
	}
}