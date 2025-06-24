// Strange default button
modded class SCR_ButtonComponent
{
	ref ScriptInvoker<Widget> m_OnHoverWithWidget = new ScriptInvoker();
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);
		m_OnHover.Invoke(); // Why not pass by default???
		m_OnHoverWithWidget.Invoke(w);
		
		return false;
	}
}