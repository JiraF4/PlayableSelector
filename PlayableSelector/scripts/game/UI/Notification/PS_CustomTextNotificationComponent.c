[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_CustomTextNotificationComponentClass : ScriptComponentClass
{
}

// This solution suck, but i don't want to mod half of the notifiaction system to only add custome ones
// At least right now...
class PS_CustomTextNotificationComponent : ScriptComponent
{
	protected static PS_CustomTextNotificationComponent s_Instance;
	
	protected ref map<int, string> m_mTextData = new map<int, string>();
	
	// --------------------------------------------------------------------------------------------
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_CustomTextNotificationComponent GetInstance()
	{
		return s_Instance;
	}
	
	// --------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		s_Instance = this;
	}
	
	// --------------------------------------------------------------------------------------------
	static void SendTextData(int reporterId, string data)
	{
		s_Instance.SendTextDataInstance(reporterId, data);
	}
	
	// --------------------------------------------------------------------------------------------
	static string GetTextData(int reporterId)
	{
		return s_Instance.m_mTextData[reporterId];
	}
	
	// --------------------------------------------------------------------------------------------
	void SendTextDataInstance(int reporterId, string data)
	{
		Rpc(RPC_SendTextData, data, reporterId);
		RPC_SendTextData(data, reporterId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SendTextData(string data, int id)
	{
		m_mTextData[id] = data;
	}
}
