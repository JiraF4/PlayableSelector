[BaseContainerProps()]
class PS_PolyZoneEffectTriggerCapture : PS_PolyZoneEffect
{
	[Attribute("")]
	string m_sTriggerName;
	
	override void OnFrame(PS_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		
	}
	
	override void OnActivate(PS_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override void OnDeactivate(PS_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override PS_EffectContainer GetEffectContainer()
	{
		PS_EffectContainer effect = new PS_EffectContainer();
		effect.m_iId = m_iId;
		effect.m_sString = m_sTriggerName;
		effect.m_iType = PS_EPolyZoneEffectHUDType.TriggerCapture;
		return effect;
	}
	
	override PS_PolyZoneEffect CreateCopyObject()
	{
		return new PS_PolyZoneEffectTriggerCapture();
	}
	
	override void CopyFields(PS_PolyZoneEffect effect)
	{
		PS_PolyZoneEffectTriggerCapture effectCurrent = PS_PolyZoneEffectTriggerCapture.Cast(effect);
		effectCurrent.m_sTriggerName = m_sTriggerName;
	}
}