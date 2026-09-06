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
			//  GetGame().GetCallqueue().CallLater(FreezeTimeDisable, 0, true);
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
		
		if (!GetGame().InPlayMode())
			return;
		if (!GetGame().GetWorld())
			return;
		if (!Replication.IsServer())
			return;
		
		BaseGameMode gamemode = GetGame().GetGameMode();
		if (!gamemode)
			return;
		
		RplComponent rpl = RplComponent.Cast(this.FindComponent(RplComponent));
		if (rpl)
		{
			PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
			if (replayWriter)
				replayWriter.WriteEntityDelete(rpl.Id());
		}
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(this);
		if (!Replication.IsServer()) return;
		if (!GetGame().InPlayMode())
			return;
		GetGame().GetCallqueue().CallLater(RegisterToReplay, 0, false);
	}
	
	void RegisterToReplay()
	{
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter)
		{
			RplComponent rpl = RplComponent.Cast(this.FindComponent(RplComponent));
			SCR_CharacterDamageManagerComponent damageComponent = SCR_CharacterDamageManagerComponent.Cast(this.FindComponent(SCR_CharacterDamageManagerComponent));
			if (damageComponent)
			{
				ScriptInvoker damageEvent = damageComponent.GetOnDamageStateChanged();
				damageEvent.Insert(DieLogger);
			}
			replayWriter.WriteCharacterRegistration(rpl.Id(), this);
			GetGame().GetCallqueue().CallLater(ReplayPositionLogger, 0, false, rpl.Id());
		}
	}
	
	protected void ReplayPositionLogger(RplId rplId)
	{
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter)
			replayWriter.WriteEntityMove(rplId, this);
		GetGame().GetCallqueue().CallLater(ReplayPositionLogger, 500, false, rplId);
	}
	
	protected void DieLogger(EDamageState state)
	{
		RplComponent rpl = RplComponent.Cast(this.FindComponent(RplComponent));
		PS_ReplayWriter replayWriter = PS_ReplayWriter.GetInstance();
		if (replayWriter && rpl)
			replayWriter.WriteCharacterDamageStateChanged(rpl.Id(), state);
	}
}