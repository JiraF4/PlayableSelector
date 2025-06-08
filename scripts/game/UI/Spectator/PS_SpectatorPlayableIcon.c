class PS_SpectatorPlayableIcon : SCR_ScriptedWidgetComponent
{	
	
	protected ImageWidget m_wPlayableIcon;
	protected TextWidget m_wPlayableLabel;
	protected PanelWidget m_wPanelLabel;
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected PS_PlayableContainer m_cPlayableComponent;
	
	protected PS_PPI_GameModeComponent m_PPI_GameModeComponent;
	protected PS_PPI_CharacterComponent m_PPI_CharacterComponent;
	protected PS_PPI_PlayerInfo m_PPI_PlayerInfo;
	
	protected float m_fMaxIconDistance = 800.0;
	protected float m_fMinIconDistance = 10.0;
	protected float m_fMaxIconSize = 64.0;
	protected float m_fMinIconSize = 4.0;
	protected float m_fMaxIconOpacity = 1;
	protected float m_fMinIconOpacity = 0.8;
	
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	protected float m_fMaxLabelDistance = 25.0;
	protected float m_fMinLabelDistance = 10.0;
	
	protected TNodeId w_iBodyBoneIndex;
	
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wPlayableIcon = ImageWidget.Cast(w.FindAnyWidget("PlayableIcon"));
		m_wPlayableLabel = TextWidget.Cast(w.FindAnyWidget("PlayableLabel"));
		m_wPanelLabel = PanelWidget.Cast(w.FindAnyWidget("PanelLabel"));

		m_PPI_GameModeComponent = PS_PPI_GameModeComponent.GetInstance();
		m_PPI_CharacterComponent = null;
		m_PPI_PlayerInfo = null;
	}

	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);

		if (m_PPI_PlayerInfo) {
			m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PPI_OnPlayerInfoChanged);
			m_PPI_PlayerInfo = null;
		};

		if (m_PPI_CharacterComponent) {
			m_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Remove(PPI_OnPlayerInfoIdChanged);
			m_PPI_CharacterComponent = null;
		};

		m_PPI_GameModeComponent = null;
	}
	
	void SetCharacter(PS_PlayableContainer playableComponent)
	{
		m_cPlayableComponent = playableComponent;
		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(playableComponent.GetPlayableComponentTemp().GetOwner());
		m_eChimeraCharacter = chimeraCharacter;
		
		if (m_PPI_PlayerInfo) {
			m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PPI_OnPlayerInfoChanged);
			m_PPI_PlayerInfo = null;
		};
		
		if (m_PPI_CharacterComponent) {
			m_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Remove(PPI_OnPlayerInfoIdChanged);
			m_PPI_CharacterComponent = null;
		};

		m_PPI_CharacterComponent = PS_PPI_CharacterComponent.GetInstance(m_eChimeraCharacter);
		if (m_PPI_CharacterComponent) {
			m_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Insert(PPI_OnPlayerInfoIdChanged);
			
			auto playerInfoId = m_PPI_CharacterComponent.GetPlayerInfoId();
			if (playerInfoId != -1) {
				m_PPI_PlayerInfo = m_PPI_GameModeComponent.GetPlayerInfo(playerInfoId);
				m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Insert(PPI_OnPlayerInfoChanged);
			};

			PPI_UpdateName();
		};
		
		w_iBodyBoneIndex = m_eChimeraCharacter.GetAnimation().GetBoneIndex("Spine3");
		SCR_Faction faction = SCR_Faction.Cast(chimeraCharacter.GetFaction());
		if (faction)
		{
			m_wPlayableIcon.SetColor(faction.GetOutlineFactionColor());
			//m_wPlayableLabel.SetColor(faction.GetOutlineFactionColor());
		}
	}
	
	void Update()
	{
		vector boneMat[4];
		m_eChimeraCharacter.GetAnimation().GetBoneMatrix(w_iBodyBoneIndex, boneMat);
		vector iconWorldPosition = m_eChimeraCharacter.GetOrigin() + boneMat[3];
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		float distanceToPlayable = vector.Distance(cameraPosition, iconWorldPosition);
		
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(iconWorldPosition, GetGame().GetWorld());
		
		if (screenPosition[2] < 0)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		if (distanceToPlayable > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		int playerId = playableManager.GetPlayerByPlayable(m_cPlayableComponent.GetRplId());
		if (playerId > 0)
		{
			if (!m_PPI_CharacterComponent) {
				string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
				if (playerName != "")
					m_wPlayableLabel.SetText(playerName);
			};
			
			if (distanceToPlayable > m_fMaxLabelDistance)
				m_wPanelLabel.SetOpacity(0.0);
			else
			{
				float opacityLabel = 1.0 - ((distanceToPlayable - m_fMinLabelDistance) / (m_fMaxLabelDistance - m_fMinLabelDistance));
				if (opacityLabel > 1.0) opacityLabel = 1.0;
				m_wPanelLabel.SetOpacity(opacityLabel);
			}
		} else {
			m_wPanelLabel.SetOpacity(0.0);
		}
		
		if (!m_bDead)
		{
			SCR_CharacterControllerComponent characterDamageComp = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
			
			if (!m_bWounded && characterDamageComp.IsUnconscious()) {
				m_wPlayableIcon.LoadImageFromSet(0, m_rIconImageSet, "Wounded");
				m_bWounded = true;
			}
			if (m_bWounded && !characterDamageComp.IsUnconscious()) {
				m_wPlayableIcon.LoadImageFromSet(0, m_rIconImageSet, "Player");
				m_bWounded = false;
			}
			
			if (characterDamageComp.IsDead()) {
				m_wPlayableIcon.LoadImageFromSet(0, m_rIconImageSet, "Dead");
				m_bDead = true;
			}
		}
		
		float distanceScale = 1.0 - ((distanceToPlayable - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance));
		
		float currentScale = distanceScale;
		if (currentScale > 1.0) currentScale = 1.0;
		float scaledIconSize = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * currentScale;
		float scaledIconOpacity = m_fMinIconOpacity + (m_fMaxIconOpacity - m_fMinIconOpacity) * currentScale;
		
		float LabelSizeX, LabelSizeY;
		m_wPanelLabel.GetScreenSize(LabelSizeX, LabelSizeY);
		float LabelSizeXD = GetGame().GetWorkspace().DPIUnscale(LabelSizeX);
		float LabelSizeYD = GetGame().GetWorkspace().DPIUnscale(LabelSizeY);
		
		if (m_bDead) scaledIconOpacity -= 0.4;
		m_wRoot.SetOpacity(scaledIconOpacity);
		FrameSlot.SetSize(m_wPlayableIcon, scaledIconSize, scaledIconSize);
		FrameSlot.SetPos(m_wPlayableIcon, -scaledIconSize/2, -scaledIconSize/2);
		FrameSlot.SetPos(m_wPanelLabel, -LabelSizeXD/2, -scaledIconSize + scaledIconSize/4);
		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);
	}

	protected event void PPI_OnPlayerInfoIdChanged(notnull PS_PPI_CharacterComponent characterComponent, int playerInfoId, int playerInfoIdOld) {
		if (m_PPI_PlayerInfo) {
			m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PPI_OnPlayerInfoChanged);
			m_PPI_PlayerInfo = null;
		};

		if (playerInfoId != -1) {
			m_PPI_PlayerInfo = m_PPI_GameModeComponent.GetPlayerInfo(playerInfoId);
			m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Insert(PPI_OnPlayerInfoChanged);
		};

		PPI_UpdateName();
	};

	protected event void PPI_OnPlayerInfoChanged(notnull PS_PPI_PlayerInfo playerInfo) {
		PPI_UpdateName();
	};

	protected event void PPI_UpdateName() {
		string playerName;
		if (m_PPI_PlayerInfo)
			playerName = m_PPI_PlayerInfo.GetPlayerName();
		else
			playerName = "AI";

		m_wPlayableLabel.SetText(playerName);
	};
}
