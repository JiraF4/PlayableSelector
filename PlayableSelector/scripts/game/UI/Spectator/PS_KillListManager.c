[ComponentEditorProps(category: "GameScripted/KillList", description: "Replicated kill event data relay for spectator kill list widget", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_KillListManagerClass : ScriptComponentClass
{
}

class PS_KillListManager : ScriptComponent
{
	protected ref array<ref PS_KillInfo> m_aKillHistory = {};

	static PS_KillListManager s_Instance;

	static PS_KillListManager GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		s_Instance = this;
		PrintFormat("[PS_KillListManager] OnPostInit owner=%1 isServer=%2 isClient=%3", owner, Replication.IsServer(), Replication.IsClient());
	}

	override void EOnDeactivate(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	void FillKillInfoWeaponAndDistance(IEntity victimEntity, IEntity killerEntity, int killerPlayerId, PS_KillInfo killInfo)
	{
		FillWeapon(killerEntity, killInfo);
		FillHitZone(victimEntity, killerPlayerId, killInfo);

		if (killInfo.m_sWeaponName == "#PS-KillList_CauseRoadkill" || killInfo.m_sAmmoType == "#PS-KillList_CauseRoadkill")
		{
			killInfo.m_fDistance = 0;
		}
		else if (killInfo.m_sWeaponName == "#PS-KillList_CauseFall" || killInfo.m_sWeaponName == "#PS-KillList_CauseBleeding")
		{
			killInfo.m_fDistance = 0;
		}
		else if (killerPlayerId > 0 && killerEntity && victimEntity)
		{
			killInfo.m_fDistance = vector.Distance(killerEntity.GetOrigin(), victimEntity.GetOrigin());
		}
		else
		{
			killInfo.m_fDistance = -1;
		}
	}

	void RPC_KillEvent(string killData)
	{
		array<string> parts = {};
		killData.Split("|", parts, true);
		if (parts.Count() < 9)
			return;

		// Format (16 fields): victimId|killerId|victimName|killerName|victimSquad|ammo|distance|hitZoneGroup|isTeamKill|weaponName|matchTimeSeconds|victimFactionKey|killerFactionKey|weaponIconPath|magazineCapacity|ammoCaliber
		PS_KillInfo killInfo = new PS_KillInfo();
		killInfo.m_iVictimPlayerId = parts[0].ToInt();
		killInfo.m_iKillerPlayerId = parts[1].ToInt();
		killInfo.m_sVictimName = parts[2];
		killInfo.m_sKillerName = parts[3];
		killInfo.m_sVictimSquad = parts[4];
		killInfo.m_sAmmoType = parts[5];
		killInfo.m_fDistance = parts[6].ToFloat();
		killInfo.m_eLastHitZoneGroup = parts[7].ToInt();
		killInfo.m_bIsTeamKill = (parts[8] == "1");

		if (parts.Count() >= 10)
			killInfo.m_sWeaponName = parts[9];
		if (parts.Count() >= 11)
			killInfo.m_iMatchTimeSeconds = parts[10].ToInt();
		if (parts.Count() >= 12)
			killInfo.m_sVictimFactionKey = parts[11];
		if (parts.Count() >= 13)
			killInfo.m_sKillerFactionKey = parts[12];
		if (parts.Count() >= 14)
			killInfo.m_sWeaponIconPath = parts[13];
		if (parts.Count() >= 15)
			killInfo.m_iMagazineCapacity = parts[14].ToInt();
		if (parts.Count() >= 16)
			killInfo.m_sAmmoCaliber = parts[15];

		m_aKillHistory.Insert(killInfo);
		PrintFormat("[PS_KillListManager] RPC_KillEvent victimId=%1 killerId=%2 isServer=%3 history=%4",
			killInfo.m_iVictimPlayerId, killInfo.m_iKillerPlayerId, Replication.IsServer(), m_aKillHistory.Count());
		m_eOnKillEvent.Invoke(killInfo);
	}

	protected ref ScriptInvokerBase<PS_ScriptInvokerKillEventMethod> m_eOnKillEvent = new ScriptInvokerBase<PS_ScriptInvokerKillEventMethod>();
	ScriptInvokerBase<PS_ScriptInvokerKillEventMethod> GetOnKillEvent()
	{
		return m_eOnKillEvent;
	}

	array<ref PS_KillInfo> GetKillHistory()
	{
		return m_aKillHistory;
	}

	protected void FillHitZone(IEntity victimEntity, int killerPlayerId, PS_KillInfo killInfo)
	{
		SCR_ChimeraCharacter victim = SCR_ChimeraCharacter.Cast(victimEntity);
		if (!victim)
			return;

		SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(victim.GetDamageManager());
		if (!dmgMgr)
			return;

		HitZone defaultHitZone = dmgMgr.GetDefaultHitZone();
		array<ref BaseDamageEffect> damageHistory = {};
		dmgMgr.GetDamageHistory(damageHistory);

		BaseDamageEffect relevantEffect;
		HitZone relevantHitZone;
		for (int i = damageHistory.Count() - 1; i >= 0; i--)
		{
			BaseDamageEffect effect = damageHistory[i];
			if (!effect)
				continue;
			EDamageType dmgType = effect.GetDamageType();
			if (dmgType == EDamageType.HEALING || dmgType == EDamageType.REGENERATION)
				continue;
			if (dmgType == EDamageType.BLEEDING && effect.GetTotalDamage() < 1)
				continue;
			Instigator effectInstigator = effect.GetInstigator();
			if (!effectInstigator || effectInstigator.GetInstigatorPlayerID() != killerPlayerId)
				continue;
			HitZone hitZone = effect.GetAffectedHitZone();
			if (!hitZone || hitZone == defaultHitZone || hitZone.GetName() == "Resilience")
				continue;
			relevantEffect = effect;
			relevantHitZone = hitZone;
			break;
		}

		if (relevantHitZone)
		{
			SCR_CharacterHitZone charHitZone = SCR_CharacterHitZone.Cast(relevantHitZone);
			if (charHitZone)
				killInfo.m_eLastHitZoneGroup = charHitZone.GetHitZoneGroup();
		}

		if (relevantEffect)
		{
			EDamageType fatalDmgType = relevantEffect.GetDamageType();
			if (fatalDmgType == EDamageType.EXPLOSIVE || fatalDmgType == EDamageType.FRAGMENTATION)
			{
				// If weapon was already resolved from vehicle turret or launcher (e.g. RPG, BTR cannon, GP-25),
				// preserve it. If not resolved yet, assign the localized Grenade cause.
				if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
				{
					killInfo.m_sWeaponName = "#PS-KillList_CauseGrenade";
					killInfo.m_sWeaponIconPath = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset@Grenade";
					killInfo.m_sAmmoType = "";
					killInfo.m_iMagazineCapacity = -1;
					killInfo.m_sAmmoCaliber = "";
				}
			}
			else if (fatalDmgType == EDamageType.BLEEDING)
			{
				killInfo.m_sWeaponName = "#PS-KillList_CauseBleeding";
				killInfo.m_sAmmoType = "";
				killInfo.m_sWeaponIconPath = "-";
				killInfo.m_iMagazineCapacity = -1;
				killInfo.m_sAmmoCaliber = "";
			}
			else if (fatalDmgType == EDamageType.COLLISION)
			{
				bool isFall = (SCR_FallDamageEffect.Cast(relevantEffect) != null);
				if (!isFall && (killerPlayerId <= 0 || killerPlayerId == killInfo.m_iVictimPlayerId))
				{
					if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
						isFall = true;
				}

				if (isFall)
				{
					killInfo.m_sWeaponName = "#PS-KillList_CauseFall";
					killInfo.m_sWeaponIconPath = "-";
				}
				else
				{
					if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
						killInfo.m_sWeaponName = "#PS-KillList_CauseRoadkill";
						killInfo.m_sWeaponIconPath = "-";
				}
				killInfo.m_sAmmoType = "";
				killInfo.m_iMagazineCapacity = -1;
				killInfo.m_sAmmoCaliber = "";
			}
		}
	}

	protected void FillWeapon(IEntity killerEntity, PS_KillInfo killInfo)
	{
		if (!killerEntity)
			return;

		// 1. If killerEntity is a vehicle entity directly:
		FillFromVehicle(killerEntity, killInfo);

		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(killerEntity.FindComponent(CharacterControllerComponent));
		if (!charCtrl)
			return;

		// 2. If character is inside a vehicle compartment (e.g. BTR gunner / driver):
		CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(killerEntity.FindComponent(CompartmentAccessComponent));
		if (compAccess && compAccess.IsInCompartment())
		{
			BaseCompartmentSlot compSlot = compAccess.GetCompartment();
			if (compSlot)
			{
				TurretCompartmentSlot turretSlot = TurretCompartmentSlot.Cast(compSlot);
				if (turretSlot)
				{
					TurretControllerComponent turretCtrl = TurretControllerComponent.Cast(turretSlot.GetController());
					if (turretCtrl)
					{
						BaseWeaponManagerComponent turretWpnMgr = turretCtrl.GetWeaponManager();
						if (turretWpnMgr)
						{
							BaseWeaponComponent turretWeapon = turretWpnMgr.GetCurrentWeapon();
							if (!turretWeapon)
							{
								array<IEntity> turretWeaponEntities = {};
								turretWpnMgr.GetWeaponsList(turretWeaponEntities);
								if (!turretWeaponEntities.IsEmpty() && turretWeaponEntities[0])
									turretWeapon = BaseWeaponComponent.Cast(turretWeaponEntities[0].FindComponent(BaseWeaponComponent));
							}
							if (turretWeapon)
								FillWeaponFromComponent(turretWeapon, killInfo);
						}
					}
				}

				// Fallback to vehicle name / icon if turret weapon had no name:
				if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
				{
					IEntity veh = compSlot.GetOwner();
					if (veh)
						FillFromVehicle(veh.GetRootParent(), killInfo);
				}
			}
		}

		// 3. Hand-held weapon (if not already resolved from vehicle turret):
		if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
		{
			BaseWeaponManagerComponent weaponMgr = charCtrl.GetWeaponManagerComponent();
			if (weaponMgr)
			{
				BaseWeaponComponent currentWeapon = weaponMgr.GetCurrentWeapon();
				if (currentWeapon)
					FillWeaponFromComponent(currentWeapon, killInfo);
			}
		}
	}

	protected void FillWeaponFromComponent(BaseWeaponComponent currentWeapon, PS_KillInfo killInfo)
	{
		if (!currentWeapon)
			return;

		IEntity weaponEntity = currentWeapon.GetOwner();
		if (weaponEntity && weaponEntity.GetPrefabData())
		{
			killInfo.m_sWeaponIconPath = weaponEntity.GetPrefabData().GetPrefabName();
		}

		UIInfo weaponUIInfo = currentWeapon.GetUIInfo();
		if (weaponUIInfo)
		{
			killInfo.m_sWeaponName = weaponUIInfo.GetName();
			if (killInfo.m_sWeaponIconPath == "")
				killInfo.m_sWeaponIconPath = weaponUIInfo.GetIconPath();
		}

		BaseMuzzleComponent muzzle = currentWeapon.GetCurrentMuzzle();
		if (muzzle)
		{
			BaseMagazineComponent mag = muzzle.GetMagazine();
			if (mag)
			{
				killInfo.m_iMagazineCapacity = mag.GetMaxAmmoCount();

				string ammoName = "";
				int ammoFlags = 0;
				string ammoTypeName = "";

				MagazineUIInfo magUIInfo = MagazineUIInfo.Cast(mag.GetUIInfo());
				if (magUIInfo)
				{
					killInfo.m_sAmmoCaliber = magUIInfo.GetAmmoCaliber();
					ammoName = magUIInfo.GetAmmoType();
					ammoFlags = magUIInfo.GetAmmoTypeFlags();
				}

				if (ammoName == "")
				{
					IEntity magEntity = mag.GetOwner();
					if (magEntity)
					{
						InventoryItemComponent magItem = InventoryItemComponent.Cast(magEntity.FindComponent(InventoryItemComponent));
						if (magItem)
						{
							ItemAttributeCollection attribs = magItem.GetAttributes();
							if (attribs && attribs.GetUIInfo())
								ammoName = attribs.GetUIInfo().GetName();
						}
					}
				}

				if (ammoFlags != 0)
				{
					if (ammoFlags & EAmmoType.AP) ammoTypeName = "AP";
					else if (ammoFlags & EAmmoType.HEAT) ammoTypeName = "HEAT";
					else if (ammoFlags & EAmmoType.HE) ammoTypeName = "HE";
					else if (ammoFlags & EAmmoType.FRAG) ammoTypeName = "FRAG";
					else if (ammoFlags & EAmmoType.SNIPER) ammoTypeName = "SNIPER";
					else if (ammoFlags & EAmmoType.FMJ) ammoTypeName = "FMJ";
					else if (ammoFlags & EAmmoType.TRACER) ammoTypeName = "TRACER";
				}

				if (ammoName != "" || ammoFlags != 0)
					killInfo.m_sAmmoType = ammoName + "#" + ammoFlags.ToString() + "#" + ammoTypeName;
			}
		}

		if (killInfo.m_sAmmoType == "" && weaponUIInfo)
		{
			GrenadeUIInfo grenadeInfo = GrenadeUIInfo.Cast(weaponUIInfo);
			if (grenadeInfo)
			{
				string gName = grenadeInfo.GetAmmoType();
				int gFlags = grenadeInfo.GetAmmoTypeFlags();
				string gTypeName = "";
				if (gFlags != 0)
				{
					if (gFlags & EAmmoType.HE) gTypeName = "HE";
					else if (gFlags & EAmmoType.FRAG) gTypeName = "FRAG";
					else if (gFlags & EAmmoType.HEAT) gTypeName = "HEAT";
					else if (gFlags & EAmmoType.SMOKE) gTypeName = "SMOKE";
					else if (gFlags & EAmmoType.ILLUMINATION) gTypeName = "ILLUM";
				}
				killInfo.m_sAmmoType = gName + "#" + gFlags.ToString() + "#" + gTypeName;
			}
		}
	}

	protected void FillFromVehicle(IEntity vehicleEntity, PS_KillInfo killInfo)
	{
		if (!vehicleEntity)
			return;

		SCR_EditableVehicleComponent editableVehicle = SCR_EditableVehicleComponent.Cast(vehicleEntity.FindComponent(SCR_EditableVehicleComponent));
		if (editableVehicle)
		{
			SCR_UIInfo vehicleInfo = editableVehicle.GetInfo();
			if (vehicleInfo)
			{
				if (killInfo.m_sWeaponName == "" || killInfo.m_sWeaponName == "-")
					killInfo.m_sWeaponName = vehicleInfo.GetName();
				if (killInfo.m_sWeaponIconPath == "" || killInfo.m_sWeaponIconPath == "-")
					killInfo.m_sWeaponIconPath = vehicleInfo.GetIconPath();
			}
		}

		if (vehicleEntity.GetPrefabData() && (killInfo.m_sWeaponIconPath == "" || killInfo.m_sWeaponIconPath == "-"))
		{
			killInfo.m_sWeaponIconPath = vehicleEntity.GetPrefabData().GetPrefabName();
		}
	}
}

void PS_ScriptInvokerKillEventMethod(PS_KillInfo killInfo);
typedef func PS_ScriptInvokerKillEventMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerKillEventMethod> PS_ScriptInvokerKillEvent;
