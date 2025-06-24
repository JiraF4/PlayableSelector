modded class SCR_PlayerController
{
	// Cahce for fast access
	PS_PlayableControllerComponent m_PS_PlayableControllerComponent;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_PS_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(owner.FindComponent(PS_PlayableControllerComponent));
	}
	
	PS_PlayableControllerComponent PS_GetPLayableComponent()
	{
		return m_PS_PlayableControllerComponent;
	}
}
