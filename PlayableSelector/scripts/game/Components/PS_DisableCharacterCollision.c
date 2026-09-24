[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_DisableCharacterCollisionComponentClass : ScriptComponentClass
{
}

/**
 * @brief Удаление временного лобби-персонажа (Character_Administrator) после занятия слота.
 * @subsystem Core | Lobby
 * @context Hybrid
 * @entity Character_Administrator.et
 * @depends PlayableSelector
 * @listens EOnInit, EOnFrame
 * @details После переключения игрока на игрового персонажа клиент запрашивает удаление
 *          «осиротевшего» временного тела через RPC_DeleteMe с защитой от флуда и серверной валидацией.
 */
class PS_DisableCharacterCollisionComponent : ScriptComponent
{
	RplComponent m_RplComponent;

	//! RPC-запрос на удаление уже отправлен
	protected bool m_bDeleteRequested;
	protected int m_iDeleteRetryCount = 0;
	protected static const int MAX_DELETE_RETRIES = 5;
	protected static const int DELETE_RETRY_INTERVAL_MS = 2000;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		//if (!Replication.IsServer() && (!m_RplComponent || !m_RplComponent.IsOwner()))
		//	physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
		GetGame().GetCallqueue().CallLater(Recolor, 1000, false, owner);
	}
	
	void Recolor(IEntity owner)
	{
		if (!m_RplComponent || !m_RplComponent.IsOwner())
			return;
		
		VObject obj = owner.GetVObject();		
		string materials[256];
		int numMats = obj.GetMaterials(materials);
		string remap = "";
		for (int i = 0; i < numMats; i++)
		{
			remap += "$remap '" + materials[i] + "' '{788C42E96DB8587C}Assets/Editor/VirtualArea/VirtualArea_01_Danger.emat';";
		}
		//owner.SetObject(obj, remap);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_RplComponent || !m_RplComponent.IsOwner())
			return;
		
		if (m_bDeleteRequested)
			return;

		if (SCR_PlayerController.GetLocalControlledEntity() == owner)
			return;

		m_bDeleteRequested = true;
		ClearEventMask(owner, EntityEvent.FRAME);
		Rpc(RPC_DeleteMe);

		// Повтор раз в 2 с с лимитом попыток: сервер мог отклонить удаление при гонке смены управления
		GetGame().GetCallqueue().CallLater(RetryDelete, DELETE_RETRY_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	void RetryDelete()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			CancelRetry();
			return;
		}

		if (SCR_PlayerController.GetLocalControlledEntity() == owner)
		{
			// Управление вернули этому телу — удаление больше не требуется
			CancelRetry();
			return;
		}

		m_iDeleteRetryCount++;
		if (m_iDeleteRetryCount >= MAX_DELETE_RETRIES)
		{
			CancelRetry();
			return;
		}

		Rpc(RPC_DeleteMe);
	}

	//------------------------------------------------------------------------------------------------
	protected void CancelRetry()
	{
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(RetryDelete);
	}

	//------------------------------------------------------------------------------------------------
	void ~PS_DisableCharacterCollisionComponent()
	{
		CancelRetry();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		CancelRetry();
		super.OnDelete(owner);
	}
	
	/**
	 * @brief Удаление «осиротевшего» тела прежнего playable по запросу клиента-владельца прокси
	 * @rpc Owner -> Server (Reliable)
	 * @issue Аудит 2026-09: доверенная граница RPC + RPC-флуд
	 * @cause Сервер удалял owner без проверок (владелец прокси мог удалить тело, которым уже
	 *        управляют), а клиент слал Reliable RPC каждый кадр до фактического удаления.
	 * @solution Удаление только если сущность — персонаж, не управляемый ни одним игроком
	 *           (проверка на сервере по актуальному состоянию); клиент шлёт запрос один раз и
	 *           повторяет с интервалом и лимитом попыток (RetryDelete), снимая EntityEvent.FRAME.
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DeleteMe()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (!ChimeraCharacter.Cast(owner))
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> playerIds = {};
			playerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
			{
				PlayerController playerController = playerManager.GetPlayerController(playerId);
				if (playerController && playerController.GetControlledEntity() == owner)
					return; // телом управляют — удалять нельзя
			}
		}

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
