modded class SCR_CharacterDamageManagerComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager && freezeManager.IsHardFreezeActive())
		{
			// Pauses any active damage effects if character spawned during hard freeze
			array<ref SCR_PersistentDamageEffect> effects = {};
			GetPersistentEffects(effects);
			foreach (SCR_PersistentDamageEffect effect : effects)
			{
				if (effect && effect.IsActive())
					effect.SetActive(false);
			}
		}
	}
}
