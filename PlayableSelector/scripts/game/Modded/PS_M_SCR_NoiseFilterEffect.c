// PS_M_SCR_NoiseFilterEffect - spectator hearing vs the corpse's "stun"/muffle.
//
// WHY: a dead player keeps CONTROLLING their corpse (body-less design, no respawn). That pushes the
// controlled character's life state into the shared engine audio variable "CharacterLifeState", and the
// sound graph turns DEAD/INCAPACITATED into the muffled, ringing low-pass. It is set on the death/
// consciousness EVENTS (not per frame), so once the corpse dies the muffle sticks for the whole spectate -
// nothing clears it because the controlled entity never changes.
//
// Pin the variable back to ALIVE every frame while the local player is a menu speaker (dead corpse / no
// controlled entity) - exactly the value vanilla writes when the respawn menu opens. UpdateEffect is ticked
// every frame by SCR_ScreenEffectsManager; the muffle resumes naturally once the player controls a living
// character again (the vanilla event handlers take the variable back over).
//
// Suggested by the LiteLobby author; adapted to our PS_IsMenuSpeaker detection (we have no LL_SpectatorManager).
modded class SCR_NoiseFilterEffect
{
	override void UpdateEffect(float timeSlice)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
		{
			AudioSystem.SetVariableByName("CharacterLifeState", ECharacterLifeState.ALIVE,
				"{A60F08955792B575}Sounds/_SharedData/Variables/GlobalVariables.conf");
			return;
		}

		super.UpdateEffect(timeSlice);
	}
}
