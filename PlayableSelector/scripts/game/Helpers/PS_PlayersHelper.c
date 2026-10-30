class PS_PlayersHelper
{
	static bool IsAdminOrServer()
	{
		if (Replication.IsServer())
			return true;
		return SCR_Global.IsAdmin();
	}

	// Strip rich-text markup so names read as PLAIN text. The Podval mods (PodvalLobby) rewrite player
	// names to embed clan-tag markup, e.g. "<b><color rgba='226, 168, 79, 200'>[kak]</color></b>Vector";
	// one rule colors, sorts and labels names across the addon.
	static string StripMarkup(string value)
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

	// "[" (ASCII 91) sorts after upper-case and before lower-case letters, so a plain Compare would not
	// put tagged names on top by itself. Runs on a markup-stripped name, where a PodvalLobby-injected
	// tag and a typed-in clan tag are equivalent.
	static bool HasUnitTag(string name)
	{
		return name.IndexOf("[") == 0 && name.IndexOf("]") >= 2;
	}
}
