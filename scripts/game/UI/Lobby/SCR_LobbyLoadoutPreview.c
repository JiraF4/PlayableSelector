class SCR_LobbyLoadoutPreview : SCR_WLibComponentBase
{
	protected ImageWidget m_wFlagImage;
	protected ImageWidget m_wStateBackgroundImage;
	protected ImageWidget m_wLoadoutBackgroundImage;
	protected OverlayWidget m_WStateOverlay;
	protected OverlayWidget m_WLoadoutOverlay;
	protected TextWidget m_wStateText;
	protected TextWidget m_wLoadoutText;
	protected SCR_LoadoutPreviewComponent m_Preview;
	
	protected SCR_PlayableComponent m_playable;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFlagImage = ImageWidget.Cast(w.FindAnyWidget("FlagImage"));
		m_wStateBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("StateBackgroundImage"));
		m_wLoadoutBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("LoadoutBackgroundImage"));
		m_wStateText = TextWidget.Cast(w.FindAnyWidget("StateText"));
		m_wLoadoutText = TextWidget.Cast(w.FindAnyWidget("LoadoutText"));
		m_WStateOverlay = OverlayWidget.Cast(w.FindAnyWidget("StateOverlay"));
		m_WLoadoutOverlay = OverlayWidget.Cast(w.FindAnyWidget("LoadoutOverlay"));
		
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
	}
	
	void SetPlayable(SCR_PlayableComponent playable)
	{
		m_playable = playable;
		UpdatePreviewInfo();
	}
	
	void UpdatePreviewInfo()
	{
		ItemPreviewManagerEntity m_PreviewManager = GetGame().GetItemPreviewManager();
		if (m_playable == null) {
			m_wFlagImage.SetVisible(false);
			m_WStateOverlay.SetVisible(false);
			m_WLoadoutOverlay.SetVisible(false);
			m_PreviewManager.SetPreviewItem(m_Preview.GetItemPreviewWidget(), null);
			return;
		}
		m_wFlagImage.SetVisible(true);
		m_WLoadoutOverlay.SetVisible(true);
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
		SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
		
		character.SetFixedLOD(0);
		m_PreviewManager.SetPreviewItem(m_Preview.GetItemPreviewWidget(), character);
		
		m_wLoadoutBackgroundImage.SetColor(faction.GetFactionColor());
		m_wLoadoutText.SetText(m_playable.GetName());
		m_wFlagImage.LoadImageTexture(0, faction.GetFactionFlag());	
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		int playerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(character);
		
		if (character.GetDamageManager().IsDestroyed()) 
		{
			m_WStateOverlay.SetVisible(true);
			m_wStateText.SetText("Dead");
			m_wStateBackgroundImage.SetColor(Color.FromInt(Color.BLACK));
		}
		else 
		{
			if (playerId != 0) 
			{
				m_WStateOverlay.SetVisible(true);
				m_wStateText.SetText(playerManager.GetPlayerName(playerId));
				m_wStateBackgroundImage.SetColor(Color.FromInt(0xFF688869));
			}else{
				m_WStateOverlay.SetVisible(false);
			}
		}
	}
	
}