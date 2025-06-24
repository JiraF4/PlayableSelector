[ComponentEditorProps(category: "GameScripted/Editor (Editables)", description: "", icon: "WBData/ComponentEditorProps/componentEditor.png")]
class PS_EditableMissionDescriptionComponentClass: SCR_EditableDescriptorComponentClass
{
	
};

class PS_EditableMissionDescriptionComponent: SCR_EditableDescriptorComponent
{
	PS_MissionDescription m_eMissionDescription;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_eMissionDescription = PS_MissionDescription.Cast(owner);
	}
	
	bool GetDescriptionVisibleForFaction(Faction faction)
	{
		return m_eMissionDescription.GetVisibleForFaction(faction.GetFactionKey());
	}
	void SetDescriptionVisibleForFaction(Faction faction, bool visible)
	{
		m_eMissionDescription.SetVisibleForFaction(faction, visible);
	}
	
	bool GetDescriptionVisibleForEmpty()
	{
		return m_eMissionDescription.GetVisibleForEmptyFaction();
	}
	void SetDescriptionVisibleForEmpty(bool visible)
	{
		m_eMissionDescription.SetVisibleForEmptyFaction(visible);
	}
	
	string GetMissionTitle()
	{
		return m_eMissionDescription.GetTitle();
	}
	void SetMissionTitle(string title)
	{
		m_eMissionDescription.SetTitle(title);
	}
	
	string GetMissionTextData()
	{
		return m_eMissionDescription.GetTextData();
	}
	void SetMissionTextData(string title)
	{
		m_eMissionDescription.SetTextData(title);
	}
};
