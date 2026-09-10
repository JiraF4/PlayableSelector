// PS_M_SCR_VONController - keep the VON push-to-talk input alive for body-less menu speakers,
// prevent corpse mic capture and handle null safety.

modded class SCR_VONController
{
	//------------------------------------------------------------------------------------------------
	override protected void Init(IEntity owner)
	{
		super.Init(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Cleanup()
	{
		super.Cleanup();
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateSystemState()
	{
		if (PS_MenuVoN.IsActive())
		{
			ConnectToHandleUpdateVONControllersSystem();
			return;
		}
		super.UpdateSystemState();
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		if (!m_InputManager || !PS_MenuVoN.IsActive())
			return;

		// Re-assert the contexts vanilla stops activating once the controlled character is dead/unconscious.
		m_InputManager.ActivateContext(VON_CONTEXT);
		m_InputManager.ActivateContext(VON_MENU_OPENING_CONTEXT);
	}

	//------------------------------------------------------------------------------------------------
	// Let PS_MenuVoN re-evaluate update-system registration when the menu device (de)activates.
	void PS_RefreshSystemState()
	{
		UpdateSystemState();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Проверка, является ли локальный игрок мертвым персонажем или спикером меню/спектатора.
	 * @return True если игрок мертв или в меню/спектаторе
	 */
	protected bool PS_IsDeadOrMenuSpeaker()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			pc = GetGame().GetPlayerController();
		if (!pc)
			return false;

		// Приоритет 1: Если под контролем живой персонаж - игрок 100% НЕ спикер меню!
		IEntity entity = pc.GetControlledEntity();
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent charCtrl = character.GetCharacterController();
			DamageManagerComponent dmg = character.GetDamageManager();
			bool isDead = (charCtrl && charCtrl.IsDead()) || (dmg && dmg.IsDestroyed());
			if (!isDead)
			{
				// Самоисцеление: если меню забыло деактивироваться, обновляем его
				if (PS_MenuVoN.IsActive())
					PS_MenuVoN.Refresh();
				return false;
			}
			return true;
		}

		// Приоритет 2: Меню активно при отсутствии живого персонажа
		if (PS_MenuVoN.IsActive())
			return true;

		if (SCR_VoNComponent.PS_IsMenuSpeaker(pc.GetPlayerId()))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool AssignVONComponent()
	{
		if (PS_IsDeadOrMenuSpeaker())
		{
			if (m_VONComp)
			{
				m_VONComp.SetCapture(false);
				SetVONComponent(null);
			}
			return false;
		}

		bool result = super.AssignVONComponent();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{
		if (PS_IsDeadOrMenuSpeaker())
		{
			if (m_VONComp)
			{
				m_VONComp.SetCapture(false);
				SetVONComponent(null);
			}
			return;
		}

		if (!GetVONComponent())
			AssignVONComponent();
		if (!GetVONComponent())
			return;

		super.SetActiveTransmit(entry);
	}

	//------------------------------------------------------------------------------------------------
	override protected void SetVONProximityToggle(bool activate)
	{
		if (!m_DirectSpeechEntry || PS_IsDeadOrMenuSpeaker())
		{
			if (m_VONComp && PS_IsDeadOrMenuSpeaker())
			{
				m_VONComp.SetCapture(false);
				SetVONComponent(null);
			}
			return;
		}

		super.SetVONProximityToggle(activate);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		if (PS_IsDeadOrMenuSpeaker())
		{
			if (m_VONComp)
				m_VONComp.SetCapture(false);
			return false;
		}

		return super.ActivateVON(entry, transmitType);
	}

	//------------------------------------------------------------------------------------------------
	override protected void DeactivateVON(EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		if (PS_IsDeadOrMenuSpeaker())
			return;

		super.DeactivateVON(transmitType);
	}

	//------------------------------------------------------------------------------------------------
	void PS_ResetVON()
	{
		ResetVON();
	}
}
