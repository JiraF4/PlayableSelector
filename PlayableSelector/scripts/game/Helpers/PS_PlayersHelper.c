class PS_PlayersHelper
{
	static bool IsAdminOrServer()
	{
		if (Replication.IsServer())
			return true;
		return SCR_Global.IsAdmin();
	}

	//! @issue BUG-85
	//! @cause Безусловный string.ToInt() на сыром аргументе чата бросал VME "Wrong parameter value" на пустом/нечисловом вводе.
	//! @solution Валидируем строку (Trim + только цифры с опциональным ведущим '-') и лишь затем зовём ToInt(); иначе возвращаем fallback.
	//! @note Аргумент чат-команды — необработанный хвост строки чата (SCR_ChatPanelManager.Internal_OnChatCommand), доверять ему нельзя.
	static int ParseChatIntOr(string data, int fallback)
	{
		string arg = data;
		arg.Trim();
		if (arg.IsEmpty())
			return fallback;

		// Разрешаем ведущий '-' и только цифры далее.
		int start = 0;
		if (arg[0] == "-")
		{
			if (arg.Length() == 1)
				return fallback;
			start = 1;
		}
		for (int i = start; i < arg.Length(); i++)
		{
			string ch = arg[i];
			if (ch < "0" || ch > "9")
				return fallback;
		}
		return arg.ToInt();
	}
}
