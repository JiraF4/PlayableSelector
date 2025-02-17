modded class SCR_EditorManagerEntity
{
	override SCR_EditorModeEntity CreateEditorMode(EEditorMode mode, bool isInit, ResourceName prefab = "")
	{
		// Since we can't mode mode base container
		// Cuz FUCKING ENFUSION LOST LINK TO IT IN EVERY FUCKING CONFIG
		// We Can only do this shit, but editor can't be more fucking horrible thin he is right now, soo i don't give a shit
		if (mode == EEditorMode.PHOTO)
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameModeCoop.IsArmaVisionDisabled())
				return null;
		}
		return super.CreateEditorMode(mode, isInit, prefab);
	}
}
