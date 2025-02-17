//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class PS_RespawnSelectedContextAction : SCR_SelectedEntitiesContextAction
{
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBePerformed(selectedEntity, cursorWorldPosition, flags);
	}
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(selectedEntity.GetOwner().FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return false;
		return selectedEntity
			&& !selectedEntity.HasEntityFlag(EEditableEntityFlag.NON_DELETABLE)
			&& !selectedEntity.HasEntityFlag(EEditableEntityFlag.NON_INTERACTIVE);
	}
	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(selectedEntity.GetOwner().FindComponent(PS_PlayableComponent));
		PS_PlayableControllerComponent playableController = PS_PlayableManager.GetPlayableController();
		playableController.RespawnPlayable(playableComponent.GetRplId(), false);
	}
};
