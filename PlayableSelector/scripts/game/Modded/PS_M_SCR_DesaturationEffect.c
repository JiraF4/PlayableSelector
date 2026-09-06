// PS_M_SCR_DesaturationEffect - spectator view vs the corpse's grey-out.
//
// WHY: a dead player keeps CONTROLLING their corpse (body-less design, no respawn), and the colour-grade
// screen effect drives saturation from the controlled character's BLOOD hit zone - a corpse is at ~empty
// blood, so it drains the whole screen to grey. That colour grade is a WORLD post-process; it sits on top
// of whatever camera is rendering, including the spectator camera, and the vanilla manager only hides its
// WIDGET effects on a non-player camera, not the post-process.
//
// Keep the spectator view full-colour while the local player is a menu speaker (dead corpse / no controlled
// entity); normal gameplay desaturation resumes the moment the player controls a living character.
// ClearEffects() neutralises the static the material reads, and skipping super stops it being re-driven
// from the dead blood hit zone.
//
// Suggested by the LiteLobby author; adapted to our PS_IsMenuSpeaker detection (we have no LL_SpectatorManager).
modded class SCR_DesaturationEffect
{
	override protected void UpdateEffect(float timeSlice)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			ClearEffects();
			return;
		}

		super.UpdateEffect(timeSlice);
	}
}
