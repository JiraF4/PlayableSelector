// Force physic for vehicles
modded class SCR_VehicleWaterPhysicsComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		Physics physic = GetOwner().GetPhysics();
		if (physic) physic.ApplyImpulse("0 1 0");
	}
}
