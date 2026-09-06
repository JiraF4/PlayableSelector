class PS_ReplayProjectileCollisionTrigger: BaseProjectileEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		if (!Replication.IsServer()) return;

		if (!instigator.GetInstigatorEntity()) return;

		RplComponent rpl = RplComponent.Cast(instigator.GetInstigatorEntity().FindComponent(RplComponent));
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter)
			replayWriter.WriteProjectileShoot(rpl.Id(), outMat[0][0], outMat[0][2]);
	}
}
