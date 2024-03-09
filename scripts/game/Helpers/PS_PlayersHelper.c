class PS_PlayersHelper
{
	bool IsAdminOrLocal()
	{
		if (Replication.IsServer())
			return true;
		return SCR_Global.IsAdmin();
	}
}