class PS_KillInfo
{
	static const string AMMO_DELIMITER = "~";

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

	static void ParseAmmoPayload(string payload, out string ammoName, out int flags, out string typeName)
	{
		ammoName = "";
		flags = 0;
		typeName = "";

		payload = Clean(payload);
		if (payload == "")
			return;

		if (payload.Contains(AMMO_DELIMITER))
		{
			array<string> parts = {};
			payload.Split(AMMO_DELIMITER, parts, false);
			if (parts.Count() > 0)
				ammoName = parts[0];
			if (parts.Count() > 1)
				flags = parts[1].ToInt();
			if (parts.Count() > 2)
				typeName = parts[2];
			return;
		}

		// Backward-compatibility fallback for legacy '#' separator
		if (payload.Contains("#"))
		{
			array<string> legacyParts = {};
			payload.Split("#", legacyParts, true);
			int count = legacyParts.Count();
			if (count == 1)
			{
				ammoName = "#" + legacyParts[0];
			}
			else if (count >= 2)
			{
				if (count >= 3 && legacyParts[count - 2].ToInt() > 0)
				{
					typeName = legacyParts[count - 1];
					flags = legacyParts[count - 2].ToInt();
					ammoName = "#" + legacyParts[0];
				}
				else if (legacyParts[count - 1].ToInt() > 0)
				{
					flags = legacyParts[count - 1].ToInt();
					ammoName = "#" + legacyParts[0];
				}
				else
				{
					ammoName = payload;
				}
			}
			return;
		}

		ammoName = payload;
	}

	static bool IsTechnicalStringId(string val)
	{
		if (val == "")
			return false;
		if (val.StartsWith("AR-") || val.StartsWith("#AR-") || val.StartsWith("RHS-") || val.StartsWith("#RHS-"))
			return true;
		if (val.Contains("AmmoType_") || val.Contains("Magazine_") || val.Contains("Ammo_"))
			return true;
		if (val.Contains("/") || val.Contains("\\") || val.Contains(".conf") || val.Contains(".et"))
			return true;
		return false;
	}

	string GetAmmoName()
	{
		string rawName;
		int flags;
		string typeName;
		ParseAmmoPayload(m_sAmmoType, rawName, flags, typeName);

		rawName = Clean(rawName);
		if (rawName == "")
			return "";

		string translated = rawName;
		if (translated.StartsWith("#"))
		{
			translated = WidgetManager.Translate(translated);
		}
		else if (translated.StartsWith("AR-") || translated.StartsWith("RHS-"))
		{
			string withHash = WidgetManager.Translate("#" + translated);
			if (withHash != "#" + translated)
				translated = withHash;
		}

		if (IsTechnicalStringId(translated))
			return "";

		return translated;
	}

	int GetAmmoTypeFlags()
	{
		string rawName;
		int flags;
		string typeName;
		ParseAmmoPayload(m_sAmmoType, rawName, flags, typeName);
		return flags;
	}

	string GetAmmoTypeName()
	{
		string rawName;
		int flags;
		string typeName;
		ParseAmmoPayload(m_sAmmoType, rawName, flags, typeName);

		if (typeName != "" && typeName != "-")
		{
			if (!typeName.ToInt() || typeName == "0")
				return typeName;
			return "";
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

		if (IsTechnicalStringId(cal))
			cal = "";

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

	// Victim label = "Squad Nickname" (squad omitted when unknown). Falls back to a live name lookup, then "AI".
	string GetVictimDisplayName()
	{
		string name = Clean(m_sVictimName);
		if (name == "" && m_iVictimPlayerId > 0)
			name = PS_PlayableManager.GetInstance().GetPlayerName(m_iVictimPlayerId);
		if (name == "")
			name = "AI";
		name = PS_PlayersHelper.StripMarkup(name);

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
		return PS_PlayersHelper.StripMarkup(name);
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
