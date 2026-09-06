// M_SCR_NotificationsLogDisplay — vanilla VME guard.
//
// During Workbench "Reload Game" the entity tree is torn down and
// SCR_NotificationsLogDisplay.m_slotHandler is already nullified when
// SCR_InfoDisplayExtended.OnStopDraw (line 517) calls DisplayStopDraw.
// Vanilla DisplayStopDraw dereferences m_slotHandler without a null check
// -> VME: "NULL pointer to instance" every reload cycle.
//
// DisplayUpdate also dereferences m_slotHandler, but that path is gated by
// m_bCanShow (false during teardown), so it never fires.  Only DisplayStopDraw
// needs the guard.
modded class SCR_NotificationsLogDisplay
{
	override void DisplayStopDraw(IEntity owner)
	{
		if (!m_slotHandler) //For workbench
			return;
		super.DisplayStopDraw(owner);
	}
}
