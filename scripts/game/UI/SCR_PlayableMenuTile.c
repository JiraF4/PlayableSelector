class SCR_PlayableMenuTile : SCR_DeployMenuTile
{
	[Attribute("LoadoutMessage", desc: "Widget name of simple message component")]
	protected string m_sSimpleMessageName;
		
	protected ImageWidget m_wFactionBackground;
	protected ImageWidget m_wFactionFlag;
	protected SCR_LoadoutPreviewComponent m_Preview;
	protected int m_PlayableId;
	
	override bool OnFocus(Widget w, int x, int y)
	{
		super.OnFocus(w, x, y);
		
		if (m_Parent)
			m_Parent.FocusTile(this);
		
		return false;
	}

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wFactionBackground = ImageWidget.Cast(w.FindAnyWidget("FactionBckg"));
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("ImageFlag"));
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
	}

	void SetPreviewedLoadout(SCR_BasePlayerLoadout loadout)
	{
		IEntity ent = m_Preview.SetPreviewedLoadout(loadout);
		ent.SetFixedLOD(0);
	}
	
	void SetFaction(SCR_Faction faction)
	{
		m_wFactionBackground.SetColor(faction.GetFactionColor());
		m_wFactionFlag.LoadImageTexture(0, faction.GetFactionFlag());
	}
	
	void SetPlayableId(int PlayableId)
	{
		m_PlayableId = PlayableId
	}
	
	int GetPlayableId()
	{
		return m_PlayableId;
	}
};