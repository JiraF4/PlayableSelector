//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class PS_DirectMessageSelectedContextAction : SCR_SelectedEntitiesContextAction
{
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBePerformed(selectedEntity, cursorWorldPosition, flags);
	}
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(selectedEntity.GetOwner().FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return false;
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return false;
		int playerId = playableManager.GetPlayerByPlayable(playableComponent.GetRplId());
		if (playerId <= 0)
			return false;
		
		return selectedEntity
			&& !selectedEntity.HasEntityFlag(EEditableEntityFlag.NON_DELETABLE)
			&& !selectedEntity.HasEntityFlag(EEditableEntityFlag.NON_INTERACTIVE);
	}
	override void PerformOwner(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(selectedEntity.GetOwner().FindComponent(PS_PlayableComponent));
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		int playerId = playableManager.GetPlayerByPlayable(playableComponent.GetRplId());
		
		MenuBase menuBase = GetGame().GetMenuManager().GetTopMenu();
		
		Widget chat = menuBase.GetRootWidget().FindAnyWidget("ChatPanel");
		if (!chat)
			chat = GetGame().GetWorkspace().FindAnyWidget("ChatPanel");
		if (!chat)
			return;
		
		SCR_ChatPanel chatPanel = SCR_ChatPanel.Cast(chat.FindHandler(SCR_ChatPanel));
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(chatPanel);
		EditBoxWidget editBoxWidget = chatPanel.PS_GetWidgets().m_MessageEditBox;
		editBoxWidget.SetText("/dmsg " + playerId.ToString() + " ");
	}
};
