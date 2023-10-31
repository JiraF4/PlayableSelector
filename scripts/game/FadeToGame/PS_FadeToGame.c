// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	FadeToGame
}

//------------------------------------------------------------------------------------------------
//! Fullscreen map menu
class PS_FadeToGame: ChimeraMenuBase
{	
	protected float m_fFadeTime = 5.0;
	protected float m_fFadeMaxTime = 5.0;
	
	protected ImageWidget m_wFade;
	
	override void OnMenuOpen()
	{
		m_wFade = ImageWidget.Cast(GetRootWidget().FindAnyWidget("Fade"));
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		m_fFadeTime = m_fFadeTime - tDelta;
		m_wFade.SetOpacity(0.6 * (m_fFadeTime / m_fFadeMaxTime) + 0.4);
		if (m_fFadeTime <= 0) Close();
	}
};