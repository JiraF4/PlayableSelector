[BaseContainerProps()]
class PS_CharacterIsPlayableEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableCharacterComponent editableCharacter = SCR_EditableCharacterComponent.Cast(item);
		if (!editableCharacter)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(editableCharacter.GetPlayable());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableCharacterComponent editableCharacter = SCR_EditableCharacterComponent.Cast(item);
		if (!editableCharacter)
			return;

		editableCharacter.SetPlayable(var.GetBool())
	}
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		return super.GetEntries(outEntries);
	}
}
