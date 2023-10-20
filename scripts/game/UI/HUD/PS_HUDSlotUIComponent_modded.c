
modded  class SCR_HUDSlotUIComponent
{
	override void Initialize()
	{
		if (m_bInitialized)
			return;

		Widget child = m_wRoot.GetChildren();
		
		// WHY THIS THING POOP IN MY LOGS
		// I'M NOT INTERESTED IN FUCKING SLOTS
		// THIS EVEN HAPPEN ON NORMAL GAMEMODE, STOP IT, IT'S AGAINST HUMANITY, GOD AND (What is more important) AGAINST ME.
		/*
		if (!child)
			Print(m_wRoot.GetName() + " Has no Content!", LogLevel.WARNING);
		*/
		// Sorry i trigger a little...
		
		if (!GetGroupComponent())
			Debug.Error2("No Group Component", "A Slot's parent must always have a Group Component! Slot: " + m_wRoot.GetName());

		m_wWorkspace = GetGame().GetWorkspace();
		m_bInitialized = true;
	}
}
