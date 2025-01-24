class PS_PlayableVehicleContainer
{
	RplId m_iRplId;
	ResourceName m_sPrefabName;
	ResourceName m_sIconPath;
	int m_iGroupCallsign;
	int m_iGroupId;
	FactionKey m_sFactionKey;
	int m_iLocked;
	
	protected ref ScriptInvokerBool m_eOnLockChange = new ScriptInvokerBool();
	ScriptInvokerBool GetOnLockChange()
		return m_eOnLockChange;
	
	void Init(int rplId, ResourceName prefabName, ResourceName iconPath, int groupCallsign, int groupId, FactionKey factionKey)
	{
		m_iRplId = rplId;
		m_sPrefabName = prefabName;
		m_sIconPath = iconPath;
		m_iGroupCallsign = groupCallsign;
		m_iGroupId = groupId;
		m_sFactionKey = factionKey;
		m_iLocked = 0;
	}
	
	// -------------------------- Replication ----------------------------
	static bool Extract(PS_PlayableVehicleContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sPrefabName);
		snapshot.SerializeString(instance.m_sIconPath);
		snapshot.SerializeInt(instance.m_iGroupCallsign);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iLocked);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayableVehicleContainer instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sPrefabName);
		snapshot.SerializeString(instance.m_sIconPath);
		snapshot.SerializeInt(instance.m_iGroupCallsign);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iLocked);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareStringSnapshots(rhs)
		    && lhs.CompareStringSnapshots(rhs)
		    && lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareStringSnapshots(rhs)
		    && lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(PS_PlayableVehicleContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iRplId)
		    && snapshot.CompareString(instance.m_sPrefabName)
		    && snapshot.CompareString(instance.m_sIconPath)
		    && snapshot.CompareInt(instance.m_iGroupCallsign)
		    && snapshot.CompareInt(instance.m_iGroupId)
		    && snapshot.CompareString(instance.m_sFactionKey)
		    && snapshot.CompareInt(instance.m_iLocked);
	}
	
	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iRplId);
		writer.WriteString(m_sPrefabName);
		writer.WriteString(m_sIconPath);
		writer.WriteInt(m_iGroupCallsign);
		writer.WriteInt(m_iGroupId);
		writer.WriteString(m_sFactionKey);
		writer.WriteInt(m_iLocked);
	}
	
	void Load(ScriptBitReader reader)
	{
		reader.ReadInt(m_iRplId);
		reader.ReadString(m_sPrefabName);
		reader.ReadString(m_sIconPath);
		reader.ReadInt(m_iGroupCallsign);
		reader.ReadInt(m_iGroupId);
		reader.ReadString(m_sFactionKey);
		reader.ReadInt(m_iLocked);
	}
	
	void SetLock(bool lock)
	{
		m_iLocked = lock;
		m_eOnLockChange.Invoke(lock);
	}
	
	// -------------------------- Get client ----------------------------
	RplId GetRplId()
	{
		return m_iRplId;
	}
	int GetLock()
	{
		return m_iLocked;
	}
	ResourceName GetIconPath()
	{
		return m_sIconPath;
	}
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}
	SCR_Faction GetFaction()
	{
		return SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(GetFactionKey()));
	}
}