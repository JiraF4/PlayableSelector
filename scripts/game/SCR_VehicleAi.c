//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_VehicleAiClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class SCR_VehicleAi : ScriptComponent
{
	
	[Attribute("", UIWidgets.EditBox, "List of Waypoint names found in the level", category: "Group Waypoints")]
	ref array<string> m_aStaticWaypoints;
	
	Vehicle vehicle;
	AIPathfindingComponent aIPathfindingComponent;
	NavmeshWorldComponent navmeshWorldComponent;
	Physics physics;
	SCR_CarControllerComponent carControllerComponent;
	VehicleWheeledSimulation vehicleWheeledSimulation;
	SCR_BaseCompartmentManagerComponent compartmentManagerComponent;
	AIAgent pilotAgent;
	AIAgent gunnerAgent;
	
	ref array<vector> positions = new array<vector>();
	int cPosition = 0;
	float backward = 0;
	float alert = 0;
	
	override void OnPostInit(IEntity owner)
	{
        owner.SetFlags(EntityFlags.ACTIVE, true);
        SetEventMask( owner, EntityEvent.FRAME);
		
		GetGame().GetCallqueue().CallLater(AddWaypoints, 1, false);
		
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
			ev.RegisterScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
			ev.RegisterScriptHandler("OnCompartmentLeft", this, OnCompartmentLeft);
		}		
	}
	protected override void OnDelete(IEntity owner)
	{
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		if (ev)
		{
			ev.RemoveScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
			ev.RemoveScriptHandler("OnCompartmentLeft", this, OnCompartmentLeft);
		}	
	}
	
	void AddWaypoints()
	{
		for (int i = 0, length = m_aStaticWaypoints.Count(); i < length; i++)
		{
			IEntity wp = GetGame().GetWorld().FindEntityByName(m_aStaticWaypoints[i]);
			positions.Insert(wp.GetOrigin());
		}
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
			pilotAgent = aiControl.GetAIAgent();
			aiControl.DeactivateAI();
		}
		
		if (compartmentSlot.Type() == CargoCompartmentSlot)
		{
			gunnerAgent = aiControl.GetAIAgent();
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
			pilotAgent = null;
		}
		
		if (compartmentSlot.Type() == CargoCompartmentSlot)
		{
			gunnerAgent = null;
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
    {
		if (!pilotAgent || carControllerComponent.IsEngineDefective()) return;
		
		
		vector nextPosition = positions[cPosition];
		vector origin = owner.GetOrigin();
		vector moveVector = nextPosition - origin;
		float length = moveVector.Length();
		if (length < 4 && alert <= 0) {
			carControllerComponent.OnEngineStart();
			vehicleWheeledSimulation.SetBreak(0, false);
			vehicleWheeledSimulation.SetClutch(1);
			carControllerComponent.SetPersistentHandBrake(false);
			vehicleWheeledSimulation.EngineStart();
			
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
		
		int dangerEvents = 0;
		if (gunnerAgent) dangerEvents = gunnerAgent.GetDangerEventsCount();
		alert += dangerEvents * timeSlice * 2;
		
		
		if (Math.AbsFloat(deltaAngle) > length*5 || backward > 0) {
			vehicleWheeledSimulation.SetGear(0);
			float throttle = vector.Dot(-forwardVector, moveVector) + 1;
			if (throttle > 1) throttle = 1;
			if (alert >= 0) throttle *= 0.3;
			vehicleWheeledSimulation.SetThrottle(throttle);
			needSteering = -needSteering;
			if (backward < 0) backward = 1;
		} else {
			float throttle = vector.Dot(forwardVector, moveVector) + 1;
			if (throttle > 1) throttle = 1;
			if (alert >= 0) throttle *= 0.3;
			vehicleWheeledSimulation.SetGear(2);
			if (vehicleWheeledSimulation.GetSpeedKmh() > 20) vehicleWheeledSimulation.SetGear(3);
			if (vehicleWheeledSimulation.GetSpeedKmh() > 30) vehicleWheeledSimulation.SetGear(4);
			if (vehicleWheeledSimulation.GetSpeedKmh() > 40) vehicleWheeledSimulation.SetGear(5);
			vehicleWheeledSimulation.SetThrottle(throttle);
		}
		if (length < 4 && alert > 0) {
			vehicleWheeledSimulation.SetThrottle(0);
			vehicleWheeledSimulation.SetBreak(1, false);
		}
		
		/*
		Print(vehicleWheeledSimulation.GetBrake().ToString());
		Print(vehicleWheeledSimulation.GetClutch().ToString());
		Print(vehicleWheeledSimulation.EngineIsOn().ToString());
		Print(carControllerComponent.GetHandBrake().ToString());
		Print(vehicleWheeledSimulation.GetThrottle().ToString());
		Print(alert.ToString());
		Print(vehicleWheeledSimulation.GetGear().ToString());
		*/
		
		if (needSteering >  1) needSteering =  1;
		if (needSteering < -1) needSteering = -1;
		float steeringChange = needSteering - currentSteering;
		if (steeringChange >  0.01) steeringChange =  0.01;
		if (steeringChange < -0.01) steeringChange = -0.01;
		vehicleWheeledSimulation.SetSteering(currentSteering + steeringChange * timeSlice * 1000);
		if (backward >= 0) backward -= timeSlice;
		if (alert >= 0) alert -= timeSlice;
	}
};