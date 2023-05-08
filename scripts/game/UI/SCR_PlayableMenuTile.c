class SCR_PlayableMenuTile : SCR_DeployMenuTile
{
	[Attribute("LoadoutMessage", desc: "Widget name of simple message component")]
	protected string m_sSimpleMessageName;
	
	//~ Disabled when loadout page is loaded
	protected bool m_bForceDisabled;
	
	protected ImageWidget m_wIcon;
	protected ImageWidget m_wFactionBackground;
	protected ImageWidget m_wFactionFlag;
	protected SCR_LoadoutPreviewComponent m_Preview;
	protected int m_PlayableId;
	
	override bool OnFocus(Widget w, int x, int y)
	{
		super.OnFocus(w, x, y);
		
		//~ Only sets the tile as selected if it was not forced disabled. Else it will set itself as null
		if (m_Parent && !m_bForceDisabled)
			m_Parent.FocusTile(this);
		else if (m_bForceDisabled)
			m_Parent.FocusTile(null);
		
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wIcon = ImageWidget.Cast(w.FindAnyWidget("Icon"));
		m_wFactionBackground = ImageWidget.Cast(w.FindAnyWidget("FactionBckg"));
		m_wFactionFlag = ImageWidget.Cast(w.FindAnyWidget("ImageFlag"));
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
	}

	//------------------------------------------------------------------------------------------------
	void SetPreviewedLoadout(SCR_BasePlayerLoadout loadout)
	{
		if (m_Preview)
		{
			IEntity ent = m_Preview.SetPreviewedLoadout(loadout);
			if (!ent)
				return;

			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (affiliation)
			{
				Faction faction = affiliation.GetAffiliatedFaction();
				if (faction)
					m_wFactionBackground.SetColor(faction.GetFactionColor());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetIcon(SCR_EditableEntityUIInfo info)
	{
		if (!info)
			return;
		info.SetIconTo(m_wIcon);
		m_wIcon.SetVisible(true);
	}
	
	void SetFaction(SCR_Faction faction)
	{
		m_wFactionBackground.SetColor(faction.GetFactionColor());
		m_wFactionFlag.LoadImageTexture(0, faction.GetFactionFlag());
		Print(faction.GetFactionFlag())
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