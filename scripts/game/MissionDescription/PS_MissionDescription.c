class PS_MissionDescriptionClass : GenericEntityClass
{

}

class PS_MissionDescription : GenericEntity
{
	[Attribute("")]
	string m_sTitle;
	[Attribute("")]
	ResourceName m_sDescriptionLayout;
	[Attribute("")]
	string m_sTextData;
	
	// TODO: somehove edit it through workbench
	ref array<Faction> VisibleForFactions = new array<Faction>();
	
	bool m_bEmptyFactionVisibility = true;
	
	// TODO: Get/Set Broadcast
	ResourceName GetDescriptionLayout()
	{
		return m_sDescriptionLayout;
	}
	
	bool GetVisibleForFaction(Faction faction)
	{
		return VisibleForFactions.Contains(faction);
	}
	void SetVisibleForFaction(Faction faction, bool visible)
	{
		RPC_SetVisibleForFaction_ByKey(faction.GetFactionKey(), visible);
		Rpc(RPC_SetVisibleForFaction_ByKey, faction.GetFactionKey(), visible);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetVisibleForFaction_ByKey(FactionKey factionKey, bool visible)
	{
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (visible)
			{if (!GetVisibleForFaction(faction)) VisibleForFactions.Insert(faction);}
		else
			{if (GetVisibleForFaction(faction)) VisibleForFactions.RemoveItem(faction);}
	}
	
	bool GetVisibleForEmptyFaction()
	{
		return m_bEmptyFactionVisibility;
	}
	void SetVisibleForEmptyFaction(bool visible)
	{
		RPC_SetVisibleForEmptyFaction(visible);
		Rpc(RPC_SetVisibleForEmptyFaction, visible);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetVisibleForEmptyFaction(bool visible)
	{
		m_bEmptyFactionVisibility = visible;
	}
	
	string GetTitle()
	{
		return m_sTitle;
	}
	void SetTitle(string title)
	{
		RPC_SetTitle(title);
		Rpc(RPC_SetTitle, title);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetTitle(string title)
	{
		m_sTitle = title;
	}
	
	string GetTextData()
	{
		return m_sTextData;
	}
	void SetTextData(string textData)
	{
		RPC_SetTextData(textData);
		Rpc(RPC_SetTextData, textData);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetTextData(string textData)
	{
		m_sTextData = textData;
	}
	
	// Main functions
	override protected void EOnInit(IEntity owner)
	{
		// Frame delay for manager init
		GetGame().GetCallqueue().CallLater(RegisterToDescriptionManager);
	}
	void RegisterToDescriptionManager()
	{
		PS_MissionDescriptionManager missionDescriptionManager = PS_MissionDescriptionManager.GetInstance();
		if (!missionDescriptionManager)	
			return;
		missionDescriptionManager.RegisterDescription(this);
	}
	
	void PS_MissionDescription(IEntitySource src, IEntity parent)
	{
		// Enable init event
		SetEventMask(EntityEvent.INIT);
	}
	
	void ~PS_MissionDescription()
	{
		PS_MissionDescriptionManager missionDescriptionManager = PS_MissionDescriptionManager.GetInstance();
		if (!missionDescriptionManager)
			return;
		missionDescriptionManager.UnregisterDescription(this);
	}
	
	// TODO: JIP
}