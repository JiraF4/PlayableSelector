class PS_DebriefingFactionReportWidgetComponent : SCR_ScriptedWidgetComponent
{
	static const ResourceName m_sObjectiveLayout = "{0860E6EBB3D508DF}UI/Debriefing/ObjectiveFrame.layout";
	static const ResourceName m_sLossLayout = "{D2DC34AA650883BC}UI/Debriefing/LossFrame.layout";
	
	SCR_Faction m_Faction;
	
	ImageWidget m_wFactionColor;
	TextWidget m_wFactionName;
	TextWidget m_wResultText;
	
	VerticalLayoutWidget m_wObjectiveVerticalLayout;
	VerticalLayoutWidget m_wLossesVerticalLayout;
	
	ref array<PS_DebriefingObjectiveWidgetComponent> m_aObjectives = {};
	ref array<PS_DebriefingLossesWidgetComponent> m_aLosses = {};
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wFactionColor = ImageWidget.Cast(w.FindAnyWidget("FactionColor"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
		m_wResultText = TextWidget.Cast(w.FindAnyWidget("ResultText"));
		
		m_wObjectiveVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("ObjectiveVerticalLayout"));
		m_wLossesVerticalLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("LossesVerticalLayout"));
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		
	}
	
	void SetFaction(Faction faction)
	{
		m_Faction = SCR_Faction.Cast(faction);
		
		m_wFactionColor.SetColor(m_Faction.GetOutlineFactionColor());
		m_wFactionName.SetText(m_Faction.GetFactionName());
		
		FillObjectives();
		FillLosses();
		
		UpdateResult();
	}
	
	void UpdateResult()
	{
		PS_ObjectiveLevel objectiveLevel = PS_ObjectiveManager.GetInstance().GetFactionScoreLevel(m_Faction.GetFactionKey());
		m_wResultText.SetText(objectiveLevel.GetName());
	}
	
	void FillObjectives()
	{
		array<PS_Objective> outObjectives = {};
		PS_ObjectiveManager.GetInstance().GetObjectivesByFactionKey(m_Faction.GetFactionKey(), outObjectives);
		foreach (PS_Objective objective : outObjectives)
		{
			Widget objectiveWidget = GetGame().GetWorkspace().CreateWidgets(m_sObjectiveLayout, m_wObjectiveVerticalLayout);
			PS_DebriefingObjectiveWidgetComponent objectiveHandler = PS_DebriefingObjectiveWidgetComponent.Cast(objectiveWidget.FindHandler(PS_DebriefingObjectiveWidgetComponent));
			m_aObjectives.Insert(objectiveHandler);
			objectiveHandler.SetObjective(objective);
			objective.GetOnObjectiveUpdate().Insert(UpdateResult);
		}
		if (outObjectives.Count() == 0)
			m_wRoot.SetVisible(false);
	}
	
	void FillLosses()
	{
		CreateLossWidget(SCR_ChimeraCharacter, m_Faction, 0, "Character", "");
		CreateLossWidget(Vehicle, m_Faction, EVehicleType.APC, "APC", "APC2");
		CreateLossWidget(Vehicle, m_Faction, EVehicleType.TRUCK | EVehicleType.REPAIR | EVehicleType.COMM_TRUCK | EVehicleType.FUEL_TRUCK | EVehicleType.SUPPLY_TRUCK, "Truck", "Truck2");
		CreateLossWidget(Vehicle, m_Faction, EVehicleType.CAR, "Car", "Car2");
		CreateLossWidget(Vehicle, m_Faction, 0, "Helicopter", "Helicopter2");
	}
	
	void CreateLossWidget(typename unitType, Faction faction, int vehicleTypes, string unitName, string quadName)
	{
		Widget lossesWidget = GetGame().GetWorkspace().CreateWidgets(m_sLossLayout, m_wLossesVerticalLayout);
		PS_DebriefingLossesWidgetComponent lossesHandler = PS_DebriefingLossesWidgetComponent.Cast(lossesWidget.FindHandler(PS_DebriefingLossesWidgetComponent));
		m_aLosses.Insert(lossesHandler);
		lossesHandler.SetUnitType(unitType, faction, vehicleTypes);
		lossesHandler.SetUnitName(unitName);
		lossesHandler.SetUnitIcon(quadName);
	}
}
