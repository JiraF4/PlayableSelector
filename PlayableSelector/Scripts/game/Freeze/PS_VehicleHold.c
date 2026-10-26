class PS_PinnedVehicle
{
	IEntity m_Entity;
	vector m_vOrigin;
	float m_fLinearDamping;
	float m_fAngularDamping;
	float m_fMaxDrift;
}

class PS_StoppedVehicle
{
	IEntity m_Entity;
	vector m_vOrigin;
}

class PS_VehicleHold
{
	protected static const float AIRBORNE_AGL_M = 2;
	protected static const float DRIFT_LOG_M = 1;
	protected static const float TRACE_DOWN_M = 1000;

	protected static bool s_bStartingHeldEngine;

	static bool IsStartingHeldEngine()
	{
		return s_bStartingHeldEngine;
	}

	protected ref array<ref PS_PinnedVehicle> m_aPinned = {};
	protected ref array<ref PS_StoppedVehicle> m_aStopped = {};
	protected int m_iStoppedCars;
	protected int m_iStoppedTracked;
	protected int m_iStoppedAircraft;

	static float MeasureAltitudeAGL(notnull IEntity vehicle)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		vector mins, maxs;
		vehicle.GetWorldBounds(mins, maxs);
		vector origin = vehicle.GetOrigin();
		vector start = Vector(origin[0], mins[1] - 0.5, origin[2]);

		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = Vector(start[0], start[1] - TRACE_DOWN_M, start[2]);
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Exclude = vehicle;

		float coef = world.TraceMove(trace, null);
		if (coef < 1)
			return TRACE_DOWN_M * coef;

