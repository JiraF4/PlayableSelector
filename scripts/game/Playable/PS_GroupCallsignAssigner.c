[ComponentEditorProps(category: "GameScripted/Callsign", description: "")]
class PS_GroupCallsignAssignerClass: ScriptComponentClass
{
};


class PS_GroupCallsignAssigner : ScriptComponent
{
	[Attribute("1", UIWidgets.EditBox, "", "")]
	protected int m_iCompanyCallsign;
	
	[Attribute("1", UIWidgets.EditBox, "", "")]
	protected int m_iPlatoonCallsign;
	
	[Attribute("1", UIWidgets.EditBox, "", "")]
	protected int m_iSquadCallsign;
	
	void GetCallsign(out int companyIndex, out int platoonIndex, out int squadIndex)
	{
		companyIndex = m_iCompanyCallsign;
		platoonIndex = m_iPlatoonCallsign;
		squadIndex = m_iSquadCallsign;
	}
}