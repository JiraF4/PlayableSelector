class PS_GameModeHeaderButton : SCR_ButtonBaseComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Game state", enums: ParamEnumArray.FromEnum(SCR_EGameModeState))]
	SCR_EGameModeState m_eState;
	
	ImageWidget m_wAdvanceImage;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wAdvanceImage = ImageWidget.Cast(w.FindAnyWidget("AdvanceImage"));
	}
	
	void Update()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_wAdvanceImage.SetVisible(m_eState == gameMode.GetState());
		bool toggle = m_eState == gameMode.GetState();
		if (IsToggled() != toggle) SetToggled(toggle);
	}
}