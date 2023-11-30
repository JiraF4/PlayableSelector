[BaseContainerProps()]
class PS_MissionDescriptionFactionsEditorAttribute: SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return null;
		
		super.ReadVariable(item, manager);
		
		SCR_DelegateFactionManagerComponent delegateFactionManager = SCR_DelegateFactionManagerComponent.GetInstance();		
		if (!delegateFactionManager)
			return null;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return null;
		
		SCR_SortedArray<SCR_EditableFactionComponent> factionDelegates = new SCR_SortedArray<SCR_EditableFactionComponent>;
		int count = delegateFactionManager.GetSortedFactionDelegates(factionDelegates);
		SCR_Faction scrFaction;
		
		for(int i = 0; i < count; ++i)
		{
			scrFaction = SCR_Faction.Cast(factionDelegates.Get(i).GetFaction());
			
			if (!scrFaction)
				continue;
			
			AddOrderedState(missionDescription.GetDescriptionVisibleForFaction(scrFaction));
		}
		
		return SCR_BaseEditorAttributeVar.CreateVector(GetFlagVector());
	}
	
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{		
		super.WriteVariable(item, var, manager, playerID);
		
		PS_EditableMissionDescriptionComponent missionDescription = PS_EditableMissionDescriptionComponent.Cast(item);
		if (!missionDescription)
			return;
		
		SCR_DelegateFactionManagerComponent delegateFactionManager = SCR_DelegateFactionManagerComponent.GetInstance();		
		if (!delegateFactionManager)
			return;
		
		SCR_SortedArray<SCR_EditableFactionComponent> factionDelegates = new SCR_SortedArray<SCR_EditableFactionComponent>;
		int count = delegateFactionManager.GetSortedFactionDelegates(factionDelegates);
		SCR_Faction scrFaction;
		
		for(int i = 0; i < count; ++i)
		{
			scrFaction = SCR_Faction.Cast(factionDelegates.Get(i).GetFaction());
			
			if (!scrFaction)
				continue;
			
			missionDescription.SetDescriptionVisibleForFaction(scrFaction, GetOrderedState());
		}
	}
	
	override protected void CreatePresets()
	{
		SCR_DelegateFactionManagerComponent delegateFactionManager = SCR_DelegateFactionManagerComponent.GetInstance();		
		if (!delegateFactionManager)
			return;
		
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;
		
		m_aValues.Clear();	
		
		SCR_SortedArray<SCR_EditableFactionComponent> factionDelegates = new SCR_SortedArray<SCR_EditableFactionComponent>;
		int count = delegateFactionManager.GetSortedFactionDelegates(factionDelegates);
			
		SCR_Faction scrFaction;
		
		for(int i = 0; i < count; ++i)
		{
			scrFaction = SCR_Faction.Cast(factionDelegates.Get(i).GetFaction());
			
			if (!scrFaction)
				continue;

			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetWithUIInfo(scrFaction.GetUIInfo(), factionManager.GetFactionIndex(factionDelegates.Get(i).GetFaction()));
			m_aValues.Insert(value);
		}
	}
};