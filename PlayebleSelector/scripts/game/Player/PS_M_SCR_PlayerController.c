modded class SCR_PlayerController
{
	// Cahce for fast access
	PS_PlayableControllerComponent m_PS_PlayableControllerComponent;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_PS_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(owner.FindComponent(PS_PlayableControllerComponent));
	}

	override event protected void EOnActivate(IEntity owner) {
		super.EOnActivate(owner);

		auto playeController = PlayerController.Cast(owner);
		auto ppiPlayerControllerComponent = PS_PPI_PlayerControllerComponent.GetInstance(playeController);
		if (ppiPlayerControllerComponent)
			ppiPlayerControllerComponent.EOnActivate(owner);
	};

	override event protected void EOnDeactivate(IEntity owner) {
		super.EOnDeactivate(owner);

		auto playeController = PlayerController.Cast(owner);
		auto ppiPlayerControllerComponent = PS_PPI_PlayerControllerComponent.GetInstance(playeController);
		if (ppiPlayerControllerComponent)
			ppiPlayerControllerComponent.EOnDeactivate(owner);
	};
	
	PS_PlayableControllerComponent PS_GetPLayableComponent()
	{
		return m_PS_PlayableControllerComponent;
	}
}
