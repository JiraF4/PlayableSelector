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

	protected bool m_bPSFireLocked;

	override protected void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		super.OnControlledByPlayer(owner, controlled);

		bool isLocal = controlled && owner && owner == SCR_PlayerController.GetLocalControlledEntity();

		GetGame().GetCallqueue().Remove(PS_FreezeFireTick);
		m_bPSFireLocked = false;

		if (isLocal)
		{
			PS_FreezeFireTick();
			GetGame().GetCallqueue().CallLater(PS_FreezeFireTick, 250, true);
		}
	}

	protected void PS_FreezeFireTick()
	{
		PS_FreezeManager freezeManager = PS_FreezeManager.GetInstance();
		bool active = freezeManager && (freezeManager.IsFreezeSafetyActive() || freezeManager.IsHardFreezeActive());

		if (active)
		{
			SetWeaponNoFireTime(0.5);
			m_bPSFireLocked = true;
		}
		else if (m_bPSFireLocked)
		{
			SetWeaponNoFireTime(0);
			m_bPSFireLocked = false;
		}
	}

	void ~SCR_CharacterControllerComponent()
	{
		GetGame().GetCallqueue().Remove(PS_FreezeFireTick);
	}
}

