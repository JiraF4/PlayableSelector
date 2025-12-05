modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	PingMessageMenu
}

class PS_PingMessageMenu: MenuBase
{
	ref ScriptInvoker m_PingInvoker;
	bool m_bUnlimitedOnly;
	vector m_vPosition;
	SCR_EditableEntityComponent m_Target;
	
	ButtonWidget m_wButtonSend;
	ButtonWidget m_wButtonCancel;
	EditBoxWidget m_wEditBox;
	
	SCR_InputButtonComponent m_ButtonSend;
	SCR_InputButtonComponent m_ButtonCancel;
	SCR_EventHandlerComponent m_EventHandler;
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		m_wButtonSend = ButtonWidget.Cast(GetRootWidget().FindWidget("Overlay.HorizontalLayoutButtons.ButtonSend"));
		m_wButtonCancel = ButtonWidget.Cast(GetRootWidget().FindWidget("Overlay.HorizontalLayoutButtons.ButtonCancel"));
		m_wEditBox = EditBoxWidget.Cast(GetRootWidget().FindWidget("Overlay.Overlay0.EditBox0"));
		
		
		m_ButtonSend = SCR_InputButtonComponent.Cast(m_wButtonSend.FindHandler(SCR_InputButtonComponent));
		m_ButtonCancel = SCR_InputButtonComponent.Cast(m_wButtonCancel.FindHandler(SCR_InputButtonComponent));
		m_EventHandler = SCR_EventHandlerComponent.Cast(m_wEditBox.FindHandler(SCR_EventHandlerComponent));
		
		m_EventHandler.GetOnChange().Insert(OnEditBoxChange);
		
		EInputDeviceType currentDevice = GetGame().GetInputManager().GetLastUsedInputDevice();
		m_ButtonSend.SetEnabled(currentDevice == EInputDeviceType.GAMEPAD);
		m_ButtonSend.m_OnActivated.Insert(OnButtonSendPressed);
		m_ButtonCancel.m_OnActivated.Insert(OnButtonCancelPressed);
		
		m_wEditBox.ActivateWriteMode();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnEditBoxChange(Widget w)
	{
		string text = m_wEditBox.GetText();
		int bytesLength = text.Length();
		int charsLength = 0;
		for (int i = 0; i < bytesLength; i++)
		{
			if (text.ToAscii(i) < 0)
				i++;
			charsLength++;
		}
		EInputDeviceType currentDevice = GetGame().GetInputManager().GetLastUsedInputDevice();
		m_ButtonSend.SetEnabled(charsLength >= 15 || currentDevice == EInputDeviceType.GAMEPAD);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnButtonSendPressed(SCR_ButtonBaseComponent button)
	{
		m_PingInvoker.Invoke(m_bUnlimitedOnly, m_vPosition, m_Target, m_wEditBox.GetText());
		Close();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnButtonCancelPressed(SCR_ButtonBaseComponent button)
	{
		Close();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetPingData(ScriptInvoker pingInvoker, bool unlimitedOnly = false, vector position = vector.Zero, SCR_EditableEntityComponent target = null)
	{
		m_PingInvoker = pingInvoker;
		m_bUnlimitedOnly = unlimitedOnly;
		m_vPosition = position;
		m_Target = target;
	}
}