//! SCR_ButtonComponent is deprecated (Constitution VII); kept only because existing layouts
//! still reference it. The `return false` is vanilla parity, not a bug: vanilla
//! SCR_ButtonComponent.OnMouseEnter discards the base result and returns false unconditionally.
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