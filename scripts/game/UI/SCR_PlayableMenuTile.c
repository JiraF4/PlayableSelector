class SCR_PlayableMenuTile : SCR_ButtonBaseComponent
{
	[Attribute("LoadoutMessage", desc: "Widget name of simple message component")]
	protected string m_sSimpleMessageName;
		
	protected OverlayWidget m_wPlayerOverlay;
	protected TextWidget m_wPlayerText;
	protected TextWidget m_wNameText;
	protected ImageWidget m_wPlayerBackground;
	
	protected ImageWidget m_wFactionBackground;
	protected ImageWidget m_wFactionFlag;
	protected SCR_LoadoutPreviewComponent m_Preview;
	protected int m_PlayableId;
	
	override bool OnFocus(Widget w, int x, int y)
	{
		super.OnFocus(w, x, y);
		
// 		if (m_Parent)
//			m_Parent.FocusTile(this);
		
		return false;
	}

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wFactionBackground = ImageWidget.Cast(w.FindAnyWidget("FactionBckg"));
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("ImageFlag"));
		
		m_wPlayerOverlay = OverlayWidget.Cast(w.FindAnyWidget("PlayerOverlay"));
		m_wPlayerText = TextWidget.Cast(w.FindAnyWidget("PlayerText"));
		m_wNameText = TextWidget.Cast(w.FindAnyWidget("NameText"));
		m_wPlayerBackground = ImageWidget.Cast(w.FindAnyWidget("PFactionBckg"));
		
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
	}
	
	void SetNameText(string strName)
	{
		m_wNameText.SetText(strName);
	}

	void SetPreviewedLoadout(SCR_BasePlayerLoadout loadout)
	{
		IEntity ent = m_Preview.SetPreviewedLoadout(loadout);
		ent.SetFixedLOD(0);
	}
	
	void SetPlayer(int playerId)
	{
		if (playerId == 0) {
			m_wPlayerOverlay.SetVisible(false);
			return;
		};
		m_wPlayerText.SetText(GetGame().GetPlayerManager().GetPlayerName(playerId));
		m_wPlayerOverlay.SetVisible(true);
	}
	
	void SetDead()
	{
		m_wPlayerOverlay.SetVisible(true);
		m_wPlayerText.SetText("Dead");
		m_wPlayerBackground.SetColor(Color.FromInt(Color.BLACK));
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