// PS_M_SCR_BleedingScreenEffect - spectator view vs the corpse's bleeding blackout.
//
// WHY: same body-less design issue as SCR_DeathScreenEffect. The bleeding effect uses
// CreateEffectOverTime / ClearEffectOverTime callbacks that cycle every 2.5s, animating
// BloodVignette1/2 and BleedingBlackOut via AnimateWidget. The animations override our
// SuppressSpectatorScreenEffects() hiding between watchdog ticks.
//
// Prevent bleeding effects from starting and clear running animations when the local player
// is a menu speaker (dead corpse / no controlled entity).
modded class SCR_BleedingScreenEffect
{
	override protected void OnDamageEffectAdded(notnull SCR_DamageEffect dmgEffect)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			Print("[PS_SpecDiag] SCR_BleedingScreenEffect.OnDamageEffectAdded: suppressed (spectating)");
			ClearEffects();
			return;
		}

		super.OnDamageEffectAdded(dmgEffect);
	}

	override protected void CreateEffectOverTime(bool repeat)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			Print("[PS_SpecDiag] SCR_BleedingScreenEffect.CreateEffectOverTime: suppressed (spectating)");
			ClearEffects();
			return;
		}

		super.CreateEffectOverTime(repeat);
	}

	override protected void BlackoutEffect(float effectStrength)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			PrintFormat("[PS_SpecDiag] SCR_BleedingScreenEffect.BlackoutEffect: suppressed (spectating) strength=%1", effectStrength);
			ClearEffects();
			return;
		}

		super.BlackoutEffect(effectStrength);
	}

	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			// Spectating: don't re-register damage effect invokers on the dead corpse.
			Print("[PS_SpecDiag] SCR_BleedingScreenEffect.DisplayControlledEntityChanged: suppressed (spectating)");
			ClearEffects();
			return;
		}

		super.DisplayControlledEntityChanged(from, to);
	}
}
