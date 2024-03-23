class PS_PlayableHelper
{
	static string GetPlayableGroupKey(PS_PlayableComponent playableComponent)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
		Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
		FactionKey factionKey = faction.GetFactionKey();
		
		int callsign = playableManager.GetGroupCallsignByPlayable(playableComponent.GetId());
		
		return factionKey + callsign.ToString();
	}
}