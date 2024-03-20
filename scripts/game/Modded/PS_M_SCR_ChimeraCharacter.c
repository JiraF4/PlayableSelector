modded class SCR_ChimeraCharacter 
{
	PS_PlayableComponent PS_m_PlayableComponent;
	
	void PS_SetPlayable(PS_PlayableComponent playableComponent)
	{
		PS_m_PlayableComponent = playableComponent;
	}
	
	PS_PlayableComponent PS_GetPlayable()
	{
		return PS_m_PlayableComponent;
	}
	
	static ref array<SCR_ChimeraCharacter> m_aCharacters_PS = {};
	
	void SCR_ChimeraCharacter(IEntitySource src, IEntity parent)
	{
		m_aCharacters_PS.Insert(this);
	}
	
	void ~SCR_ChimeraCharacter()
	{
		m_aCharacters_PS.RemoveItem(this);
	}
}