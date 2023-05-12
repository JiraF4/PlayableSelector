//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_BestVechicleAiClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_BestVechicleAi : ScriptComponent
{
	ref array<vector> positions = new array<vector>();
	int cPosition = 0;
	
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(saii, 1, false);
	}
	
	void saii()
	{
		IEntity wp = GetGame().GetWorld().FindEntityByName("WP1");
		positions.Insert(wp.GetOrigin());
		wp = GetGame().GetWorld().FindEntityByName("WP2");
		positions.Insert(wp.GetOrigin());
		wp = GetGame().GetWorld().FindEntityByName("WP3");
		positions.Insert(wp.GetOrigin());
		
		GetGame().GetCallqueue().CallLater(sai, 0, false);
	}
	
	void sai()
	{
		IEntity owner = GetOwner();
		Vehicle vehicle = Vehicle.Cast(owner);
		AIPathfindingComponent aIPathfindingComponent = AIPathfindingComponent.Cast(vehicle.FindComponent(AIPathfindingComponent));
		NavmeshWorldComponent navmeshWorldComponent = aIPathfindingComponent.GetNavmeshComponent();
		Physics physics = vehicle.GetPhysics();
		
		/*
		physics.SetAngularVelocity(vector.Up * 0.5);
		vector rotation = owner.GetLocalYawPitchRoll();
		physics.SetVelocity(rotation.AnglesToVector() * 8);
		*/
		
		vector nextPosition = positions[cPosition];
		vector origin = owner.GetOrigin();
		vector moveVector = nextPosition - origin;
		float length = moveVector.Length();
		if (length > 1) moveVector.Normalize();
		else if (length < 0.2) {
			cPosition++;
			if (cPosition >= positions.Count())
				cPosition = 0;
		}
		
		
		
		vector rotation = owner.GetLocalYawPitchRoll();
		vector needRotation = moveVector.VectorToAngles();
		float deltaAngle = SCR_Math.DeltaAngle(rotation[0] + 180, needRotation[0] + 180);
		if (length > 1) {
			physics.SetAngularVelocity(vector.Up * deltaAngle * 0.04);
		}
		vector forwardVector = rotation.AnglesToVector();
		physics.SetVelocity(forwardVector * vector.Dot(forwardVector, moveVector) * 8);
		
		GetGame().GetCallqueue().CallLater(sai, 0, false);
	}
};