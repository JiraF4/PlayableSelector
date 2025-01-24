void PS_ScriptInvokerOnContextActionMethod(PS_ContextAction contextAction, PS_ContextActionData contextActionData);
typedef func PS_ScriptInvokerOnContextActionMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnContextActionMethod> PS_ScriptInvokerOnContextAction;

class PS_ContextAction : SCR_ButtonComponent
{
	// Widgets
	ImageWidget m_wIcon;
	TextWidget m_wText;
	ButtonWidget m_wActionHotkey;
	ref PS_ContextActionData m_ContextActionData;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
	
		m_wIcon = ImageWidget.Cast(w.FindAnyWidget("Icon"));
		m_wText = TextWidget.Cast(w.FindAnyWidget("Text"));
		m_wActionHotkey = ButtonWidget.Cast(w.FindAnyWidget("ActionHotkey"));
	}
	
	//------------------------------------------------------------------------------------------------
	void Init(ResourceName icon, string quad, string name, string actionName, PS_ContextActionData contextActionData)
	{
		if (quad == "")
			m_wIcon.LoadImageTexture(0, icon);
		else
			m_wIcon.LoadImageFromSet(0, icon, quad);
		m_wText.SetText(name);
		m_wActionHotkey.SetVisible(false);
		m_ContextActionData = contextActionData;
		
		m_wBackground.SetColor(m_BackgroundDefault); // WTF?
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		
		m_eOnContextAction.Invoke(this, m_ContextActionData);
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	ref PS_ScriptInvokerOnContextAction m_eOnContextAction = new PS_ScriptInvokerOnContextAction();
	PS_ScriptInvokerOnContextAction GetOnOnContextAction()
		return m_eOnContextAction;
}

class PS_ContextActionData {
	
}
class PS_ContextActionDataPlayer : PS_ContextActionData {
	protected int m_iPlayerId;
	void PS_ContextActionDataPlayer(int playerId)
	{
		m_iPlayerId = playerId;
	}
	
	int GetPlayerId()
	{
		return m_iPlayerId;
	}
}
class PS_ContextActionDataPlayable : PS_ContextActionData {
	protected RplId m_iPlayableId;
	void PS_ContextActionDataPlayable(RplId playableId)
	{
		m_iPlayableId = playableId;
	}
	
	RplId GetPlayableId()
	{
		return m_iPlayableId;
	}
}
class PS_ContextActionDataCharacter : PS_ContextActionData {
	protected SCR_ChimeraCharacter m_Chracter;
	void PS_ContextActionDataCharacter(SCR_ChimeraCharacter character)
	{
		m_Chracter = character;
	}
	
	SCR_ChimeraCharacter GetCharacter()
	{
		return m_Chracter;
	}
}
