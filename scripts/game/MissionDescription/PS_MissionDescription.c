[BaseContainerProps()]
class PS_MapDescription
{
	[Attribute("")]
	string m_sTitle;
	[Attribute("")]
	string m_sFactions;
	[Attribute("")]
	ResourceName m_sDescriptionLayout;
	ref array<string> m_aFactions = new array<string>();
}