modded class DSGameConfig: JsonApiStruct
{
	void DSGameConfig()
	{
		maxPlayers = 128;
		
	}
}


modded class DSConfig
{
	
	void DSGameConfig()
	{
		game.maxPlayers = 128;
	}
}