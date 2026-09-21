class PS_KillInfo
{
	int m_iVictimPlayerId;
	int m_iKillerPlayerId;
	string m_sVictimName;
	string m_sKillerName;
	string m_sVictimSquad;   // victim's group/squad name (resolved server-side at kill time)
	string m_sAmmoType;      // kept for backward compatibility
	string m_sWeaponName;
	string m_sMagazineName;
	float m_fDistance;
	int m_eLastHitZoneGroup;
	bool m_bIsTeamKill;

	// Plain-English hit zone (kept for backward compatibility)
	static string HitZoneGroupToString(int group)
	{
		switch (group)
		{
			case ECharacterHitZoneGroup.HEAD:         return "Head";
			case ECharacterHitZoneGroup.UPPERTORSO:   return "Torso";
			case ECharacterHitZoneGroup.LOWERTORSO:   return "Stomach";
			case ECharacterHitZoneGroup.LEFTARM:      return "Left Arm";
			case ECharacterHitZoneGroup.RIGHTARM:     return "Right Arm";
			case ECharacterHitZoneGroup.LEFTLEG:      return "Left Leg";
			case ECharacterHitZoneGroup.RIGHTLEG:     return "Right Leg";
		}
		return "";
	}

	// Tactical Tarkov / Arena Breakout localized hit zone label with headshot badge
	static string HitZoneGroupToLocalized(int group)
	{
		bool isRu = (WidgetManager.Translate("#PS-KillList_Head") == "Голова");
		switch (group)
		{
			case ECharacterHitZoneGroup.HEAD:
				if (isRu) return "Голова";
				return "Head";
			case ECharacterHitZoneGroup.UPPERTORSO:
				if (isRu) return "Грудь";
				return "Thorax";
			case ECharacterHitZoneGroup.LOWERTORSO:
				if (isRu) return "Живот";
				return "Stomach";
			case ECharacterHitZoneGroup.LEFTARM:
				if (isRu) return "Левая рука";
				return "Left Arm";
			case ECharacterHitZoneGroup.RIGHTARM:
				if (isRu) return "Правая рука";
				return "Right Arm";
			case ECharacterHitZoneGroup.LEFTLEG:
				if (isRu) return "Левая нога";
				return "Left Leg";
			case ECharacterHitZoneGroup.RIGHTLEG:
				if (isRu) return "Правая нога";
				return "Right Leg";
		}
		return "";
	}

	string GetLocalizedHitZone()
	{
		return HitZoneGroupToLocalized(m_eLastHitZoneGroup);
	}

	static string DistanceToString(float distance)
	{
		if (distance < 0)
			return "";
		int meters = Math.Round(distance);
		return meters.ToString() + "m";
	}

	string GetFormattedDistance()
	{
		if (m_fDistance < 0)
			return "";
		int meters = Math.Round(m_fDistance);
		bool isRu = (WidgetManager.Translate("#PS-KillList_Head") == "Голова");
		if (isRu)
			return meters.ToString() + "м";
		return meters.ToString() + "m";
	}

	string GetWeaponDisplayName()
	{
		string weapon = Clean(m_sWeaponName);
		if (weapon == "")
			weapon = Clean(m_sAmmoType);
		if (weapon.StartsWith("#"))
			weapon = WidgetManager.Translate(weapon);
		return weapon;
	}

	string GetMagazineDisplayName()
	{
		string mag = Clean(m_sMagazineName);
		if (mag == "" && m_sWeaponName != "")
			mag = Clean(m_sAmmoType);
		if (mag == "")
			return "";
		if (mag.StartsWith("#"))
			mag = WidgetManager.Translate(mag);

		// Clean up redundant inventory words
		mag.Replace(" Пластиковый Магазин", "");
		mag.Replace(" Магазин", "");
		mag.Replace(" Magazine", "");
		mag.Replace("Магазин ", "");
		mag.Replace("Magazine ", "");
		return mag;
	}

	// Empty fields are sent as "-" placeholders (so the pipe-split never drops a token); treat them as empty.
	protected static string Clean(string value)
	{
		if (value == "-")
			return "";
		return value;
	}

	// Strip rich-text markup tags so names show as PLAIN text. The Podval mods (PodvalLobby) rewrite player
	// names to embed clan-tag markup, e.g. "<b><color rgba='226, 168, 79, 200'>[kak]</color></b>Vector";
	// peer-tool test names have none. The kill list wants plain nicknames, so remove any <...> tags here.
	protected static string StripMarkup(string value)
	{
		if (!value.Contains("<"))
			return value;
		string result = "";
		bool inTag = false;
		int len = value.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = value.Get(i);
			if (ch == "<")
				inTag = true;
			else if (ch == ">")
				inTag = false;
			else if (!inTag)
				result += ch;
		}
		return result;
	}

	// Victim label = "Squad Nickname" (squad omitted when unknown). Falls back to a live name lookup, then "AI".
	string GetVictimDisplayName()
	{
		string name = Clean(m_sVictimName);
		if (name == "" && m_iVictimPlayerId > 0)
			name = PS_PlayableManager.GetInstance().GetPlayerName(m_iVictimPlayerId);
		if (name == "")
			name = "AI";
		name = StripMarkup(name);

		string squad = Clean(m_sVictimSquad);
		if (squad != "")
			return squad + " " + name;
		return name;
	}

	// Killer label = nickname only (falls back to a live name lookup, then "AI").
	string GetKillerDisplayName()
	{
		string name = Clean(m_sKillerName);
		if (name == "" && m_iKillerPlayerId > 0)
			name = PS_PlayableManager.GetInstance().GetPlayerName(m_iKillerPlayerId);
		if (name == "")
			name = "AI";
		return StripMarkup(name);
	}

	// One row: "[N) ]<name>  <distance>  [<ammo>]  <hit zone>". Pass number<=0 to omit the numbering prefix
	// (used for the single "Killed by" line). Only the name is guaranteed; the rest append when present.
	string FormatLine(string displayName, int number = 0)
	{
		string result = "";
		if (number > 0)
			result += number.ToString() + ") ";
		result += displayName;

		string distance = DistanceToString(m_fDistance);
		if (distance != "")
			result += "  " + distance;

		string ammo = Clean(m_sAmmoType);
		if (ammo != "")
		{
			// Ammo comes through as a localization key (e.g. "#AR-AmmoType_AK_FMJ_Tracer"); resolve it so the
			// row shows the real caliber instead of the raw key (concatenated text is not auto-translated).
			if (ammo.StartsWith("#"))
				ammo = WidgetManager.Translate(ammo);
			result += "  [" + ammo + "]";
		}

		string hitZone = HitZoneGroupToString(m_eLastHitZoneGroup);
		if (hitZone != "")
			result += "  " + hitZone;

		return result;
	}

	bool IsVictim(int playerId)
	{
		return playerId > 0 && m_iVictimPlayerId == playerId;
	}

	bool IsKiller(int playerId)
	{
		return playerId > 0 && m_iKillerPlayerId == playerId;
	}
}
