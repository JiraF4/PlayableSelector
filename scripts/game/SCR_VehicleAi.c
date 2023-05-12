//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_VehicleAiClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_VehicleAi : ScriptComponent
{
	
	Vehicle vehicle;
	AIPathfindingComponent aIPathfindingComponent;
	NavmeshWorldComponent navmeshWorldComponent;
	Physics physics;
	SCR_CarControllerComponent carControllerComponent;
	VehicleWheeledSimulation vehicleWheeledSimulation;
	SCR_BaseCompartmentManagerComponent compartmentManagerComponent;
	
	ref array<vector> positions = new array<vector>();
	int cPosition = 0;
	float backward = 0;
	
	override void OnPostInit(IEntity owner)
	{
        owner.SetFlags(EntityFlags.ACTIVE, true);
        SetEventMask( owner, EntityEvent.FRAME);
		
		GetGame().GetCallqueue().CallLater(saii, 1, false);
		
		vehicle = Vehicle.Cast(owner);
		aIPathfindingComponent = AIPathfindingComponent.Cast(vehicle.FindComponent(AIPathfindingComponent));
		navmeshWorldComponent = aIPathfindingComponent.GetNavmeshComponent();
		physics = vehicle.GetPhysics();
		carControllerComponent = SCR_CarControllerComponent.Cast(vehicle.FindComponent(SCR_CarControllerComponent));
		vehicleWheeledSimulation = carControllerComponent.GetSimulation();
		compartmentManagerComponent = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		if (ev)
		{
			//ev.RegisterScriptHandler("OnDestroyed", this, OnDestroyed);
			ev.RegisterScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
			ev.RegisterScriptHandler("OnCompartmentLeft", this, OnCompartmentLeft);
		}	
		
		
		
	}
	protected override void OnDelete(IEntity owner)
	{
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		if (ev)
		{
			//ev.RemoveScriptHandler("OnDestroyed", this, OnDestroyed);
			ev.RemoveScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
			ev.RemoveScriptHandler("OnCompartmentLeft", this, OnCompartmentLeft);
		}	
	}
	
	void saii()
	{
		IEntity wp = GetGame().GetWorld().FindEntityByName("WP1");
		positions.Insert(wp.GetOrigin());
		wp = GetGame().GetWorld().FindEntityByName("WP2");
		positions.Insert(wp.GetOrigin());
		wp = GetGame().GetWorld().FindEntityByName("WP3");
		positions.Insert(wp.GetOrigin());
		
		
	}
	
	protected void OnCompartmentEntered(IEntity vehicle, BaseCompartmentManagerComponent mgr, IEntity occupant, int managerId, int slotID)
	{
		if (!occupant)
			return;

		AIControlComponent aiControl = AIControlComponent.Cast(occupant.FindComponent(AIControlComponent));
		if (!aiControl)
			return;
		
		BaseCompartmentSlot compartmentSlot = mgr.FindCompartment(slotID);
		if (compartmentSlot.Type() == PilotCompartmentSlot)
		{
			carControllerComponent.OnEngineStart();
			vehicleWheeledSimulation.SetBreak(0, false);
			vehicleWheeledSimulation.SetClutch(1);
			carControllerComponent.SetPersistentHandBrake(false);
			vehicleWheeledSimulation.EngineStart();
			//aiControl.DeactivateAI();
		}
		
		if (compartmentSlot.Type() == CargoCompartmentSlot)
		{
			ref array<BaseCompartmentSlot> outCompartments = new array<BaseCompartmentSlot>();
			mgr.GetCompartments(outCompartments);
			foreach (BaseCompartmentSlot outCompartment: outCompartments)
			{
				if (outCompartment.Type() == TurretCompartmentSlot)
				{
					if (!outCompartment.IsOccupied()) {
						//aiControl.ActivateAI();
						CompartmentAccessComponent compartmentAccess = CompartmentAccessComponent.Cast(occupant.FindComponent(CompartmentAccessComponent));
						compartmentAccess.MoveInVehicle(vehicle, outCompartment);
					}
				}
			}
		}
	}
	
	protected void OnCompartmentLeft(IEntity vehicle, BaseCompartmentManagerComponent mgr, IEntity occupant, int managerId, int slotID)
	{
		BaseCompartmentSlot compartmentSlot = mgr.FindCompartment(slotID);
		if (compartmentSlot.Type() == PilotCompartmentSlot)
		{
			carControllerComponent.OnEngineStop();
			vehicleWheeledSimulation.SetBreak(1, true);
			vehicleWheeledSimulation.SetClutch(0);
			carControllerComponent.SetPersistentHandBrake(true);
			vehicleWheeledSimulation.EngineStop();
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
    {
		if (!vehicle.IsOccupied() || carControllerComponent.IsEngineDefective()) return;
		
		
		vector nextPosition = positions[cPosition];
		vector origin = owner.GetOrigin();
		vector moveVector = nextPosition - origin;
		float length = moveVector.Length();
		if (length < 4) {
			cPosition++;
			if (cPosition >= positions.Count())
				cPosition = 0;
		}
		if (length > 1) moveVector.Normalize();
		
		vector rotation = owner.GetLocalYawPitchRoll();
		vector needRotation = moveVector.VectorToAngles();
		
		
		float deltaAngle = SCR_Math.fmod(rotation[0] - needRotation[0], 360);
		if (deltaAngle >  180) deltaAngle -= 360;
		if (deltaAngle < -180) deltaAngle += 360;
		
		
		float rotateAngle = deltaAngle;
		if (rotateAngle >  30) rotateAngle =  30;
		if (rotateAngle < -30) rotateAngle = -30;
		vector forwardVector = rotation.AnglesToVector();
		
		float currentSteering = vehicleWheeledSimulation.GetSteering();
		float needSteering = -rotateAngle/30;
		if (Math.AbsFloat(deltaAngle) > length*5 || backward > 0) {
			vehicleWheeledSimulation.SetGear(0);
			vehicleWheeledSimulation.SetThrottle(vector.Dot(-forwardVector, moveVector) + 1);
			needSteering = -needSteering;
			
			if (backward < 0) backward = 1;
		} else {
			vehicleWheeledSimulation.SetGear(4);
			vehicleWheeledSimulation.SetThrottle(vector.Dot(forwardVector, moveVector) + 1);
		}
		if (needSteering >  1) needSteering =  1;
		if (needSteering < -1) needSteering = -1;
		float steeringChange = needSteering - currentSteering;
		if (steeringChange >  0.01) steeringChange =  0.01;
		if (steeringChange < -0.01) steeringChange = -0.01;
		vehicleWheeledSimulation.SetSteering(currentSteering + steeringChange * timeSlice * 1000);
		if (backward >= 0) backward -= timeSlice;
	}
};