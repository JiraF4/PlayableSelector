[BaseContainerProps()]
class PS_MissionDescriptionEmptyFactionEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(missionDescription.GetDescriptionVisibleForEmpty());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return;

		int value = var.GetBool();
		missionDescription.SetDescriptionVisibleForEmpty(value);
	}
}
