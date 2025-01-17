class PS_SpectatorLabelPingClass : PS_SpectatorLabelClass
{
}

class PS_SpectatorLabelPing : PS_SpectatorLabel
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		GetGame().GetCallqueue().CallLater(DeleteSelf, 5000, false);
	}
	
	void DeleteSelf()
	{
		delete GetOwner();
	}
}