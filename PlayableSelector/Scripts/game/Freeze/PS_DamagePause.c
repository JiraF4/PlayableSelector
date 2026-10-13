class PS_DamagePause
{
	protected ref array<SCR_PersistentDamageEffect> m_aPaused = {};

	//! Server: every active effect on every character in the world.
	void PauseAll_S()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		world.QueryEntitiesByAABB(mins, maxs, PauseCharacter_S, null, EQueryEntitiesFlags.DYNAMIC);
	}

	//! Server: the paused effects resume where they stopped; damaged bodies regenerate again.
	void ResumeAll_S()
	{
		foreach (SCR_PersistentDamageEffect effect : m_aPaused)
		{
			if (effect)
				effect.SetActive(true);
		}
		m_aPaused.Clear();

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		world.QueryEntitiesByAABB(mins, maxs, RearmRegeneration_S, null, EQueryEntitiesFlags.DYNAMIC);
	}

	void Clear_S()
	{
		m_aPaused.Clear();
	}

	protected bool PauseCharacter_S(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		SCR_ExtendedDamageManagerComponent damageManager = SCR_ExtendedDamageManagerComponent.Cast(character.FindComponent(SCR_ExtendedDamageManagerComponent));
		if (!damageManager)
			return true;

		array<ref SCR_PersistentDamageEffect> effects = {};
		damageManager.GetPersistentEffects(effects);
		foreach (SCR_PersistentDamageEffect effect : effects)
		{
			if (!effect.IsActive())
				continue;

			effect.SetActive(false);
			m_aPaused.Insert(effect);
		}
		return true;
	}

	protected bool RearmRegeneration_S(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
			return true;

		SCR_CharacterResilienceHitZone resilience = damageManager.GetResilienceHitZone();
		if (resilience && resilience.GetHealthScaled() < 1)
			damageManager.RegenVirtualHitZone(resilience);

		SCR_CharacterBloodHitZone blood = damageManager.GetBloodHitZone();
		if (blood && blood.GetHealthScaled() < 1)
			damageManager.RegenVirtualHitZone(blood);

		HitZone defaultHitZone = damageManager.GetDefaultHitZone();
		if (defaultHitZone && !damageManager.IsDamageEffectPresentOnHitZones(SCR_PhysicalHitZonesRegenDamageEffect, {defaultHitZone}))
			damageManager.RegenPhysicalHitZones();

		return true;
	}
}
