[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_PlayableComponentClass : ScriptComponentClass
{
}


class PS_PlayableComponent : ScriptComponent
{
	[Attribute()]
	protected string m_sName;
	[Attribute()]
	protected bool m_bIsPlayable;
	[Attribute()]
	ref array<ResourceName> m_aRespawnPrefabs;

	// Actually just RplId from RplComponent
	protected RplId m_RplId;
	protected vector spawnTransform[4];
	[RplProp()]
	int m_iRespawnCounter = 0;
	protected bool m_bRespawned;

	// Server only
	protected ref PS_PlayableContainer m_PlayableContainer;
	PS_PlayableContainer GetPlayableContainer()
	{
		return m_PlayableContainer;
	}

	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;

	// Cache components
	protected SCR_ChimeraCharacter m_Owner;
	SCR_ChimeraCharacter GetOwnerCharacter()
	{
		return m_Owner;
	}
	protected FactionAffiliationComponent m_FactionAffiliationComponent;
	FactionAffiliationComponent GetFactionAffiliationComponent()
	{
		return m_FactionAffiliationComponent;
	}
	protected SCR_EditableCharacterComponent m_EditableCharacterComponent;
	SCR_EditableCharacterComponent GetEditableCharacterComponent()
	{
		return m_EditableCharacterComponent;
	}
	protected SCR_UIInfo m_EditableUIInfo;
	SCR_UIInfo GetEditableUIInfo()
	{
		return m_EditableUIInfo;
	}
	protected SCR_CharacterDamageManagerComponent m_CharacterDamageManagerComponent;
	SCR_CharacterDamageManagerComponent GetCharacterDamageManagerComponent()
	{
		return m_CharacterDamageManagerComponent;
	}
	protected AIControlComponent m_AIControlComponent;
	AIControlComponent GetAIControlComponent()
	{
		return m_AIControlComponent;
	}
	protected AIAgent m_AIAgent;
	AIAgent GetAIAgent()
	{
		return m_AIAgent;
	}

	// Temporally
	static protected int s_iRespawnTime;
	
	void CopyState(PS_RespawnData respawnData)
	{
		if (respawnData.m_PlayableComponent)
			respawnData.m_PlayableComponent.SetPlayable(false);
		m_iRespawnCounter = respawnData.m_iRespawnCounter;
		m_aRespawnPrefabs = respawnData.m_aRespawnPrefabs;
		Math3D.MatrixCopy(respawnData.m_aSpawnTransform, spawnTransform);
		Replication.BumpMe();
	}

	override void OnPostInit(IEntity owner)
	{
		m_Owner = SCR_ChimeraCharacter.Cast(owner);
		m_Owner.PS_SetPlayable(this);

		if (Replication.IsServer())
			owner.GetTransform(spawnTransform);

		SetEventMask(owner, EntityEvent.INIT);
	}

	void GetSpawnTransform(inout vector outMat[4])
	{
		Math3D.MatrixCopy(spawnTransform, outMat);
	}

	ResourceName GetNextRespawn(bool nextPrefab)
	{
		if (!nextPrefab)
		{
			return m_Owner.GetPrefabData().GetPrefabName();
		}
		ResourceName prefab = "";
		if (!m_aRespawnPrefabs)
			return "";
		if (m_aRespawnPrefabs.Count() > m_iRespawnCounter)
			prefab = m_aRespawnPrefabs[m_iRespawnCounter];
		m_iRespawnCounter++;
		Replication.BumpMe();
		return prefab;
	}

	override void EOnInit(IEntity owner)
	{
		m_FactionAffiliationComponent = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_EditableCharacterComponent = SCR_EditableCharacterComponent.Cast(owner.FindComponent(SCR_EditableCharacterComponent));
		m_EditableUIInfo = m_EditableCharacterComponent.GetInfo();
		m_CharacterDamageManagerComponent = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_AIControlComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		m_AIAgent = m_AIControlComponent.GetAIAgent();
		GetGame().GetCallqueue().Call(LateInit);
	}

	void LateInit()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		m_RplId = rpl.Id();
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();

		m_PlayableContainer = new PS_PlayableContainer();
		m_PlayableContainer.Init(this);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(AddToList, 0, false, GetOwner()); // init delay
	}

	// Get/Set Broadcast
	bool GetPlayable()
	{
		return m_bIsPlayable;
	}
	void SetPlayable(bool isPlayable)
	{
		RPC_SetPlayable(isPlayable);
		Rpc(RPC_SetPlayable, isPlayable);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetPlayable(bool isPlayable)
	{
		m_bIsPlayable = isPlayable;
		if (m_bIsPlayable)
			GetGame().GetCallqueue().CallLater(AddToList, 0, false, m_Owner);
		else RemoveFromList();
	}

	void OpenRespawnMenu(int time)
	{
		Rpc(RPC_OpenRespawnMenu, time);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_OpenRespawnMenu(int time)
	{
		s_iRespawnTime = time;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.PlayableRespawnMenu);
	}
	int GetRespawnTime()
	{
		return s_iRespawnTime;
	}

	void ResetRplStream()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		rpl.EnableStreaming(true);
	}

	private void RemoveFromList()
	{
		GetGame().GetCallqueue().Remove(AddToList);
		GetGame().GetCallqueue().Remove(AddToListWrap);
		GetGame().GetCallqueue().Remove(ForceDeactivateAI);

		BaseGameMode gamemode = GetGame().GetGameMode();
		if (!gamemode)
			return;

		if (m_Owner)
		{
			AIControlComponent aiComponent = AIControlComponent.Cast(m_Owner.FindComponent(AIControlComponent));
			if (aiComponent)
			{
				AIAgent agent = aiComponent.GetAIAgent();
				if (agent)
				{
					agent.ActivateAI();
				}
			}

			RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
			rpl.EnableStreaming(true);
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		playableManager.UnRegisterPlayable(GetRplId());
	}

	private void OnDamageStateChange(EDamageState state)
	{
		if (m_PlayableManager && !m_bRespawned && state == EDamageState.DESTROYED)
		{
			GetGame().GetCallqueue().CallLater(TryRespawn, 200, false, m_PlayableManager.GetPlayerByPlayable(m_RplId));
			m_bRespawned = true;
		}
		PS_PlayableManager.GetInstance().OnPlayableDamageStateChanged(m_RplId, state);
	}

	private void TryRespawn(int playerId)
	{
		if (m_GameModeCoop)
			m_GameModeCoop.TryRespawn(m_RplId, playerId);
	}

	private void AddToList(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(ForceActivateAI);
		if (GetOwner().GetWorld() != GetGame().GetWorld())
			return;
		if (Replication.IsServer())
			m_CharacterDamageManagerComponent.GetOnDamageStateChanged().Insert(OnDamageStateChange);

		if (!m_bIsPlayable)
			return;
		
		GetGame().GetCallqueue().CallLater(AddToListWrap, 0, false, owner) // init delay
	}

	private void AddToListWrap(IEntity owner)
	{
		if (!m_bIsPlayable)
			return;

		// Why it's activating randomly?
		// Retarded shit.
		if (GetGame().GetAIWorld().CanAIBeActivated())
			GetGame().GetCallqueue().CallLater(ForceDeactivateAI, 500, true);

		if (PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetDisablePlayablesStreaming())
		{
			RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
			rpl.EnableStreaming(false);
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;

		playableManager.RegisterPlayable(this);
	}

	void ForceActivateAI()
	{
		if (!m_AIAgent)
		{
			GetGame().GetCallqueue().Remove(ForceActivateAI);
			return;
		}
		if (!m_AIAgent.IsAIActivated())
			m_AIAgent.ActivateAI();
		else
			GetGame().GetCallqueue().Remove(ForceActivateAI);
	}
	void ForceDeactivateAI()
	{
		if (!m_AIAgent)
		{
			GetGame().GetCallqueue().Remove(ForceDeactivateAI);
			return;
		}
		if (m_AIAgent.IsAIActivated())
			m_AIAgent.DeactivateAI();
	}

	void HolsterWeapon()
	{
		m_Owner.GetCharacterController().TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual);
	}

	string GetName()
	{
		if (m_sName != "")
			return m_sName;
		return m_EditableUIInfo.GetName();
	}

	[Obsolete("Use GetRplId() instead")]
	RplId GetId()
	{
		return GetRplId();
	}
	RplId GetRplId()
	{
		return m_RplId;
	}

	FactionKey GetFactionKey()
	{
		return GetFactionAffiliationComponent().GetDefaultFactionKey();
	}
	string GetRoleIconPath()
	{
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(GetOwner().FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		return uiInfo.GetIconPath();
	}
	string GetRoleName()
	{
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(GetOwner().FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		return uiInfo.GetName();
	}
	SCR_ECharacterRank GetCharacterRank()
	{
		return SCR_CharacterRankComponent.GetCharacterRank(GetOwner());
	}
	EDamageState GetDamageState()
	{
		return m_CharacterDamageManagerComponent.GetState();
	}

	void PS_PlayableComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{

	}

	void ~PS_PlayableComponent()
	{
		if (Replication.IsServer())
			RemoveFromList();
	}

	// Send our precision data, we need it on clients
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteString(m_sName);
		writer.WriteBool(m_bIsPlayable);
		return true;
	}
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadString(m_sName);
		reader.ReadBool(m_bIsPlayable);
		return true;
	}
	
	SCR_ChimeraCharacter GetCharacter()
	{
		return SCR_ChimeraCharacter.Cast(GetOwner());
	}
}

class PS_RespawnData
{
	PS_PlayableComponent m_PlayableComponent;

	RplId m_Id;
	ResourceName m_sPrefabName;
	vector m_aSpawnTransform[4];
	int m_iRespawnCounter;
	ref array<ResourceName> m_aRespawnPrefabs;

	void PS_RespawnData(PS_PlayableComponent playableComponent, ResourceName prefabName)
	{
		m_PlayableComponent = playableComponent;

		m_sPrefabName = prefabName;
		m_Id = playableComponent.GetRplId();
		playableComponent.GetSpawnTransform(m_aSpawnTransform);
		m_iRespawnCounter = playableComponent.m_iRespawnCounter;
		m_aRespawnPrefabs = playableComponent.m_aRespawnPrefabs;
	}
}
