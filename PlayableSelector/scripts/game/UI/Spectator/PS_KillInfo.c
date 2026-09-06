class PS_KillInfo
{
	int m_iVictimPlayerId;
	int m_iKillerPlayerId;
	string m_sVictimName;
	string m_sKillerName;
	string m_sVictimSquad;   // victim's group/squad name (resolved server-side at kill time)
	string m_sAmmoType;
	float m_fDistance;
	int m_eLastHitZoneGroup;
	bool m_bIsTeamKill;

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
