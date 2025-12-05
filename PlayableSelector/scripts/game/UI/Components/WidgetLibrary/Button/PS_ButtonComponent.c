void PS_ScriptInvokerOnClickMethod(PS_ButtonComponent buttonComponent, int x, int y, int button);
typedef func PS_ScriptInvokerOnClickMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnClickMethod> PS_ScriptInvokerOnClick;

class PS_ButtonComponent : SCR_ButtonComponent
{	
	protected ref PS_ScriptInvokerOnClick m_PS_OnClick = new PS_ScriptInvokerOnClick();
	
	//------------------------------------------------------------------------------------------------
	PS_ScriptInvokerOnClick GetOnClick_PS()
	{
		return m_PS_OnClick;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		m_PS_OnClick.Invoke(this, x, y, button);
		return false;
	}
};