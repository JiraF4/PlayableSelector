// Hook for playable attribute
modded class SCR_EditableCharacterComponent
{
	bool GetPlayable()
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(GetOwner().FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return false;
		return playableComponent.GetPlayable();
	}
	
	void SetPlayable(bool isPlayable)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(GetOwner().FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return;
		playableComponent.SetPlayable(isPlayable);
	}
}