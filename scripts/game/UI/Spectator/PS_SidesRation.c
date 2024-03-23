class PS_SidesRation : SCR_ScriptedWidgetComponent
{	
	protected ResourceName m_sRatioLine = "{5DA4D8022F3FE19B}UI/Spectator/SideRatioLine.layout";
	
	ref map<FactionKey, PS_SidesRatioLine> m_RatioLines = new map<FactionKey, PS_SidesRatioLine>();
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		if (!GetGame().InPlayMode())	
			return;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
			playableManager.m_eOnFactionChange.Insert(UpdateInfo);
		
		UpdateInfo(0, "", "");	
	}
	
	void UpdateInfo(int _playerId, FactionKey _factionKey, FactionKey _factionKeyOld)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		map<FactionKey, int> factionsCount = new map<FactionKey, int>();
		
		map<RplId, PS_PlayableComponent> playables = playableManager.GetPlayables();
		foreach (RplId id, PS_PlayableComponent playableComponent : playables)
		{
			SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetOwner());
			if (!chimeraCharacter)
				continue;
			SCR_CharacterDamageManagerComponent characterDamageManagerComponent = SCR_CharacterDamageManagerComponent.Cast(chimeraCharacter.FindComponent(SCR_CharacterDamageManagerComponent));
			if (!characterDamageManagerComponent)
				continue;
			if (characterDamageManagerComponent.IsDestroyed())
				continue;
			
			FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(chimeraCharacter.FindComponent(FactionAffiliationComponent));
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			if (factionsCount.Contains(factionKey))
				factionsCount[factionKey] = factionsCount[factionKey] + 1;
			else 
				factionsCount[factionKey] = 1;
		}
		
		foreach (FactionKey factionKey, PS_SidesRatioLine sidesRatioLine : m_RatioLines)
		{
			if (!factionsCount.Contains(factionKey))
			{
				sidesRatioLine.GetRootWidget().RemoveFromHierarchy();
				m_RatioLines.Remove(factionKey);
			}
		}
		
		foreach (FactionKey factionKey, int count : factionsCount)
		{
			if (!m_RatioLines.Contains(factionKey))
			{
				Widget lineWidget = GetGame().GetWorkspace().CreateWidgets(m_sRatioLine, m_wRoot);
				PS_SidesRatioLine sidesRatioLine = PS_SidesRatioLine.Cast(lineWidget.FindHandler(PS_SidesRatioLine));
				sidesRatioLine.SetFaction(factionKey);
				sidesRatioLine.SetCount(count);
				m_RatioLines[factionKey] = sidesRatioLine;
			}
			else
				m_RatioLines[factionKey].SetCount(count);
		}
	}
}