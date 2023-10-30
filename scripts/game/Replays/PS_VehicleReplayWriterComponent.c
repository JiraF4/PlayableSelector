[ComponentEditorProps(category: "GameScripted/Callsign", description: "")]
class PS_VehicleReplayWriterComponentClass: ScriptComponentClass
{
	
};

class PS_VehicleReplayWriterComponent : ScriptComponent
{
	RplId m_id;
	IEntity m_eOwner;
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddToReplay, 0, false, owner)
	}
	
	protected void PositionLogger()
	{
		// Regulary write position to replay
		PS_ReplayWriter.GetInstance().WriteEntityMove(m_id, m_eOwner);
		GetGame().GetCallqueue().CallLater(PositionLogger, 100);
	}
	
	void AddToReplay(IEntity owner)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_id = rpl.Id();
		m_eOwner = owner;
		PS_ReplayWriter.GetInstance().WriteVehicleRegistration(m_id, PS_EReplayVehicleType.BTR70);
		GetGame().GetCallqueue().CallLater(PositionLogger, 0);
	}	
};