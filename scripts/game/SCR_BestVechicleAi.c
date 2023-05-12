//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_BestVechicleAiClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_BestVechicleAi : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		sai()
	}
	
	void sai()
	{
		IEntity owner = GetOwner();
		Vehicle vehicle = Vehicle.Cast(owner);
		AIPathfindingComponent aIPathfindingComponent = AIPathfindingComponent.Cast(vehicle.FindComponent(AIPathfindingComponent));
		NavmeshWorldComponent navmeshWorldComponent = aIPathfindingComponent.GetNavmeshComponent();
		
		Physics physics = vehicle.GetPhysics();
		physics.SetAngularVelocity(vector.Up * 0.5);
		vector rotation = owner.GetLocalYawPitchRoll();
		physics.SetVelocity(rotation.AnglesToVector() * 8);
		//physics.ApplyTorque(vector.Right * 500000000);
		
		GetGame().GetCallqueue().CallLater(sai, 0, false);
	}
};