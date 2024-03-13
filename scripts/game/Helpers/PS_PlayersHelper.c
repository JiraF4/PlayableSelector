class PS_PlayersHelper
{
	static bool IsAdminOrServer()
	{
		if (Replication.IsServer())
			return true;
		return SCR_Global.IsAdmin();
	}
}