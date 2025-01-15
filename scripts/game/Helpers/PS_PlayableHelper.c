class PS_PlayableHelper
{
	static string GetPlayableGroupKey(PS_PlayableContainer playableContainer)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		FactionKey factionKey = playableContainer.GetFactionKey();
		
		int callsign = playableManager.GetGroupCallsignByPlayable(playableContainer.GetRplId());
		
		return factionKey + callsign.ToString();
	}
}