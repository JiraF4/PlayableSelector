class PS_ObjectiveClass : GenericEntityClass
{

}

class PS_Objective : GenericEntity
{
	[Attribute("")]
	int m_iCost;
	
	[Attribute("")]
	string m_sName;
	
	[Attribute("")]
	string m_sDescription;
	
	[Attribute("")]
	FactionKey m_sFactionKey;
	
	void PS_Objective(IEntitySource src, IEntity parent)
	{
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().RegisterObjective(this);
	}
	
	FactionKey GetFactionKey()
	{
		return m_sFactionKey;
	}
	
	void ~PS_Objective()
	{
		if (PS_ObjectiveManager.GetInstance())
			PS_ObjectiveManager.GetInstance().UnRegisterObjective(this);
	}
}