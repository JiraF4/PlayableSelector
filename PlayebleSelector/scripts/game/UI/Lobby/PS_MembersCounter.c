class PS_MembersCounter : SCR_ScriptedWidgetComponent
{
	int m_iCount = 0;
	
	// Widgets
	protected ImageWidget m_wUnitIcon;
	protected TextWidget m_wCounter;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wCounter = TextWidget.Cast(w.FindAnyWidget("Counter"));
	}

	//------------------------------------------------------------------------------------------------
	void SetIcon(ResourceName iconPath)
	{
		m_wUnitIcon.LoadImageTexture(0, iconPath);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCount(int count)
	{
		m_iCount = count;
		m_wCounter.SetText("x" + count);
		m_wRoot.SetVisible(m_iCount > 0);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCount()
	{
		return m_iCount;
	}
	
	
	//------------------------------------------------------------------------------------------------
	void Add(int add)
	{
		SetCount(m_iCount + add);
	}
}
