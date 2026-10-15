// Freezes local character controls for UI windows (refcounted) and the hard freeze
// (replicated state, passed to EnforceView). A one-shot SetDisable*Controls cannot work:
// SCR_BaseGameMode rewrites weapon and movement flags every frame and SCR_PlayerController
// re-enables all three when the last menu closes, so the per-frame writer in
// PS_GameModeCoop.SetLocalControls consults this class. Refcounted because multiple
// UI windows may open concurrently.
class PS_InputLock
{
	protected static int s_iLocks;
	protected static IEntity s_ViewFrozenEntity;

	//! Resets static state on world start/teardown.
	static void ResetStatic()
	{
		if (s_ViewFrozenEntity)
		{
			SetViewDisabled(s_ViewFrozenEntity, false);
			s_ViewFrozenEntity = null;
		}
		s_iLocks = 0;
	}

	static void Acquire()
	{
		s_iLocks++;
	}

	static void Release()
	{
		if (s_iLocks > 0)
			s_iLocks--;
	}

	static bool IsLocked()
	{
		return s_iLocks > 0;
	}

	//! Per-frame view lock enforcement called from PS_GameModeCoop.SetLocalControls
	static void EnforceView(bool alsoLocked = false)
	{
		IEntity target;
		if (s_iLocks > 0 || alsoLocked)
		{
			PlayerController playerController = GetGame().GetPlayerController();
			if (playerController)
				target = playerController.GetControlledEntity();
		}

		if (s_ViewFrozenEntity && s_ViewFrozenEntity != target)
			SetViewDisabled(s_ViewFrozenEntity, false);

		s_ViewFrozenEntity = target;
		if (target)
			SetViewDisabled(target, true);
	}

	protected static void SetViewDisabled(IEntity entity, bool disabled)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return;

		CharacterControllerComponent charController = character.GetCharacterController();
		if (!charController)
			return;

		charController.SetDisableViewControls(disabled);
	}
}
