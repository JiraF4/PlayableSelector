// PS_NetStat - lightweight network-traffic instrumentation to help pinpoint Replication Flooded/Stalled
// kicks at scale. Counts our RPC handler invocations by name and dumps per-window totals to the console
// log every ~5s of activity. WHY console log (not the Diag overlay): this lands in console.log, so it
// works on the LIVE Podval server where the dev-only network Diag overlay isn't available - that is where
// the flood actually happens (3-player local tests can't reproduce it).
//
// Usage: add  PS_NetStat.Hit("RPC_Name");  at the top of any RPC handler you want to watch. The dump only
// lists RPCs that actually fired in the window, so instrumenting more handlers is cheap and self-filtering.
// The [SERVER]/[CLIENT] tag tells you which side logged it - for a Broadcast RPC sent via the usual
// "RPC_X(...); Rpc(RPC_X, ...)" pattern the SERVER line counts how often it was BROADCAST (the load
// source), while CLIENT lines count receives. Set s_bEnabled = false to silence everything.
class PS_NetStat
{
	static bool s_bEnabled = true;

	protected static ref map<string, int> s_mCounts = new map<string, int>();
	protected static bool s_bScheduled;
	protected static float s_fWindowStart;

	static void Hit(string name)
	{
		if (!s_bEnabled)
			return;

		int c = 0;
		s_mCounts.Find(name, c);
		s_mCounts.Set(name, c + 1);

		if (!s_bScheduled)
		{
			s_bScheduled = true;
			s_fWindowStart = GetGame().GetWorld().GetWorldTime();
			GetGame().GetCallqueue().CallLater(Dump, 5000, false);
		}
	}

	protected static void Dump()
	{
		s_bScheduled = false;
		if (s_mCounts.IsEmpty())
			return;

		float secs = (GetGame().GetWorld().GetWorldTime() - s_fWindowStart) / 1000.0;
		string role = "CLIENT";
		if (Replication.IsServer())
			role = "SERVER";

		string line = string.Format("[PS_NetStat][%1] RPC handler calls over %2s:", role, secs);
		for (int i = 0; i < s_mCounts.Count(); i++)
			line += string.Format(" %1=%2", s_mCounts.GetKey(i), s_mCounts.GetElement(i));

		Print(line, LogLevel.NORMAL);
		s_mCounts.Clear();
	}
}
