modded class SCR_PlayerController
{
	PS_PlayableControllerComponent m_PS_PlayableControllerComponent;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_PS_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(owner.FindComponent(PS_PlayableControllerComponent));
	}
	
	PS_PlayableControllerComponent PS_GetPLayableComponent()
	{
		return m_PS_PlayableControllerComponent;
	}
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if (Replication.IsServer())
		{
			PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
			if (replayWriter)
			{
				if (to)
				{
					RplComponent rpl = RplComponent.Cast(to.FindComponent(RplComponent));
					if (rpl)
						replayWriter.WriteCharacterPossess(rpl.Id(), GetPlayerId());
				}
				else
				{
					replayWriter.WriteCharacterPossess(RplId.Invalid(), GetPlayerId());
				}
			}
		}
		super.OnControlledEntityChanged(from, to);
	}
}
