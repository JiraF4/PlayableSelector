/**
 * @brief Атрибут редактора Game Master для переключения 3D-иконок отделения в гейммоде.
 * @subsystem Admin | UI
 * @context Hybrid
 * @entity PS_GameModeCoop
 * @depends PlayableSelector
 * @listens None
 * @details Позволяет администратору в Game Master динамически включать или отключать
 *          3D-иконки лидера и бойцов отделения в свойствах PS_GameModeCoop.
 */
[BaseContainerProps()]
class PS_DisableSquadNametagIconsEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!IsGameMode(item))
			return null;

		PS_GameModeCoop coopMode = PS_GameModeCoop.Cast(item);
		if (!coopMode)
			return null;

		bool value = coopMode.GetDisableSquadNametagIcons();
		return SCR_BaseEditorAttributeVar.CreateBool(value);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		PS_GameModeCoop coopMode = PS_GameModeCoop.Cast(item);
		if (!coopMode)
			return null;

		int value = var.GetBool();
		coopMode.SetDisableSquadNametagIcons(value);
	}
}
