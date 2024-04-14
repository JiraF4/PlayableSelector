class PS_PolyZoneEffectHUDTriggerCapture : PS_PolyZoneEffectHUD
{
	PS_PolyZoneObjectiveTriggerCapture m_PolyZoneObjectiveTriggerCapture;
	
	override void SetString(string str)
	{
		super.SetString(str);
		
		m_PolyZoneObjectiveTriggerCapture = PS_PolyZoneObjectiveTriggerCapture.Cast(GetGame().GetWorld().FindEntityByName(str));
	}
	
	override void Update(float timeSlice)
	{
		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		FactionAffiliationComponent factionAffiliationComponent = chimeraCharacter.PS_GetPlayable().GetFactionAffiliationComponent();
		Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
		FactionKey factionKey = faction.GetFactionKey();
		if (m_PolyZoneObjectiveTriggerCapture.IsCurrentFaction(factionKey))
		{
			m_wCounter.SetText("Captured");
			return;
		}
		m_fTime = m_PolyZoneObjectiveTriggerCapture.GetFactionTime(factionKey);
		super.Update(0);
	}
}

