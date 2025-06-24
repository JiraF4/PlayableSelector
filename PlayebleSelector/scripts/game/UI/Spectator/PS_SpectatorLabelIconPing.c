class PS_SpectatorLabelIconPing : PS_SpectatorLabelIcon
{	
	override void Update()
	{
		super.Update();
		
		m_fMaxIconSize = 64 + Math.Sin(GetGame().GetWorld().GetWorldTime() / 10) * 12;
	}
}