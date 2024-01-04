class PS_ObjectiveClass : GenericEntityClass
{

}

class PS_Objective : GenericEntity
{
	void PS_Objective(IEntitySource src, IEntity parent)
	{
		PS_ObjectiveManager.GetInstance().RegisterObjective(this);
	}
	
	void ~PS_Objective()
	{
		PS_ObjectiveManager.GetInstance().UnRegisterObjective(this);
	}
}