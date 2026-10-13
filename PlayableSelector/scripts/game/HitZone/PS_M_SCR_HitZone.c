modded class SCR_HitZone
{
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT)
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		if (freezeManager)
		{
			if ((freezeManager.IsFreezeSafetyActive() || freezeManager.IsHardFreezeActive()) && damageContext.damageType != EDamageType.TRUE)
				return 0;
		}
		else
		{
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (gameModeCoop && (gameModeCoop.IsFreezeTimeShootingForbiden() && !gameModeCoop.IsFreezeTimeEnd()) && damageContext.damageType != EDamageType.TRUE)
				return 0;
		}

		return super.ComputeEffectiveDamage(damageContext, isDOT);
	}
}