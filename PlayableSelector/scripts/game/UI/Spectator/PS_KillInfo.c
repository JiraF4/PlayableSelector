class PS_KillInfo
{
	int m_iVictimPlayerId;
	int m_iKillerPlayerId;
	string m_sVictimName;
	string m_sKillerName;
	string m_sVictimSquad;   // victim's group/squad name (resolved server-side at kill time)
	string m_sAmmoType;
	string m_sWeaponName;
	float m_fDistance;
	int m_eLastHitZoneGroup;
	bool m_bIsTeamKill;

	int m_iMatchTimeSeconds;
	string m_sVictimFactionKey;
	string m_sKillerFactionKey;
	ResourceName m_sWeaponIconPath;
	int m_iMagazineCapacity = -1;
	string m_sAmmoCaliber;

	// Plain-English hit zone (NOT a localization key): the row is built by concatenation, so an embedded
	// "#..." key would be shown raw by the TextWidget (only a whole-string key is auto-translated).
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

	static string DistanceToString(float distance)
	{
		if (distance < 0)
			return "";
		int meters = Math.Round(distance);
		return meters.ToString() + "m";
	}

	static string HitZoneGroupToLocalizationKey(int group)
	{
		switch (group)
		{
			case ECharacterHitZoneGroup.HEAD: return "#PS-KillList_Head";
			case ECharacterHitZoneGroup.UPPERTORSO: return "#PS-KillList_UpperTorso";
			case ECharacterHitZoneGroup.LOWERTORSO: return "#PS-KillList_LowerTorso";
			case ECharacterHitZoneGroup.LEFTARM: return "#PS-KillList_LeftArm";
			case ECharacterHitZoneGroup.RIGHTARM: return "#PS-KillList_RightArm";
			case ECharacterHitZoneGroup.LEFTLEG: return "#PS-KillList_LeftLeg";
			case ECharacterHitZoneGroup.RIGHTLEG: return "#PS-KillList_RightLeg";
		}
		return "";
	}

	string GetLocalizedHitZone()
	{
		string key = HitZoneGroupToLocalizationKey(m_eLastHitZoneGroup);
		if (key == "")
			return "";
		return WidgetManager.Translate(key);
	}

	string GetFormattedDistance()
	{
		if (m_fDistance < 0)
			return "—";
		int meters = Math.Round(m_fDistance);
		bool isRu = (WidgetManager.Translate("#PS-KillList_Head") == "ГОЛОВА" || WidgetManager.Translate("#PS-KillList_Head") == "Голова");
		if (isRu)
			return meters.ToString() + " м";
		return meters.ToString() + " m";
	}

	string GetFormattedTimestamp()
	{
		int totalSeconds = m_iMatchTimeSeconds;
		if (totalSeconds < 0)
			totalSeconds = 0;
		int minutes = totalSeconds / 60;
		int seconds = totalSeconds % 60;
		string sMinutes = minutes.ToString();
		if (minutes < 10)
			sMinutes = "0" + sMinutes;
		string sSeconds = seconds.ToString();
		if (seconds < 10)
			sSeconds = "0" + sSeconds;
		return sMinutes + ":" + sSeconds;
	}

	static string FactionKeyToDisplayName(string factionKey)
	{
		factionKey = Clean(factionKey);
		if (factionKey == "")
			return "—";

		FactionManager fMgr = GetGame().GetFactionManager();
		if (fMgr)
		{
			Faction faction = fMgr.GetFactionByKey(factionKey);
			if (faction)
			{
				string name = faction.GetFactionName();
				if (name != "")
					return WidgetManager.Translate(name);
			}
		}
		return "—";
	}

	string GetVictimFactionDisplayName()
	{
		return FactionKeyToDisplayName(m_sVictimFactionKey);
	}

	string GetKillerFactionDisplayName()
	{
		return FactionKeyToDisplayName(m_sKillerFactionKey);
	}

	string GetWeaponDisplayName()
	{
		string weapon = Clean(m_sWeaponName);
		if (weapon == "")
			return "—";
		if (weapon.StartsWith("#"))
			weapon = WidgetManager.Translate(weapon);
		return weapon;
	}

	string GetAmmoName()
	{
		string ammo = Clean(m_sAmmoType);
		if (ammo == "")
			return "";
		if (ammo.Contains("#"))
		{
			array<string> parts = {};
			ammo.Split("#", parts, false);
			if (parts.Count() > 0)
			{
				string name = parts[0];
				if (name.StartsWith("#"))
					name = WidgetManager.Translate(name);
				return name;
			}
		}
		if (ammo.StartsWith("#"))
			ammo = WidgetManager.Translate(ammo);
		return ammo;
	}

	int GetAmmoTypeFlags()
	{
		string ammo = Clean(m_sAmmoType);
		if (ammo.Contains("#"))
		{
			array<string> parts = {};
			ammo.Split("#", parts, false);
			if (parts.Count() > 1)
				return parts[1].ToInt();
		}
		return 0;
	}

	string GetAmmoTypeName()
	{
		string ammo = Clean(m_sAmmoType);
		if (ammo.Contains("#"))
		{
			array<string> parts = {};
			ammo.Split("#", parts, false);
			if (parts.Count() > 2)
				return parts[2];
		}
		return "";
	}

	string GetMagazineDisplayName()
	{
		return GetAmmoName();
	}

	string GetFormattedMagazine()
	{
		if (m_iMagazineCapacity <= 0)
			return "";
		bool isRu = (WidgetManager.Translate("#PS-KillList_Head") == "ГОЛОВА" || WidgetManager.Translate("#PS-KillList_Head") == "Голова");
		if (isRu)
			return m_iMagazineCapacity.ToString() + " птр";
		return m_iMagazineCapacity.ToString() + " rnds";
	}

	string GetFormattedCaliber()
	{
		string cal = Clean(m_sAmmoCaliber);
		if (cal.StartsWith("#"))
			cal = WidgetManager.Translate(cal);

		string ammoName = GetAmmoName();
		if (cal != "" && ammoName != "")
		{
			if (ammoName.Contains(cal))
				return ammoName;
			return cal + " (" + ammoName + ")";
		}
		if (cal != "")
			return cal;
		return ammoName;
	}

	// Empty fields are sent as "-" placeholders (so the pipe-split never drops a token); treat them as empty.
	static string Clean(string value)
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

		string ammo = GetAmmoName();
		if (ammo != "")
			result += "  [" + ammo + "]";

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
