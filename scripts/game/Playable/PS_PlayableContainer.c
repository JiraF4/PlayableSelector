// Playable replicatable container
class PS_PlayableContainer
{
	protected RplId m_iRplId;
	protected string m_sName;
	protected FactionKey m_sFactionKey;
	protected SCR_ECharacterRank m_iCharacterRank;
	protected string m_sRoleIconPath;
	protected string m_sRoleName;
	protected EDamageState m_iDamageState;
	
	void Init(PS_PlayableComponent playableComponent) // Rpc workaround
	{
		m_PlayableComponent = playableComponent;
		m_iRplId = playableComponent.GetRplId();
		m_sName = playableComponent.GetName();
		m_sFactionKey = playableComponent.GetFactionKey();
		m_iCharacterRank = playableComponent.GetCharacterRank();
		m_sRoleIconPath = playableComponent.GetRoleIconPath();
		m_sRoleName = playableComponent.GetRoleName();
		m_iDamageState = playableComponent.GetDamageState();
	}
	
	
	// -------------------------- Replication ----------------------------
	static bool Extract(PS_PlayableContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iCharacterRank);
		snapshot.SerializeString(instance.m_sRoleIconPath);
		snapshot.SerializeString(instance.m_sRoleName);
		snapshot.SerializeInt(instance.m_iDamageState);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, PS_PlayableContainer instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeInt(instance.m_iCharacterRank);
		snapshot.SerializeString(instance.m_sRoleIconPath);
		snapshot.SerializeString(instance.m_sRoleName);
		snapshot.SerializeInt(instance.m_iDamageState);
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

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
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
		return snapshot.CompareInt(instance.m_iRplId)
		    && snapshot.CompareString(instance.m_sName)
		    && snapshot.CompareString(instance.m_sFactionKey)
		    && snapshot.CompareInt(instance.m_iCharacterRank)
		    && snapshot.CompareString(instance.m_sRoleIconPath)
		    && snapshot.CompareString(instance.m_sRoleName)
		    && snapshot.CompareInt(instance.m_iDamageState);
	}
	
	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iRplId);
		writer.WriteString(m_sName);
		writer.WriteString(m_sFactionKey);
		writer.WriteInt(m_iCharacterRank);
		writer.WriteString(m_sRoleIconPath);
		writer.WriteString(m_sRoleName);
		writer.WriteInt(m_iDamageState);
	}
	
	void Load(ScriptBitReader reader)
	{
		reader.ReadInt(m_iRplId);
		reader.ReadString(m_sName);
		reader.ReadString(m_sFactionKey);
		reader.ReadInt(m_iCharacterRank);
		reader.ReadString(m_sRoleIconPath);
		reader.ReadString(m_sRoleName);
		reader.ReadInt(m_iDamageState);
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
	protected ref ScriptInvokerInt m_eOnPlayerChange = new ScriptInvokerInt(); // int playerId
	ScriptInvokerInt GetOnPlayerChange()
		return m_eOnPlayerChange;
	void InvokeOnPlayerChanged(int playerId)
		m_eOnPlayerChange.Invoke(playerId);
	
	ref ScriptInvokerInt m_eOnDamageStateChanged = new ScriptInvokerInt();
	ScriptInvokerInt GetOnDamageStateChanged()
		return m_eOnDamageStateChanged;
	ref ScriptInvokerVoid m_eOnUnregister = new ScriptInvokerVoid();
	ScriptInvokerVoid GetOnUnregister()
		return m_eOnUnregister;
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_eOnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPlayerDisconnected()
		return m_eOnPlayerDisconnected;
	protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_eOnPlayerConnected = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
	ScriptInvokerBase<SCR_BaseGameMode_PlayerId> GetOnPlayerConnected()
		return m_eOnPlayerConnected;
	protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_eOnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
	ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> GetOnPlayerRoleChange()
		return m_eOnPlayerRoleChange;
	protected ref ScriptInvokerInt m_eOnPlayerStateChange = new ScriptInvokerInt();
	ScriptInvokerInt GetOnPlayerStateChange()
		return m_eOnPlayerStateChange;
	protected ref ScriptInvokerBool m_eOnPlayerPinChange = new ScriptInvokerBool();
	ScriptInvokerBool GetOnPlayerPinChange()
		return m_eOnPlayerPinChange;
	
	void OnDamageStateChanged(EDamageState damageState)
	{
		m_iDamageState = damageState;
		m_eOnDamageStateChanged.Invoke(damageState);
	}

	// -------------------------- Get client ----------------------------
	RplId GetRplId()
	{
		return m_iRplId;
	}
	string GetName()
	{
		return m_sName;
	}
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
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
		return m_iCharacterRank;
	}
	EDamageState GetDamageState()
	{
		return m_iDamageState;
	}
}
