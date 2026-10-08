class PS_MissionDescriptionEditableUI : ScriptedWidgetComponent
{
	Widget m_wRoot;
	
	protected PS_MissionDescription m_rMissionDescription;
	protected ref PS_RichTextFlowRenderer m_RichTextRenderer;
	
	RichTextWidget m_wRichTextEditable;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
		
		m_wRichTextEditable = RichTextWidget.Cast(w.FindAnyWidget("RichTextEditable"));
		
		GetGame().GetCallqueue().CallLater(GetDescription, 0);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(GetDescription);
		if (m_RichTextRenderer)
			m_RichTextRenderer.Clear();
		m_RichTextRenderer = null;
		super.HandlerDeattached(w);
	}

	void GetDescription()
	{
		PS_MissionDescriptionContentUI currentHandlerContent = PS_MissionDescriptionContentUI.Cast(m_wRoot.FindHandler(PS_MissionDescriptionContentUI));
		if (!currentHandlerContent)
			return;

		m_rMissionDescription = currentHandlerContent.GetMissionDescription();
		UpdateDescription();
	}
	
	void UpdateDescription()
	{
		if (!m_wRichTextEditable || !m_rMissionDescription)
			return;

		string description = m_rMissionDescription.GetTextData();
		if (m_RichTextRenderer)
		{
			m_RichTextRenderer.Clear();
			m_RichTextRenderer = null;
		}

		if (!description.Contains("<link=") && !description.Contains("<h>") && !description.Contains("<b>") && !description.Contains("<color=") && !description.Contains("<br>") && !description.Contains("<br/>") && !description.Contains("<hr") && !description.Contains("<gap"))
		{
			m_wRichTextEditable.SetVisible(true);
			m_wRichTextEditable.SetText(PS_RichTextFlowRenderer.StripUnknownTags(description));
			return;
		}

		Widget scroll = m_wRichTextEditable.GetParent();
		if (!scroll)
			return;

		m_wRichTextEditable.SetVisible(false);
		m_wRichTextEditable.SetText("");
		m_RichTextRenderer = new PS_RichTextFlowRenderer();
		m_RichTextRenderer.Render(scroll, description, 449);
	}
	
}
