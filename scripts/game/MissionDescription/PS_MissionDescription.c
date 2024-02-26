class PS_MissionDescriptionClass : GenericEntityClass
{

}

class PS_MissionDescription : GenericEntity
{
	[Attribute("")]
	string m_sTitle;
	[Attribute("")]
	ResourceName m_sDescriptionLayout;
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sTextData;
	
	[Attribute("")]
	ref array<FactionKey> m_aVisibleForFactions;
	
	[Attribute("")]
	bool m_bEmptyFactionVisibility;
	
	[Attribute("")]
	bool m_bShowForAnyFaction;
	
	[Attribute("")]
	int m_iOrder;
	
	int GetOrder()
	{
		return m_iOrder;
	}
	
	ResourceName GetDescriptionLayout()
	{
		return m_sDescriptionLayout;
	}
	
	bool GetVisibleForFaction(FactionKey factionKey)
	{
		if (m_bShowForAnyFaction) return true;
		return m_aVisibleForFactions.Contains(factionKey);
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
			{if (!GetVisibleForFaction(faction.GetFactionKey())) m_aVisibleForFactions.Insert(faction.GetFactionKey());}
		else
			{if (GetVisibleForFaction(faction.GetFactionKey())) m_aVisibleForFactions.RemoveItem(faction.GetFactionKey());}
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
		if (m_aVisibleForFactions == null)
			m_aVisibleForFactions = new array<FactionKey>();
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
	
	// JIP Replication
	override bool RplSave(ScriptBitWriter writer)
	{
		// Pack every changeable variable
		writer.WriteString(m_sTitle);
		writer.WriteString(m_sDescriptionLayout);
		writer.WriteString(m_sTextData);
		writer.WriteBool(m_bEmptyFactionVisibility);
		
		string factions = "";
		foreach (FactionKey factionKey: m_aVisibleForFactions)
		{
			if (factions != "") factions += ",";
			factions += factionKey;
		}
		writer.WriteString(factions);
		
		return true;
	}
	override bool RplLoad(ScriptBitReader reader)
	{
		// Unpack every changeable variable
		reader.ReadString(m_sTitle);
		reader.ReadString(m_sDescriptionLayout);
		reader.ReadString(m_sTextData);
		reader.ReadBool(m_bEmptyFactionVisibility);
		
		string factions;
		reader.ReadString(factions);
		GetGame().GetCallqueue().CallLater(FactionsInit, 0, false, factions);
		
		return true;
	}
	
	void FactionsInit(string factions)
	{
		array<string> outTokens = new array<string>();
		factions.Split(",", outTokens, false);
		foreach (FactionKey factionKey: outTokens)
		{
			m_aVisibleForFactions.Insert(factionKey);
		}
	}
}