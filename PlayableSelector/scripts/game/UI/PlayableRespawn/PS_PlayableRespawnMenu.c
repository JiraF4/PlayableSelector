// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	PlayableRespawnMenu
}

//------------------------------------------------------------------------------------------------
//! Respawn await menu
class PS_PlayableRespawnMenu: ChimeraMenuBase
{	
	protected Widget m_wRoot;
	protected TextWidget m_wTimerText;
	
	protected PS_PlayableComponent m_PlayableComponent;
	
	protected float m_fRespawnTime;
	
	override void OnMenuOpen()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			Close();
			return;
		}
		
		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalMainEntity());
		if (!chimeraCharacter)
		{
			Close();
			return;
		}
		
		m_PlayableComponent = chimeraCharacter.PS_GetPlayable();
		m_fRespawnTime = (float) m_PlayableComponent.GetRespawnTime() / 1000.0;
		
		m_wRoot = GetRootWidget();
		m_wTimerText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerText"));
		
		m_wTimerText.SetText(m_fRespawnTime.ToString(7, 3));
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		m_fRespawnTime -= tDelta;
		if (m_fRespawnTime < 0)
			m_fRespawnTime = 0;
		m_wTimerText.SetText(m_fRespawnTime.ToString(7, 3));
		if (m_fRespawnTime <= 0)
			Close();
	}
};