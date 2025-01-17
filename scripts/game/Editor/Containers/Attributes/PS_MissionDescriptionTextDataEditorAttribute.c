[BaseContainerProps()]
class PS_MissionDescriptionTextDataEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return null;
		return SCR_BaseEditorAttributeVar.CreateString(missionDescription.GetMissionTextData());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return;

		missionDescription.SetMissionTextData(var.GetString())
	}
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		return super.GetEntries(outEntries);
	}
}
