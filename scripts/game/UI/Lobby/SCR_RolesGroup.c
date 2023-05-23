class SCR_RolesGroup : SCR_WLibComponentBase
{
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ref array<Widget> m_aCharactersListWidgets = {};
	Widget m_wCharactersList;
	TextWidget m_wRolesGroupName;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCharactersList = w.FindAnyWidget("CharactersList");
		m_wRolesGroupName = TextWidget.Cast(w.FindAnyWidget("RolesGroupName"));
	}
	
	Widget AddPlayable(SCR_PlayableComponent playable)
	{
		Widget CharacterSelector = GetGame().GetWorkspace().CreateWidgets(m_sCharacterSelectorPrefab);
		SCR_CharacterSelector handler = SCR_CharacterSelector.Cast(CharacterSelector.FindHandler(SCR_CharacterSelector));
		handler.SetPlayable(playable);
		m_aCharactersListWidgets.Insert(CharacterSelector);
		m_wCharactersList.AddChild(CharacterSelector);
		return CharacterSelector;
	}
	
	void SetName(string name)
	{
		m_wRolesGroupName.SetText(name);
	}
	
	void GetWidgets(out array<Widget> charactersListWidgets)
	{
		charactersListWidgets.Copy(m_aCharactersListWidgets);
	}
}