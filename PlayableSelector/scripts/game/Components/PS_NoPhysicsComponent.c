[ComponentEditorProps(category: "GameScripted/Entity", description: "Disable physics and collisions, for parked lobby/VoN bodies")]
class PS_NoPhysicsComponentClass : ScriptComponentClass
{
}

// Keeps the parked initial (VoN) entity fully inert: no simulation, no collisions.
// Without this the client owner has to keep teleporting the body back to its parking
// spot, which dirties its replication state.
class PS_NoPhysicsComponent : ScriptComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		DisablePhysics(owner);

		SetEventMask(owner, EntityEvent.POSTFRAME);
	}

	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		DisablePhysics(owner);
	}

	private void DisablePhysics(IEntity ent)
	{
		Physics phys = ent.GetPhysics();
		if (!phys)
			return;

		phys.ChangeSimulationState(SimulationState.NONE);
		phys.SetActive(ActiveState.INACTIVE);

		phys.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
	}
}
