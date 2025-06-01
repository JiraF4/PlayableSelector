modded class SCR_MuzzleEffectComponent
{
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		
		PS_SpectatorDrawProj draw = PS_SpectatorDrawProj.Cast(projectileEntity.FindComponent(PS_SpectatorDrawProj));
		if(draw)
			draw.SetPos(projectileEntity.GetOrigin());
	}
}