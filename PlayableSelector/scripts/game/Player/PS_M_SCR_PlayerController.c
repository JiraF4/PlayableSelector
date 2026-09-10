modded class SCR_PlayerController
{
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
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		// Only execute on local client for own controller
		if (GetGame().GetPlayerController() != this)
			return;

		// Invalidate editor loc cache on possession changes (living character <-> dead/spectator/lobby)
		SCR_VoNComponent.InvalidateEditorLocCache(GetPlayerId());
	}
}
