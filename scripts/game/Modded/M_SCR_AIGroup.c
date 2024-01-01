modded class SCR_AIGroup : ChimeraAIGroup
{
	[Attribute("", UIWidgets.EditBox, "Custom name", category: "Group")]
	string m_sCustomNameSet;
	
	[Attribute("", UIWidgets.CheckBox, "Set all characters playable", category: "Group")]
	bool m_bSetPlayable;
	
	override void EOnInit(IEntity owner)
	{
		// WHY ther's no way to set it in group directly without this stupid injection?
		if (m_iNameAuthorID == 0)
		{
			m_sCustomName = m_sCustomNameSet;
			m_iNameAuthorID = -1;
		}
		
		if (m_bSetPlayable)
			Event_OnInit.Insert(MakePlayable);
		
		super.EOnInit(owner);
	}
	
	void MakePlayable(SCR_AIGroup group)
	{
		array<AIAgent> outAgents = new array<AIAgent>();
		GetAgents(outAgents);
		foreach (AIAgent agent : outAgents)
		{
			IEntity entity = agent.GetControlledEntity();
			PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
			if (playableComponent)
				playableComponent.SetPlayable(true);
		}
	}
	
	void SetPlayable(bool playable)
	{
		m_bPlayable = playable;
	}
	
	override string GetCustomName()
	{
		// THERE IS NO WAY TO MAKE GROUP NAME FROM SERVER
		// WHY? THIS IS STUPID...
		
		if (RplSession.Mode() != RplMode.Dedicated)
			if (!GetGame().GetPlayerController().CanViewContentCreatedBy(m_iNameAuthorID) && m_iNameAuthorID != -1 /* -1 is server "player" since 0 = invalide*/)
				return string.Empty;
		
		return m_sCustomName;
	}
	
	int GetCalsignNum()
	{
		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(FindComponent(SCR_CallsignGroupComponent));
		int company, platoon, squad;
		callsignComponent.GetCallsignIndexes(company, platoon, squad);
		return 1000000 * company + 1000 * platoon + 1 * squad;
	}
}