		return start[1] - world.GetSurfaceY(start[0], start[2]);
	}

	protected static void ReadPrefabDamping(IEntity entity, out float linear, out float angular)
	{
		linear = 0;
		angular = 0;

		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return;

		IEntitySource source = IEntitySource.Cast(prefabData.GetPrefab());
		IEntityComponentSource rigidBody = SCR_BaseContainerTools.FindComponentSource(source, "RigidBody");
		if (!rigidBody)
			return;

		rigidBody.Get("LinearDamping", linear);
		rigidBody.Get("AngularDamping", angular);
	}

	protected static string Describe(notnull IEntity entity)
	{
		string name = entity.GetName();
		if (name != "")
			return name;

		return SCR_ResourceNameUtils.GetPrefabName(entity);
	}

	protected PS_PinnedVehicle Find(IEntity entity)
	{
		foreach (PS_PinnedVehicle pinned : m_aPinned)
		{
			if (pinned.m_Entity == entity)
				return pinned;
		}
		return null;
	}

	//! Server: pin all vehicles in the world to freeze physics instantly.
	void PinAll_S()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		m_iStoppedCars = 0;
		m_iStoppedTracked = 0;
		m_iStoppedAircraft = 0;

		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		world.QueryEntitiesByAABB(mins, maxs, HoldVehicle_S, null, EQueryEntitiesFlags.DYNAMIC);

		Print(string.Format("[PS_Lobby] Hold: stopped %1 car(s), %2 tracked, %3 grounded aircraft; %4 pinned",
			m_iStoppedCars, m_iStoppedTracked, m_iStoppedAircraft, m_aPinned.Count()), LogLevel.NORMAL);
	}

	//! Server: stop engines and apply persistent brake for vehicles on game start
	void StopAllVehicles_S()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		world.QueryEntitiesByAABB(mins, maxs, StopVehicleQuery_S, null, EQueryEntitiesFlags.DYNAMIC);
	}

	protected bool StopVehicleQuery_S(IEntity entity)
	{
		Vehicle vehicle = Vehicle.Cast(entity);
		if (!vehicle || vehicle.IsEnableMoveOnFreeze())
			return true;

		CarControllerComponent car = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
		TrackedControllerComponent tracked = TrackedControllerComponent.Cast(vehicle.FindComponent(TrackedControllerComponent));
		HelicopterControllerComponent heli = HelicopterControllerComponent.Cast(vehicle.FindComponent(HelicopterControllerComponent));

		if (car)
		{
			car.Shutdown();
			car.StopEngine(false);
			car.SetPersistentHandBrake(true);
		}
		else if (tracked)
		{
			tracked.Shutdown();
			tracked.StopEngine(false);
		}
		else if (heli)
		{
			float agl = MeasureAltitudeAGL(entity);
			if (agl <= AIRBORNE_AGL_M)
			{
				heli.SetPersistentWheelBrake(true);
				heli.StopEngine(false);
			}
		}

		return true;
	}

	protected bool HoldVehicle_S(IEntity entity)
	{
		Vehicle vehicle = Vehicle.Cast(entity);
		if (!vehicle || vehicle.IsEnableMoveOnFreeze())
			return true;

		HelicopterControllerComponent heli = HelicopterControllerComponent.Cast(entity.FindComponent(HelicopterControllerComponent));
		CarControllerComponent car = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
		TrackedControllerComponent tracked = TrackedControllerComponent.Cast(vehicle.FindComponent(TrackedControllerComponent));

		float agl = 0;
		if (heli)
			agl = MeasureAltitudeAGL(entity);

		Pin_S(entity, heli, car, tracked, agl);
		return true;
	}

	protected void Stop_S(notnull IEntity vehicle, HelicopterControllerComponent heli)
	{
		if (heli)
		{
			heli.SetPersistentWheelBrake(true);
			heli.SetAutohoverEnabled(true);
			heli.StopEngine(false);
			m_iStoppedAircraft++;
		}
		else
		{
			CarControllerComponent car = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
			TrackedControllerComponent tracked = TrackedControllerComponent.Cast(vehicle.FindComponent(TrackedControllerComponent));
			if (car)
			{
				car.Shutdown();
				car.StopEngine(false);
				car.SetPersistentHandBrake(true);
				m_iStoppedCars++;
			}
			else if (tracked)
			{
				tracked.Shutdown();
				tracked.StopEngine(false);
				m_iStoppedTracked++;
			}
			else
				return;
		}

		PS_StoppedVehicle stopped = new PS_StoppedVehicle();
		stopped.m_Entity = vehicle;
		stopped.m_vOrigin = vehicle.GetOrigin();
		m_aStopped.Insert(stopped);
	}

	protected void Pin_S(notnull IEntity vehicle, HelicopterControllerComponent heli, CarControllerComponent car, TrackedControllerComponent tracked, float agl)
	{
		if (Find(vehicle))
			return;

		if (!heli && !car && !tracked)
			return;

		Physics physics = vehicle.GetPhysics();
		if (!physics)
		{
			Stop_S(vehicle, heli);
			return;
		}

		PS_PinnedVehicle pinned = new PS_PinnedVehicle();
		pinned.m_Entity = vehicle;
		pinned.m_vOrigin = vehicle.GetOrigin();
		ReadPrefabDamping(vehicle, pinned.m_fLinearDamping, pinned.m_fAngularDamping);
		m_aPinned.Insert(pinned);

		TakeOwnership_S(vehicle);

		physics.SetLinearFactor(vector.Zero);
		physics.SetDamping(1000, 1000);
		physics.SetVelocity(vector.Zero);
		physics.SetAngularVelocity(vector.Zero);

		if (heli)
		{
			if (agl > AIRBORNE_AGL_M)
			{
				if (!heli.IsEngineOn())
				{
					ForceStartEngineParams params = new ForceStartEngineParams();
					params.m_bAirborne = true;
					s_bStartingHeldEngine = true;
					heli.ForceStartEngine(params);
					s_bStartingHeldEngine = false;
				}
				Print(string.Format("[PS_Lobby] Hold: pinned %1 at %2 m AGL", Describe(vehicle), Math.Round(agl)), LogLevel.NORMAL);
			}
			else
			{
				heli.SetPersistentWheelBrake(true);
				heli.SetAutohoverEnabled(true);
				heli.StopEngine(false);
				m_iStoppedAircraft++;
			}
		}
		else if (car)
		{
			car.Shutdown();
			car.StopEngine(false);
			car.SetPersistentHandBrake(true);
			m_iStoppedCars++;
		}
		else if (tracked)
		{
			tracked.Shutdown();
			tracked.StopEngine(false);
			m_iStoppedTracked++;
		}
	}

	protected void TakeOwnership_S(notnull IEntity vehicle)
	{
		RplComponent rpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!rpl || rpl.IsOwner())
			return;

		rpl.GiveExt(RplIdentity.Local(), false);
	}

	//! Server, per frame while held: keep ownership and measure drift
	void Tick_S()
	{
		foreach (PS_PinnedVehicle pinned : m_aPinned)
		{
			IEntity vehicle = pinned.m_Entity;
			if (!vehicle)
				continue;

			TakeOwnership_S(vehicle);

			float drift = vector.Distance(vehicle.GetOrigin(), pinned.m_vOrigin);
			if (drift > pinned.m_fMaxDrift)
				pinned.m_fMaxDrift = drift;
		}
	}

	protected void ReleaseStopped_S()
	{
		foreach (PS_StoppedVehicle stopped : m_aStopped)
		{
			IEntity vehicle = stopped.m_Entity;
			if (!vehicle)
				continue;

			float drift = vector.Distance(vehicle.GetOrigin(), stopped.m_vOrigin);
			if (drift > DRIFT_LOG_M)
				Print(string.Format("[PS_Lobby] Hold: %1 moved %2 m while stopped", Describe(vehicle), Math.Round(drift)), LogLevel.WARNING);
		}
		m_aStopped.Clear();
	}

	//! Server: release physics, autohover on, return vehicle ownership to driver
	void Release_S()
	{
		ReleaseStopped_S();

		foreach (PS_PinnedVehicle pinned : m_aPinned)
		{
			IEntity vehicle = pinned.m_Entity;
			if (!vehicle)
				continue;

			Physics physics = vehicle.GetPhysics();
			if (physics)
			{
				physics.SetLinearFactor(Vector(1, 1, 1));
				physics.SetDamping(pinned.m_fLinearDamping, pinned.m_fAngularDamping);
				physics.SetActive(ActiveState.ACTIVE);
			}

			HelicopterControllerComponent heli = HelicopterControllerComponent.Cast(vehicle.FindComponent(HelicopterControllerComponent));
			if (heli)
			{
				heli.SetAutohoverEnabled(true);
			}

			GiveToDriver_S(vehicle);

			if (pinned.m_fMaxDrift > DRIFT_LOG_M)
				Print(string.Format("[PS_Lobby] Hold: %1 drifted %2 m while pinned", Describe(vehicle), pinned.m_fMaxDrift), LogLevel.WARNING);
			else
				Print(string.Format("[PS_Lobby] Hold: released %1", Describe(vehicle)), LogLevel.NORMAL);
		}

		m_aPinned.Clear();
	}

	void Clear_S()
	{
		m_aStopped.Clear();
		m_aPinned.Clear();
	}

	protected void GiveToDriver_S(notnull IEntity vehicle)
	{
		Vehicle veh = Vehicle.Cast(vehicle);
		if (!veh)
			return;

		PilotCompartmentSlot driverSlot = veh.GetPilotCompartmentSlot();
		if (!driverSlot)
			return;

		IEntity driver = driverSlot.GetOccupant();
		if (!driver)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		int playerId = playerManager.GetPlayerIdFromControlledEntity(driver);
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		RplComponent rpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (playerController && rpl)
			rpl.GiveExt(playerController.GetRplIdentity(), false);
	}
}
