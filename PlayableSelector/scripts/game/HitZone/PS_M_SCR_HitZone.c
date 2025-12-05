modded class SCR_HitZone
{
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT) {
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop)
		{
			if (gameModeCoop.IsFreezeTimeShootingForbiden() && !gameModeCoop.IsFreezeTimeEnd())
				return 0;
		}
		return super.ComputeEffectiveDamage(damageContext, isDOT);
	};
}