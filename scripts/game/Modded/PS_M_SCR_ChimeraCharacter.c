modded class SCR_ChimeraCharacter 
{
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