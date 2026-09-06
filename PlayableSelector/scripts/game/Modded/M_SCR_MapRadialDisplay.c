modded class SCR_MapRadialDisplay
{
	override protected bool DisplayStartDrawInit(IEntity owner) { m_eLayer = EHudLayers.ALWAYS_TOP; return super.DisplayStartDrawInit(owner); }
	override void Show(bool show, float speed = UIConstants.FADE_RATE_INSTANT, EAnimationCurve curve = EAnimationCurve.LINEAR) { m_bCanShow = true; super.Show(show, speed, curve); }
}
