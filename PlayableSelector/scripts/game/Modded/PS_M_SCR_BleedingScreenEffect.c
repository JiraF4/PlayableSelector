// PS_M_SCR_BleedingScreenEffect - spectator view vs the corpse's bleeding blackout.
//
// WHY: same body-less design issue as SCR_DeathScreenEffect. The bleeding effect uses
// CreateEffectOverTime / ClearEffectOverTime callbacks that cycle every 2.5s, animating
// BloodVignette1/2 and BleedingBlackOut via AnimateWidget. The animations override our
// SuppressSpectatorScreenEffects() hiding between watchdog ticks.
//
// Prevent bleeding effects from starting and clear running animations when the local player
// is spectating (dead corpse / no controlled entity).
modded class SCR_BleedingScreenEffect
{
	protected bool IsControlledEntityDead(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		CharacterControllerComponent charController = character.GetCharacterController();
		if (charController && charController.IsDead())
			return true;

		SCR_DamageManagerComponent damageManager = character.GetDamageManager();
		if (damageManager && (damageManager.IsDestroyed() || damageManager.GetState() == EDamageState.DESTROYED))
			return true;

		return false;
	}

	override protected void OnDamageEffectAdded(notnull SCR_DamageEffect dmgEffect)
	{
		if (IsControlledEntityDead(m_CharacterEntity))
		{
			ClearEffects();
			return;
		}

		super.OnDamageEffectAdded(dmgEffect);
	}

	override protected void CreateEffectOverTime(bool repeat)
	{
		if (IsControlledEntityDead(m_CharacterEntity))
		{
			ClearEffects();
			return;
		}

		super.CreateEffectOverTime(repeat);
	}

	override protected void BlackoutEffect(float effectStrength)
	{
		if (IsControlledEntityDead(m_CharacterEntity))
		{
			ClearEffects();
			return;
		}

		super.BlackoutEffect(effectStrength);
	}

	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		if (IsControlledEntityDead(to))
		{
			// Spectating or no entity: don't register damage effect invokers on the dead corpse.
			ClearEffects();
			return;
		}

		super.DisplayControlledEntityChanged(from, to);
	}
}
