// PS_M_SCR_CharacterControllerComponent.c
// Stop the body capture and refresh menu voice immediately upon character death.

modded class SCR_CharacterControllerComponent
{
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState, bool isJIP)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState, isJIP);

		if (newLifeState == ECharacterLifeState.DEAD)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && pc.GetControlledEntity() == GetOwner())
			{

				PS_MenuVoN.Refresh();
			}
		}
	}

	protected bool m_bPSFireLocked;

	protected bool PS_IsFreezeCombatLocked()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		return freezeManager && (freezeManager.IsFreezeSafetyActive() || freezeManager.IsHardFreezeActive());
	}

	protected bool PS_IsHardFreezeLocked()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		return freezeManager && freezeManager.IsHardFreezeActive();
	}

	protected bool PS_IsLocalOutFreezeTime()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (!freezeManager || freezeManager.GetFreezePhase() != PS_EFreezePhase.ORDINARY)
			return false;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetControlledEntity() != GetOwner())
			return false;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(
			pc.FindComponent(PS_PlayableControllerComponent)
		);
		return playableController && playableController.IsOutFreezeTime();
	}

	override bool GetCanMeleeAttack()
	{
		if (PS_IsFreezeCombatLocked())
			return false;

		return super.GetCanMeleeAttack();
	}

	override bool CanGetOutVehicleScript()
	{
		if (PS_IsHardFreezeLocked() || PS_IsLocalOutFreezeTime())
			return false;

		return super.CanGetOutVehicleScript();
	}

	override bool CanJumpOutVehicleScript()
	{
		if (PS_IsHardFreezeLocked() || PS_IsLocalOutFreezeTime())
			return false;

		return super.CanJumpOutVehicleScript();
	}

	override bool CanJumpClimb()
	{
		if (PS_IsHardFreezeLocked() || PS_IsLocalOutFreezeTime())
			return false;

		return super.CanJumpClimb();
	}

	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && am)
		{
			if (PS_IsFreezeCombatLocked())
			{
				SetWeaponNoFireTime(0.5);
				SetFireWeaponWanted(false);
				SetMeleeAttack(false);
				SetThrow(false, true);
				m_bPSFireLocked = true;

				am.SetActionValue("CharacterFire", 0);
				am.SetActionValue("CharacterThrowGrenade", 0);
				am.SetActionValue("CharacterMelee", 0);
				am.SetActionValue("CharacterFireStatic", 0);
				am.SetActionValue("TurretFire", 0);
				am.SetActionValue("VehicleFire", 0);
				am.SetActionValue("VehicleHorn", 0);
			}
			else if (m_bPSFireLocked)
			{
				SetWeaponNoFireTime(0);
				m_bPSFireLocked = false;
			}

			if (PS_IsLocalOutFreezeTime())
			{
				am.SetActionValue("CharacterForward", 0);
				am.SetActionValue("CharacterRight", 0);
				am.SetActionValue("GetOut", 0);
				am.SetActionValue("JumpOut", 0);
				am.SetActionValue("CharacterStand", 0);
				am.SetActionValue("CharacterCrouch", 0);
				am.SetActionValue("CharacterProne", 0);
				am.SetActionValue("CharacterStandCrouchToggle", 0);
				am.SetActionValue("CharacterStandProneToggle", 0);
				am.SetActionValue("CharacterRoll", 0);
				am.SetActionValue("CharacterJump", 0);

				if (owner)
				{
					Vehicle vehicle = Vehicle.Cast(owner.GetRootParent());
					if (vehicle)
						PS_DisableVehicleMove(vehicle, am);
				}
			}
		}

		super.OnPrepareControls(owner, am, dt, player);
	}

	protected override void OnApplyControls(IEntity owner, float timeSlice)
	{
		super.OnApplyControls(owner, timeSlice);

		if (!owner || owner != SCR_PlayerController.GetLocalControlledEntity())
			return;

		if (PS_IsFreezeCombatLocked())
		{
			SetWeaponNoFireTime(0.5);
			SetFireWeaponWanted(false);
			SetMeleeAttack(false);

			CharacterInputContext inputCtx = GetInputContext();
			if (inputCtx)
			{
				inputCtx.SetWeaponPullTrigger(false);
				inputCtx.SetMeleeAttack(false);
				if (inputCtx.GetThrow())
					inputCtx.CancelThrow();
				inputCtx.SetThrow(false);
				inputCtx.SetVehicleHorn(0);
			}
		}

		if (PS_IsLocalOutFreezeTime())
		{
			SetRoll(0);
			SetJump(0);

			CharacterInputContext inputCtx = GetInputContext();
			if (inputCtx)
			{
				inputCtx.SetRoll(0);
				inputCtx.SetJump(0);
			}
		}
	}

	protected void PS_DisableVehicleMove(notnull Vehicle vehicle, notnull ActionManager am)
	{
		am.SetActionValue("VehicleEngineStop", 1);
		am.SetActionValue("VehicleEngineStart", 0);
		am.SetActionValue("AutohoverToggle", 0);
		am.SetActionValue("WheelBrake", 0);
		am.SetActionValue("WheelBrakePersistent", 1);
		am.SetActionValue("CyclicForward", 0);
		am.SetActionValue("CyclicBack", 0);
		am.SetActionValue("CyclicLeft", 0);
		am.SetActionValue("CyclicRight", 0);
		am.SetActionValue("AntiTorqueLeft", 0);
		am.SetActionValue("AntiTorqueRight", 0);
		am.SetActionValue("CollectiveIncrease", 0);
		am.SetActionValue("CollectiveDecrease", 0);
		am.SetActionValue("HelicopterEngineStop", 0);
		am.SetActionValue("HelicopterEngineStart", 0);
		am.SetActionValue("CarThrust", 0);
		am.SetActionValue("CarBrake", 1);
		am.SetActionValue("CarSteering", 0);
		am.SetActionValue("CarTurbo", 0);
		am.SetActionValue("CarTurboToggle", 0);
		am.SetActionValue("CarShift", 0);
		am.SetActionValue("CarShiftReverse", 0);
		am.SetActionValue("CarHandBrake", 1);
		am.SetActionValue("CarHandBrakePersistent", 1);

		CarControllerComponent car = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
		if (car)
		{
			if (car.IsEngineOn())
				car.StopEngine(false);
			car.SetPersistentHandBrake(true);
		}

		TrackedControllerComponent tracked = TrackedControllerComponent.Cast(vehicle.FindComponent(TrackedControllerComponent));
		if (tracked && tracked.IsEngineOn())
			tracked.StopEngine(false);

		VehicleWheeledSimulation wheeledSim = VehicleWheeledSimulation.Cast(vehicle.FindComponent(VehicleWheeledSimulation));
		if (wheeledSim && wheeledSim.EngineIsOn())
			wheeledSim.EngineStop();

		HelicopterControllerComponent heli = HelicopterControllerComponent.Cast(vehicle.FindComponent(HelicopterControllerComponent));
		if (heli)
		{
			heli.SetPersistentWheelBrake(true);
			if (!heli.GetAutohoverEnabled())
			{
				heli.SetAutohoverEnabled(true);
				am.SetActionValue("AutohoverToggle", 1);
			}
		}
	}

	override protected void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		super.OnControlledByPlayer(owner, controlled);

		bool isLocal = controlled && owner && owner == SCR_PlayerController.GetLocalControlledEntity();

		GetGame().GetCallqueue().Remove(PS_FreezeFireTick);
		m_bPSFireLocked = false;

		if (isLocal)
		{
			PS_FreezeFireTick();
			GetGame().GetCallqueue().CallLater(PS_FreezeFireTick, 250, true);
		}
	}

	protected void PS_FreezeFireTick()
	{
		if (PS_IsFreezeCombatLocked())
		{
			SetWeaponNoFireTime(0.5);
			m_bPSFireLocked = true;
		}
		else if (m_bPSFireLocked)
		{
			SetWeaponNoFireTime(0);
			m_bPSFireLocked = false;
		}
	}

	void ~SCR_CharacterControllerComponent()
	{
		GetGame().GetCallqueue().Remove(PS_FreezeFireTick);
	}
}

