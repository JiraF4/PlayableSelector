


class PS_ReplayProjectileCollisionTrigger: BaseProjectileEffect
{
	// GOD bless that ravage body
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, IEntity damageSourceGunner, IEntity damageSourceParent, string colliderName, float speed)
	{
		if (!Replication.IsServer()) return;
		
		if (!damageSourceParent) return;
		
		RplComponent rpl = RplComponent.Cast(damageSourceParent.FindComponent(RplComponent));
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		replayWriter.WriteProjectileShoot(rpl.Id(), outMat[0][0], outMat[0][2]);
	}
}