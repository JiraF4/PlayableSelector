[BaseContainerProps()]
class PS_PolyZoneEffectFreezeZone : PS_PolyZoneEffect
{
	[Attribute("10")]
	float m_fKillTime;
	
	bool m_bTriggerd = false;
	bool m_bOutside = false;
	
	override void OnFrame(PS_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.IsFreezeTimeEnd())
		{
			handler.RemoveByEffect(this);
			return;	
		}
		
		m_fKillTime -= timeSlice;
		if (m_fKillTime <= 0 && !m_bTriggerd)
		{
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
			if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
				return;
			damageManager.SetHealthScaled(0);
		}
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
		effect.m_fTime = m_fKillTime;
		effect.m_iType = PS_EPolyZoneEffectHUDType.FreezeZone;
		return effect;
	}
	
	override PS_PolyZoneEffect CreateCopyObject()
	{
		return new PS_PolyZoneEffectFreezeZone();
	}
	
	override void CopyFields(PS_PolyZoneEffect effect)
	{
		PS_PolyZoneEffectFreezeZone effectCurrent = PS_PolyZoneEffectFreezeZone.Cast(effect);
		effectCurrent.m_fKillTime = m_fKillTime;
	}
}