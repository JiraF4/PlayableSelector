class PS_PlayableVehicleContainer
{
	RplId m_iRplId;
	ResourceName m_sPrefabName;
	int m_iGroupCallsign;
	int m_iGroupId;
	FactionKey m_sFactionKey;
	
	void Init(int rplId, ResourceName prefabName, int groupCallsign, int groupId, FactionKey factionKey)
	{
		m_iRplId = rplId;
		m_sPrefabName = prefabName;
		m_iGroupCallsign = groupCallsign;
		m_iGroupId = groupId;
		m_sFactionKey = factionKey;
	}
	
	// -------------------------- Replication ----------------------------
	static bool Extract(PS_PlayableVehicleContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sPrefabName);
		snapshot.SerializeInt(instance.m_iGroupCallsign);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeString(instance.m_sFactionKey);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayableVehicleContainer instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sPrefabName);
		snapshot.SerializeInt(instance.m_iGroupCallsign);
		snapshot.SerializeInt(instance.m_iGroupId);
		snapshot.SerializeString(instance.m_sFactionKey);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.EncodeString(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareStringSnapshots(rhs)
		    && lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareSnapshots(rhs, 4)
		    && lhs.CompareStringSnapshots(rhs);
	}

	static bool PropCompare(PS_PlayableVehicleContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iRplId)
		    && snapshot.CompareString(instance.m_sPrefabName)
		    && snapshot.CompareInt(instance.m_iGroupCallsign)
		    && snapshot.CompareInt(instance.m_iGroupId)
		    && snapshot.CompareString(instance.m_sFactionKey);
	}
	
	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iRplId);
		writer.WriteString(m_sPrefabName);
		writer.WriteInt(m_iGroupCallsign);
		writer.WriteInt(m_iGroupId);
		writer.WriteString(m_sFactionKey);
	}
	
	void Load(ScriptBitReader reader)
	{
		reader.ReadInt(m_iRplId);
		reader.ReadString(m_sPrefabName);
		reader.ReadInt(m_iGroupCallsign);
		reader.ReadInt(m_iGroupId);
		reader.ReadString(m_sFactionKey);
	}
	
	// -------------------------- Get client ----------------------------
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}
	SCR_Faction GetFaction()
	{
		return SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(GetFactionKey()));
	}
}