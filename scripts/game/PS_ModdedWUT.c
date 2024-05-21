
modded class SCR_CallsignGroupComponent
{
	override bool IsUniqueRoleInUse(int roleToCheck)
	{
		// HOW THIS CAN LOST !!!OWN!!! GROUP?
		if (!m_Group) return true;
		
		return super.IsUniqueRoleInUse(roleToCheck);
	}
}