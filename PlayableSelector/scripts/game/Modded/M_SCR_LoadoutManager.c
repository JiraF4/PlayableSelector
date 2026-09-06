modded class SCR_LoadoutManager
{
	// The base constructor calls RegisterLoadoutManager() before our modded body
	// runs, so initializing m_aPlayerLoadouts in the constructor is too late.
	// Instead we guard every method that iterates the array without a null check.

	override int GetLoadoutIndex(SCR_BasePlayerLoadout loadout)
	{
		if (!m_aPlayerLoadouts)
			return -1;

		foreach (int i, SCR_BasePlayerLoadout inst : m_aPlayerLoadouts)
		{
			if (inst == loadout)
				return i;
		}
		return -1;
	}

	override SCR_BasePlayerLoadout GetLoadoutByIndex(int index)
	{
		if (!m_aPlayerLoadouts)
			return null;

		if (index < 0 || index >= m_aPlayerLoadouts.Count())
			return null;

		return m_aPlayerLoadouts[index];
	}

	override SCR_BasePlayerLoadout GetLoadoutByName(string name, FactionKey faction = string.Empty)
	{
		if (!m_aPlayerLoadouts)
			return null;

		foreach (SCR_BasePlayerLoadout candidate : m_aPlayerLoadouts)
		{
			if (candidate.GetLoadoutName() == name)
			{
				auto factionLoadout = SCR_FactionPlayerLoadout.Cast(candidate);
				if (faction && factionLoadout && factionLoadout.GetFactionKey() != faction)
					continue;

				return candidate;
			}
		}

		return null;
	}

	override bool IsFactionSupportingCsutomLoadouts(notnull Faction faction)
	{
		if (!m_aPlayerLoadouts)
			return false;

		FactionKey factionKey = faction.GetFactionKey();
		SCR_PlayerArsenalLoadout playerLoadout;
		foreach (SCR_BasePlayerLoadout baseLoadout : m_aPlayerLoadouts)
		{
			playerLoadout = SCR_PlayerArsenalLoadout.Cast(baseLoadout);
			if (!playerLoadout)
				continue;

			if (playerLoadout.GetFactionKey() == factionKey)
				return true;
		}

		return false;
	}
}
