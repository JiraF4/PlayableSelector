// PS_M_SCR_VONController - keep ownership transitions local and stop old capture before replacement.

modded class SCR_VONController
{
	override void SetVONComponent(SCR_VoNComponent VONComp)
	{
		if (GetVONComponent() != VONComp)
			CleanupVONBeforeComponentChange();

		super.SetVONComponent(VONComp);
	}

	protected void CleanupVONBeforeComponentChange()
	{
		bool canReset = m_VONComp && m_DirectSpeechEntry && m_DirectSpeechEntry.IsUsable() && m_LongRangeEntry && m_LongRangeEntry.IsUsable();
		if (canReset)
		{
			ResetVON();
		}
		else
		{
			if (m_bIsToggledDirect)
			{
				if (m_VONComp && m_DirectSpeechEntry && m_DirectSpeechEntry.IsUsable())
					SetVONProximityToggle(false);
				else
				{
					m_bIsToggledDirect = false;
					if (m_OnVONActiveToggled)
						m_OnVONActiveToggled.Invoke(false, false);
				}
			}

			if (m_bIsLongRangeToggled && m_LongRangeEntry && m_LongRangeEntry.IsUsable())
				SetVONLongRange(false, false);
		}

		if (m_VONComp)
		{
			DeactivateVON();
			m_VONComp.SetCapture(false);
		}

		if (m_bIsToggledDirect)
		{
			m_bIsToggledDirect = false;
			if (m_OnVONActiveToggled)
				m_OnVONActiveToggled.Invoke(false, false);
		}

		m_bIsLongRangeToggled = false;
		m_bIsActive = false;
		m_eVONType = EVONTransmitType.NONE;
		m_sActiveHoldAction = string.Empty;
		m_fToggleOffDelay = 0;
	}

	override protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		PS_MenuVoN.Refresh();
	}

	override bool AssignVONComponent()
	{
		if (PS_MenuVoN.ShouldOwnVoice())
			return false;

		return super.AssignVONComponent();
	}

	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{
		if (PS_MenuVoN.ShouldOwnVoice())
			return;

		if (!GetVONComponent())
			AssignVONComponent();
		if (!GetVONComponent())
			return;

		super.SetActiveTransmit(entry);
	}

	override protected void SetVONProximityToggle(bool activate)
	{
		if (!m_DirectSpeechEntry)
			return;

		super.SetVONProximityToggle(activate);
	}

	override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		if (PS_MenuVoN.ShouldOwnVoice())
			return false;

		return super.ActivateVON(entry, transmitType);
	}
}
