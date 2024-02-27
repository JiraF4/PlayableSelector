modded class SCR_CharacterDamageManagerComponent
{
	override void UpdateConsciousness()
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return;
		
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;
		
		bool unconscious = ShouldBeUnconscious();
		controller.SetUnconscious(unconscious);
		if (!unconscious)
		{
			GetGame().GetCallqueue().Call(ForceDisableAI, character);
			return;
		}

		// If unconsciousness is not allowed, kill character
		if (!GetPermitUnconsciousness())
		{
			Kill(GetInstigator());
			return;
		}

		if (m_pBloodHitZone && m_pBloodHitZone.GetDamageState() == ECharacterBloodState.DESTROYED)
		{
			Kill(GetInstigator());
			return;
		}
	}
	
	void ForceDisableAI(ChimeraCharacter character)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(character.FindComponent(PS_PlayableComponent));
		if (playableComponent && playableComponent.GetPlayable())
		{
			AIControlComponent aiComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			if (aiComponent)
			{
				AIAgent agent = aiComponent.GetAIAgent();
				if (agent) 
					agent.DeactivateAI();
			}
		}
	}
}