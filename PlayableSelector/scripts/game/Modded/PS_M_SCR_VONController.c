// PS_M_SCR_VONController - keep the VON push-to-talk input alive for body-less menu speakers,
// prevent corpse mic capture, handle null safety, and support voice volume modes (whisper / normal / shout).

modded class SCR_VONController
{
	protected const string PS_ACTION_VON_MODE_CYCLE = "PS_VoNModeCycle";

	protected PS_EVoNMode m_ePSVoNMode = PS_EVoNMode.NORMAL;
	protected bool m_bPSVoNModeHooked;

	//------------------------------------------------------------------------------------------------
	override protected void Init(IEntity owner)
	{
		super.Init(owner);

		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		if (m_bPSVoNModeHooked)
			return;

		if (!m_DirectSpeechEntry)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] SCR_VONController.Init: skipped hook (m_DirectSpeechEntry is null on controller %1)", this), LogLevel.NORMAL);
			return;
		}

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] SCR_VONController.Init: inputManager is null!", LogLevel.WARNING);
			return;
		}

		inputManager.AddActionListener(PS_ACTION_VON_MODE_CYCLE, EActionTrigger.DOWN, PS_ActionVoNModeCycle);
		m_bPSVoNModeHooked = true;

		if (PS_VoNModes.s_bDebugVoN)
			Print(string.Format("[PS_VoN] SCR_VONController.Init: SUCCESS! Action '%1' hooked on live controller %2", PS_ACTION_VON_MODE_CYCLE, this), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Cleanup()
	{
		if (m_bPSVoNModeHooked)
		{
			InputManager inputManager = GetGame().GetInputManager();
			if (inputManager)
				inputManager.RemoveActionListener(PS_ACTION_VON_MODE_CYCLE, EActionTrigger.DOWN, PS_ActionVoNModeCycle);

			m_bPSVoNModeHooked = false;

			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] SCR_VONController.Cleanup: unhooked action '%1'", PS_ACTION_VON_MODE_CYCLE), LogLevel.NORMAL);
		}

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

		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return;

		m_ePSVoNMode = PS_EVoNMode.NORMAL;
		if (PS_VoNModes.s_bDebugVoN)
			Print(string.Format("[PS_VoN] OnControlledEntityChanged: from=%1, to=%2. Mode reset to NORMAL.", from, to), LogLevel.NORMAL);
		PS_ApplyVoNMode();
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
		PS_ApplyVoNMode();
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
	protected void PS_ActionVoNModeCycle(float value, EActionTrigger reason)
	{
		if (PS_VoNModes.s_bDebugVoN)
			Print(string.Format("[PS_VoN] ActionVoNModeCycle pressed! Reason=%1, value=%2, CurrentMode=%3, MenuVoNActive=%4", reason, value, typename.EnumToString(PS_EVoNMode, m_ePSVoNMode), PS_MenuVoN.IsActive()), LogLevel.NORMAL);

		if (PS_IsDeadOrMenuSpeaker())
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] Cycle ignored: player is dead or menu/spectator active!", LogLevel.WARNING);
			return;
		}

		PS_EVoNMode previous = m_ePSVoNMode;
		m_ePSVoNMode = PS_VoNModes.GetNext(previous);

		if (PS_ApplyVoNMode())
		{
			if (PS_VoNModes.s_bDebugVoN)
			{
				string dbgText = string.Format("[VoN Mode]: %1 -> %2", typename.EnumToString(PS_EVoNMode, previous), typename.EnumToString(PS_EVoNMode, m_ePSVoNMode));
				Print("[PS_VoN] " + dbgText, LogLevel.NORMAL);
			}
			PS_VoNModeHud.Show(m_ePSVoNMode);
		}
		else
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] Failed to apply mode %1! Reverting back to %2", typename.EnumToString(PS_EVoNMode, m_ePSVoNMode), typename.EnumToString(PS_EVoNMode, previous)), LogLevel.WARNING);
			m_ePSVoNMode = previous;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool PS_ApplyVoNMode()
	{
		if (System.IsConsoleApp() || (RplSession.Mode() == RplMode.Dedicated))
			return false;

		if (PS_IsDeadOrMenuSpeaker())
		{
			if (m_VONComp)
			{
				m_VONComp.SetCapture(false);
				SetVONComponent(null);
			}
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] PS_ApplyVoNMode failed: player is dead or menu/spectator active!", LogLevel.WARNING);
			return false;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetOwner());
		if (!pc || pc.GetPlayerId() <= 0)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] PS_ApplyVoNMode failed: invalid player controller (pc=%1, id=%2)", pc, pc != null && pc.GetPlayerId()), LogLevel.WARNING);
			return false;
		}

		if (m_VONComp && m_VONComp.IsLocalActiveEditor())
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] PS_ApplyVoNMode failed: Game Master editor owns VoN", LogLevel.WARNING);
			return false;
		}

		ChimeraCharacter character = ChimeraCharacter.Cast(pc.GetControlledEntity());
		if (!character)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] PS_ApplyVoNMode failed: pc.GetControlledEntity() is null (no possessed character!)", LogLevel.WARNING);
			return false;
		}

		DamageManagerComponent dmg = character.GetDamageManager();
		if (dmg && dmg.IsDestroyed())
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print("[PS_VoN] PS_ApplyVoNMode failed: character is dead/destroyed!", LogLevel.WARNING);
			return false;
		}

		typename compType = PS_VoNModes.GetComponentType(m_ePSVoNMode);
		SCR_VoNComponent modeComp = SCR_VoNComponent.Cast(character.FindComponent(compType));
		if (!modeComp)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] PS_ApplyVoNMode CRITICAL: Component %1 NOT found on character %2! Check Character_Base prefab override!", compType, character), LogLevel.ERROR);
			return false;
		}

		if (m_VONComp == modeComp)
		{
			if (PS_VoNModes.s_bDebugVoN)
				Print(string.Format("[PS_VoN] PS_ApplyVoNMode: mode %1 already active on controller", typename.EnumToString(PS_EVoNMode, m_ePSVoNMode)), LogLevel.NORMAL);
			return true;
		}

		// Stop any transmission still keyed on the old component before swapping.
		if (m_VONComp)
			ResetVON();

		SetVONComponent(modeComp);

		if (PS_VoNModes.s_bDebugVoN)
			Print(string.Format("[PS_VoN] PS_ApplyVoNMode SUCCESS: Applied %1 (%2) -> SetVONComponent(%3)", typename.EnumToString(PS_EVoNMode, m_ePSVoNMode), compType, modeComp), LogLevel.NORMAL);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void PS_ResetVON()
	{
		ResetVON();
	}
}
