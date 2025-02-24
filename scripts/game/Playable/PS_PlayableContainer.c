// Playable replicatable container
class PS_PlayableContainer
{
	protected RplId m_RplId;
	protected string m_sName;
	protected FactionKey m_FactionKey;
	protected SCR_ECharacterRank m_eCharacterRank;
	protected string m_sRoleIconPath;
	protected string m_sRoleName;
	protected EDamageState m_eDamageState;

	void Init(PS_PlayableComponent playableComponent) // Rpc workaround
	{
		m_PlayableComponent = playableComponent;
		m_RplId = playableComponent.GetRplId();
		m_sName = playableComponent.GetName();
		m_FactionKey = playableComponent.GetFactionKey();
		m_eCharacterRank = playableComponent.GetCharacterRank();
		m_sRoleIconPath = playableComponent.GetRoleIconPath();
		m_sRoleName = playableComponent.GetRoleName();
		m_eDamageState = playableComponent.GetDamageState();
	}

	// -------------------------- Replication ----------------------------
	static bool Extract(PS_PlayableContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_RplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_FactionKey);
		snapshot.SerializeInt(instance.m_eCharacterRank);
		snapshot.SerializeString(instance.m_sRoleIconPath);
		snapshot.SerializeString(instance.m_sRoleName);
		snapshot.SerializeInt(instance.m_eDamageState);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayableContainer instance)
	{
		snapshot.SerializeInt(instance.m_RplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_FactionKey);
		snapshot.SerializeInt(instance.m_eCharacterRank);
		snapshot.SerializeString(instance.m_sRoleIconPath);
		snapshot.SerializeString(instance.m_sRoleName);
		snapshot.SerializeInt(instance.m_eDamageState);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(PS_PlayableContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_RplId)
			&& snapshot.CompareString(instance.m_sName)
			&& snapshot.CompareString(instance.m_FactionKey)
			&& snapshot.CompareInt(instance.m_eCharacterRank)
			&& snapshot.CompareString(instance.m_sRoleIconPath)
			&& snapshot.CompareString(instance.m_sRoleName)
			&& snapshot.CompareInt(instance.m_eDamageState);
	}

	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_RplId);
		writer.WriteString(m_sName);
		writer.WriteString(m_FactionKey);
		writer.WriteInt(m_eCharacterRank);
		writer.WriteString(m_sRoleIconPath);
		writer.WriteString(m_sRoleName);
		writer.WriteInt(m_eDamageState);
	}

	void Load(ScriptBitReader reader)
	{
		reader.ReadInt(m_RplId);
		reader.ReadString(m_sName);
		reader.ReadString(m_FactionKey);
		reader.ReadInt(m_eCharacterRank);
		reader.ReadString(m_sRoleIconPath);
		reader.ReadString(m_sRoleName);
		reader.ReadInt(m_eDamageState);
	}

	// -------------------------- Get server ----------------------------
	protected PS_PlayableComponent m_PlayableComponent;
	PS_PlayableComponent GetPlayableComponent()
	{
		return m_PlayableComponent;
	}
	PS_PlayableComponent GetPlayableComponentTemp()
	{
		return m_PlayableComponent;
	}

	// ---------------------------- Events ------------------------------
	protected ref ScriptInvokerInt2 m_eOnPlayerChange = new ScriptInvokerInt2(); // int playerId
	ScriptInvokerInt2 GetOnPlayerChange()
	{
		return m_eOnPlayerChange;
	}
	void InvokeOnPlayerChanged(int oldPlayerId, int playerId)
	{
		m_eOnPlayerChange.Invoke(oldPlayerId, playerId);
	}

	ref ScriptInvokerInt m_eOnDamageStateChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnDamageStateChanged()
	{
		return m_eOnDamageStateChanged;
	}
	ref ScriptInvokerVoid m_eOnUnregister = new ScriptInvokerVoid();
	ScriptInvokerVoid GetOnUnregister()
	{
		return m_eOnUnregister;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
	{
		return m_eOnPlayerDisconnected;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_eOnPlayerConnected = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
	ScriptInvokerBase<SCR_BaseGameMode_PlayerId> GetOnPlayerConnected()
	{
		return m_eOnPlayerConnected;
	}
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_eOnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> GetOnPlayerRoleChange()
	{
		return m_eOnPlayerRoleChange;
	}
	protected ref ScriptInvokerInt m_eOnPlayerStateChange = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerStateChange()
	{
		return m_eOnPlayerStateChange;
	}
	protected ref ScriptInvokerBool m_eOnPlayerPinChange = new ScriptInvokerBool();
	ScriptInvokerBool GetOnPlayerPinChange()
	{
		return m_eOnPlayerPinChange;
	}

	void OnDamageStateChanged(EDamageState damageState)
	{
		m_eDamageState = damageState;
		m_eOnDamageStateChanged.Invoke(damageState);
	}

	// -------------------------- Get client ----------------------------
	RplId GetRplId()
	{
		return m_RplId;
	}
	string GetName()
	{
		return m_sName;
	}
	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}
	SCR_Faction GetFaction()
	{
		return SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(GetFactionKey()));
	}
	string GetRoleIconPath()
	{
		return m_sRoleIconPath;
	}
	string GetRoleName()
	{
		return m_sRoleName;
	}
	SCR_ECharacterRank GetCharacterRank()
	{
		return m_eCharacterRank;
	}
	EDamageState GetDamageState()
	{
		return m_eDamageState;
	}
}
