modded class SCR_ChimeraCharacter 
{
	PS_PlayableComponent PS_m_PlayableComponent;
	
	void PS_SetPlayable(PS_PlayableComponent playableComponent)
	{
		PS_m_PlayableComponent = playableComponent;
	}
	
	PS_PlayableComponent PS_GetPlayable()
	{
		return PS_m_PlayableComponent;
	}
	
	static ref array<SCR_ChimeraCharacter> m_aCharacters_PS = {};
	
	void SCR_ChimeraCharacter(IEntitySource src, IEntity parent)
	{
		m_aCharacters_PS.Insert(this);
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && !gameModeCoop.IsFreezeTimeEnd())
		{
			GetGame().GetCallqueue().CallLater(FreezeTimeDisable, 0, true);
		}
		else
		{
			Activate();
		}
	}
	
	void FreezeTimeDisable()
	{
		Deactivate();
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.IsDisableTimeEnd())
		{
			Activate();
			GetGame().GetCallqueue().Remove(FreezeTimeDisable);
		}
	}
	
	void ~SCR_ChimeraCharacter()
	{
		m_aCharacters_PS.RemoveItem(this);
	}
}