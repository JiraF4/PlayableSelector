// PS_M_SCR_VoNComponent — engine hooks for the body-less lobby "talking device".
// Ported from LiteLobby (LL_M_SCR_VoNComponent).
//
// Players without a (living) character — slot selection, briefing, spectator —
// transmit through a SCR_VoNComponent + radio on their replicated VoN proxy entity
// (PS_VoNProxyComponent). The engine only accepts a VoNComponent that is NOT on the
// controlled entity when it is registered via ConnectEditorToVoNSystem(playerId)
// (this is how the Game Master editor talks); it then resolves the sender through the
// three editor callbacks below. The vanilla implementations only recognize
// SCR_EditorManagerEntity, so a connected lobby device would be rejected — these
// overrides teach the engine to accept a player's VoN proxy as the sender whenever
// that player is expected to speak through the menu device.
//
// Also tracks per-player "is talking" state for the voice UI: OnReceive fires for
// incoming audio (other players), OnCapture fires every frame while the local player
// transmits (the engine never echoes your own voice back through OnReceive).

modded class SCR_VoNComponent
{
	// WHY 400ms: OnReceive/OnCapture fire continuously during a transmission but stop
	// without any "ended" event — talking ends when no packet arrived for this long.
	protected static const int PS_TALK_TIMEOUT_MS = 400;

	// playerId -> world time (ms) after which the player counts as silent.
	protected static ref map<int, float> s_mPSTalkUntil = new map<int, float>();

	// (int playerId, bool talking)
	protected static ref ScriptInvoker s_OnPSTalkingChanged = new ScriptInvoker();

	static ScriptInvoker PS_GetOnTalkingChanged()
	{
		return s_OnPSTalkingChanged;
	}

	static bool PS_IsTalking(int playerId)
	{
		float until;
		if (!s_mPSTalkUntil.Find(playerId, until))
			return false;

		return until > GetGame().GetWorld().GetWorldTime();
	}

	// =====================================================================
	// MENU SPEAKER PREDICATE
	// =====================================================================

	// Authority derives menu ownership from the body; clients follow its parked flag.
	// DESTROYED is checked separately because the spectator transition can precede IsDead().
	static bool PS_IsMenuSpeaker(int playerId)
	{
		// Clients use the authoritative flag even while the corpse/possession is still replicating.
		PS_VoNRoomsManager vonManager = PS_VoNRoomsManager.GetInstance();
		if (!Replication.IsServer() && vonManager && vonManager.IsReplicated())
			return !vonManager.IsPlayerParked(playerId);

		IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!controlled)
			return true;

		ChimeraCharacter character = ChimeraCharacter.Cast(controlled);
		if (character)
		{
			CharacterControllerComponent characterController = character.GetCharacterController();
			if (characterController && characterController.IsDead())
				return true;

			// Also treat a DESTROYED damage state as dead. THIS is the spectator-voice fix: this mod
			// drives the death -> spectator transition off the damage manager
			// (PS_PlayableComponent.OnDamageStateChange == DESTROYED, no respawns -> SwitchToInitialEntity),
			// which can be set WITHOUT CharacterControllerComponent.IsDead() ever reading true here (unlike
			// vanilla OnPlayerKilled, which LiteLobby keys off). The engine re-checks this predicate on
			// EVERY capture/receive (via the IsEntityActiveEditor / GetEditorEntity callbacks below), so if
			// it reads false for a dead spectator the engine rejects the proxy's voice outright: no
			// OnCapture, no reception, the talking-state widget never lights = the exact reported bug.
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
			if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
				return true;
		}

		return false;
	}

	// =====================================================================
	// EDITOR CALLBACKS — engine asks these to validate/locate a connected sender
	// =====================================================================

	override protected event IEntity GetEditorEntity(int playerId)
	{
		// An OPENED editor always wins the "editor sender" slot — the GM took over
		// VoN and the menu device is suspended for that player.
		IEntity editor = super.GetEditorEntity(playerId);
		if (editor && super.IsEntityActiveEditor(editor))
			return editor;

		if (PS_IsMenuSpeaker(playerId))
		{
			// The proxy replicates to every machine, so the sender resolves on
			// receivers too (a PlayerController would not — owner<->server only).
			IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
			if (proxy)
				return proxy;
		}

		return editor;
	}

	override protected event vector GetEditorWorldLocation(int playerId)
	{
		// Real GM editor retains vanilla behavior
		SCR_EditorManagerEntity gm = SCR_EditorManagerEntity.Cast(super.GetEditorEntity(playerId));
		if (gm && super.IsEntityActiveEditor(gm))
			return super.GetEditorWorldLocation(playerId);

		// Resolve virtual position for menu/lobby/spectator voice rooms
		PS_VoNRoomsManager vonMgr = PS_VoNRoomsManager.GetInstance();
		if (vonMgr)
			return vonMgr.GetRoomPosition(playerId);

		// Fallback to proxy entity origin if rooms manager is unavailable
		IEntity proxy = PS_VoNProxyComponent.GetProxyEntity(playerId);
		if (proxy)
			return proxy.GetOrigin();

		return vector.Zero;
	}

	override bool IsEntityActiveEditor(IEntity entity)
	{
		if (super.IsEntityActiveEditor(entity))
			return true;

		// A VoN proxy counts as an "active editor" while its player is a menu speaker
		// — that is the sender GetEditorEntity resolves to.
		if (entity)
		{
			PS_VoNProxyComponent proxyComp = PS_VoNProxyComponent.Cast(entity.FindComponent(PS_VoNProxyComponent));
			if (proxyComp)
				return PS_IsMenuSpeaker(proxyComp.GetPlayerId());
		}

		return false;
	}

	// =====================================================================
	// TALKING STATE — feed the voice UI
	// =====================================================================

	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{
		super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
		PS_MarkTalking(playerId);
	}

	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		super.OnCapture(transmitter);

		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			PS_MarkTalking(pc.GetPlayerId());
	}

	protected static void PS_MarkTalking(int playerId)
	{
		float now = GetGame().GetWorld().GetWorldTime();

		float until;
		bool wasTalking = s_mPSTalkUntil.Find(playerId, until) && until > now;

		s_mPSTalkUntil.Set(playerId, now + PS_TALK_TIMEOUT_MS);

		if (!wasTalking)
		{
			s_OnPSTalkingChanged.Invoke(playerId, true);
			GetGame().GetCallqueue().CallLater(PS_CheckTalkEnd, PS_TALK_TIMEOUT_MS, false, playerId);
		}
	}

	protected static void PS_CheckTalkEnd(int playerId)
	{
		float until;
		if (!s_mPSTalkUntil.Find(playerId, until))
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (until > now)
		{
			GetGame().GetCallqueue().CallLater(PS_CheckTalkEnd, until - now + 10, false, playerId);
			return;
		}

		s_mPSTalkUntil.Remove(playerId);
		s_OnPSTalkingChanged.Invoke(playerId, false);
	}
}
