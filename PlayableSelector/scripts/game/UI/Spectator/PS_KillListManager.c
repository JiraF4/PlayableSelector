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

	/**
	 * @brief Очистка синглтона при удалении компонента
	 * @issue BUG-55
	 * @cause EOnDeactivate не является методом жизненного цикла ScriptComponent и никогда не вызывался
	 * @solution Использование override OnDelete(IEntity owner)
	 */
	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;
		super.OnDelete(owner);
	}

	void FillKillInfoWeaponAndDistance(IEntity victimEntity, IEntity killerEntity, int killerPlayerId, PS_KillInfo killInfo)
	{
		FillHitZone(victimEntity, killerPlayerId, killInfo);
		FillWeapon(killerEntity, killInfo);

		if (killerPlayerId > 0 && killerEntity)
			killInfo.m_fDistance = vector.Distance(killerEntity.GetOrigin(), victimEntity.GetOrigin());
		else
			killInfo.m_fDistance = -1;
	}

	void RPC_KillEvent(string killData)
	{
		array<string> parts = {};
		killData.Split("|", parts, true);
		if (parts.Count() < 9)
			return;

		PS_KillInfo killInfo = new PS_KillInfo();
		killInfo.m_iVictimPlayerId = parts[0].ToInt();
		killInfo.m_iKillerPlayerId = parts[1].ToInt();
		killInfo.m_sVictimName = parts[2];
		killInfo.m_sKillerName = parts[3];
		killInfo.m_sVictimSquad = parts[4];

		if (parts.Count() >= 10)
		{
			// 10 fields: victimId|killerId|victimName|killerName|victimSquad|weaponName|magazineName|distance|hitZoneGroup|isTeamKill
			killInfo.m_sWeaponName = parts[5];
			killInfo.m_sMagazineName = parts[6];
			killInfo.m_sAmmoType = parts[6];
			killInfo.m_fDistance = parts[7].ToFloat();
			killInfo.m_eLastHitZoneGroup = parts[8].ToInt();
			killInfo.m_bIsTeamKill = (parts[9] == "1");
		}
		else
		{
			// Legacy 9 fields: victimId|killerId|victimName|killerName|victimSquad|ammo|distance|hitZoneGroup|isTeamKill
			killInfo.m_sAmmoType = parts[5];
			killInfo.m_sMagazineName = parts[5];
			killInfo.m_fDistance = parts[6].ToFloat();
			killInfo.m_eLastHitZoneGroup = parts[7].ToInt();
			killInfo.m_bIsTeamKill = (parts[8] == "1");
		}

		if (m_aKillHistory.Count() >= 200)
			m_aKillHistory.Remove(0);
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
	}

	protected void FillWeapon(IEntity killerEntity, PS_KillInfo killInfo)
	{
		if (!killerEntity)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(killerEntity);
		if (!character)
		{
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(killerEntity.FindComponent(SCR_EditableEntityComponent));
			if (editable)
			{
				SCR_UIInfo info = editable.GetInfo();
				if (info)
					killInfo.m_sWeaponName = info.GetName();
			}
			return;
		}

		// If character is in a vehicle turret compartment
		CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(killerEntity.FindComponent(CompartmentAccessComponent));
		if (compAccess)
		{
			BaseCompartmentSlot slot = compAccess.GetCompartment();
			if (slot)
			{
				TurretCompartmentSlot turretSlot = TurretCompartmentSlot.Cast(slot);
				if (turretSlot)
				{
					IEntity turretEntity = turretSlot.GetOwner();
					if (turretEntity)
					{
						BaseWeaponManagerComponent turretWpnMgr = BaseWeaponManagerComponent.Cast(turretEntity.FindComponent(BaseWeaponManagerComponent));
						if (turretWpnMgr)
						{
							BaseWeaponComponent turretWeapon = turretWpnMgr.GetCurrentWeapon();
							if (turretWeapon && turretWeapon.GetUIInfo())
								killInfo.m_sWeaponName = turretWeapon.GetUIInfo().GetName();
						}
					}
				}
			}
		}

		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(killerEntity.FindComponent(CharacterControllerComponent));
		if (!charCtrl)
			return;

		BaseWeaponManagerComponent weaponMgr = charCtrl.GetWeaponManagerComponent();
		if (!weaponMgr)
			return;

		BaseWeaponComponent currentWeapon = weaponMgr.GetCurrentWeapon();
		if (currentWeapon)
		{
			UIInfo weaponUIInfo = currentWeapon.GetUIInfo();
			if (weaponUIInfo && killInfo.m_sWeaponName == "")
				killInfo.m_sWeaponName = weaponUIInfo.GetName();

			BaseMuzzleComponent muzzle = currentWeapon.GetCurrentMuzzle();
			if (muzzle)
			{
				BaseMagazineComponent mag = muzzle.GetMagazine();
				if (mag)
				{
					IEntity magEntity = mag.GetOwner();
					if (magEntity)
					{
						InventoryItemComponent magItem = InventoryItemComponent.Cast(magEntity.FindComponent(InventoryItemComponent));
						if (magItem)
						{
							ItemAttributeCollection attribs = magItem.GetAttributes();
							if (attribs && attribs.GetUIInfo())
								killInfo.m_sMagazineName = attribs.GetUIInfo().GetName();
						}
					}

					if (killInfo.m_sMagazineName == "")
					{
						MagazineUIInfo magUIInfo = MagazineUIInfo.Cast(mag.GetUIInfo());
						if (magUIInfo)
							killInfo.m_sMagazineName = magUIInfo.GetAmmoType();
					}
				}
			}

			if (killInfo.m_sMagazineName == "" && weaponUIInfo)
			{
				GrenadeUIInfo grenadeInfo = GrenadeUIInfo.Cast(weaponUIInfo);
				if (grenadeInfo)
					killInfo.m_sMagazineName = grenadeInfo.GetAmmoType();
			}
		}

		if (killInfo.m_sWeaponName == "" && currentWeapon && currentWeapon.GetUIInfo())
			killInfo.m_sWeaponName = currentWeapon.GetUIInfo().GetName();

		// Synchronize backward-compatible m_sAmmoType
		if (killInfo.m_sMagazineName != "")
			killInfo.m_sAmmoType = killInfo.m_sMagazineName;
		else if (killInfo.m_sWeaponName != "")
			killInfo.m_sAmmoType = killInfo.m_sWeaponName;
	}
}

void PS_ScriptInvokerKillEventMethod(PS_KillInfo killInfo);
typedef func PS_ScriptInvokerKillEventMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerKillEventMethod> PS_ScriptInvokerKillEvent;
