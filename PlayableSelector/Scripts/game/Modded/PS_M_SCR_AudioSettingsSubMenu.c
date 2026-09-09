// Adds the PlayableSelector earplugs suppression slider to the vanilla audio settings tab.

modded class SCR_AudioSettingsSubMenu
{
	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		SCR_SettingBindingGameplay bind = new SCR_SettingBindingGameplay("PS_EarplugSettings", "PS_Suppression", "Earplugs");
		m_aSettingsBindings.Insert(bind);

		bind.LoadEntry(m_wScroll, false, true);
		bind.GetEntryChangedInvoker().Insert(OnMenuItemChanged);
	}
}

// PS_EarplugSettings.c
// User-tunable earplugs suppression strength (percent of SFX removed by F2).
// Displayed in the vanilla audio settings tab via PS_M_SCR_AudioSettingsSubMenu.
//, desc: "How strongly earplugs (F2) suppress world sound. 80 keeps 20% of SFX audible, 100 fully mutes.")
class PS_EarplugSettings extends ModuleGameSettings
{
	[Attribute(defvalue: "80", uiwidget: UIWidgets.Slider, params: "0 100 1")]
	int PS_Suppression;
}