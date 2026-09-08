// PS_M_SCR_CharacterControllerComponent.c
// Mute mic, clear VoN component, power down body radios, and refresh menu voice immediately upon character death.

modded class SCR_CharacterControllerComponent
{
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState, bool isJIP)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState, isJIP);

		if (newLifeState == ECharacterLifeState.DEAD)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && pc.GetControlledEntity() == GetOwner())
			{
				SCR_VONController vonCtrl = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
				if (vonCtrl)
				{
					VoNComponent vonComp = vonCtrl.GetVONComponent();
					if (vonComp)
					{
						vonComp.SetCapture(false);
						vonCtrl.SetVONComponent(null);
					}
					vonCtrl.PS_ResetVON();
				}

				PS_PlayableControllerComponent playableCtrl = PS_PlayableControllerComponent.Cast(pc.FindComponent(PS_PlayableControllerComponent));
				if (playableCtrl)
					playableCtrl.DisableBodyVoNRadios();

				PS_MenuVoN.Refresh();
			}
		}
	}
}
