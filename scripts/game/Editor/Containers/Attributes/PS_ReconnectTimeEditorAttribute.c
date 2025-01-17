[BaseContainerProps()]
class PS_ReconnectTimeEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		//If opened in global attributes
		if (!IsGameMode(item))
			return null;

		PS_GameModeCoop coopMode = PS_GameModeCoop.Cast(item);
		if (!coopMode)
			return null;

		float value = coopMode.GetReconnectTime();
		return SCR_BaseEditorAttributeVar.CreateFloat(((float) value) * 0.001);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		PS_GameModeCoop coopMode = PS_GameModeCoop.Cast(item);
		if (!coopMode)
			return;

		int value = var.GetFloat() * 1000;
		coopMode.SetReconnectTime(value);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		return super.GetEntries(outEntries);
	}
}
