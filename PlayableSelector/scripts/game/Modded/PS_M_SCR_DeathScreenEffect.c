// PS_M_SCR_DeathScreenEffect - spectator view vs the corpse's death overlay.
//
// WHY: a dead player keeps CONTROLLING their corpse (body-less design, no respawn). Vanilla's
// SCR_DeathScreenEffect listens for damage state changes on the controlled entity and starts
// AnimateWidget animations that fade DeathOverlay / DeathBlackOut to full opacity over 8-10s.
// AnimateWidget runs every frame and overrides our SuppressSpectatorScreenEffects() hiding,
// so the blackout creeps back in between the 500ms watchdog ticks.
//
// Prevent the animations from starting while the local player is a menu speaker (dead corpse /
// no controlled entity). Also clear any running animations on the initial spectator entry
// (DisplayControlledEntityChanged fires with the dead corpse BEFORE the spectator camera opens).
modded class SCR_DeathScreenEffect
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

	override protected void DeathEffect()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			ClearEffects();
			return;
		}

		super.DeathEffect();
	}

	override protected void InstaDeathEffect()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			ClearEffects();
			return;
		}

		super.InstaDeathEffect();
	}

	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		if (IsControlledEntityDead(to))
		{
			// Spectating or no entity: don't re-register damage invokers on the dead corpse.
			// Clear any running animations so they don't persist into spectator view.
			ClearEffects();
			return;
		}

		super.DisplayControlledEntityChanged(from, to);
	}
}
