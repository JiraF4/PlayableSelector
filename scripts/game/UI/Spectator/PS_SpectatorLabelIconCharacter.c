class PS_SpectatorLabelIconCharacter : PS_SpectatorLabelIcon
{
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected SCR_CharacterControllerComponent m_ControllerComponent;
	protected PS_PlayableComponent m_cPlayableComponent;
	protected SCR_EditableCharacterComponent m_EditableCharacterComponent;
	
	protected PS_PPI_GameModeComponent m_PPI_GameModeComponent;
	protected PS_PPI_CharacterComponent m_PPI_CharacterComponent;
	protected PS_PPI_PlayerInfo m_PPI_PlayerInfo;
	
	protected ImageWidget m_wSpectatorLabelIconBackground;
	protected ImageWidget m_wSpectatorLabelIconCircle;
	protected ImageWidget m_wSpectatorLabelIconWounded;
	protected ImageWidget m_wSpectatorLabelIconCircleSmall;
	protected OverlayWidget m_wOverlayCircle;
	
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	protected PS_PlayableManager m_PlayableManager;
	
	protected ref Color m_cDeadColor = Color.Gray;
	
	protected PS_EventHandlerComponent m_LabelEventHandler;
	
	protected float m_fClickIgnoreTime;
	
	override void HandlerAttached(Widget w)
	{
		m_wSpectatorLabelIconBackground = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconBackground"));
		m_wSpectatorLabelIconCircle = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconCircle"));
		m_wSpectatorLabelIconWounded = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconWounded"));
		m_wSpectatorLabelIconCircleSmall = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconCircleSmall"));
		m_wOverlayCircle = OverlayWidget.Cast(w.FindAnyWidget("OverlayCircle"));
		
		m_PlayableManager = PS_PlayableManager.GetInstance();

		m_PPI_GameModeComponent = PS_PPI_GameModeComponent.GetInstance();
		m_PPI_CharacterComponent = null;
		m_PPI_PlayerInfo = null;
		
		super.HandlerAttached(w);
		
		
		m_LabelEventHandler = PS_EventHandlerComponent.Cast(m_wLabelButton.FindHandler(PS_EventHandlerComponent));
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetFriendliesSpectatorOnly())
		{
			return;
		}
		
		m_LabelEventHandler.GetOnDoubleClick().Insert(AttachCamera);
		m_LabelEventHandler.GetOnClick().Insert(LookCamera);
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
	
	void LookCamera(Widget w, int x, int y, int button)
	{
		GetGame().GetCallqueue().CallLater(LateLook, 50, false, w, x, y, button);
	}
	
	void LateLook(Widget w, int x, int y, int button)
	{
		if (m_fClickIgnoreTime > GetGame().GetWorld().GetWorldTime())
		{
			return;
		}
		
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!spectatorMenu)
			return;
		
		if (button == 1)
		{
			spectatorMenu.SetLookTarget(this);
		}
		
		if (button == 0)
		{
			if (attachComponent.GetTarget() != m_eChimeraCharacter)
			{
				attachComponent.AttachTo(m_eChimeraCharacter);
			}
			else
				attachComponent.Detach();
		}
	}
	
	void AttachCamera(Widget w, int x, int y, int button)
	{
		m_fClickIgnoreTime = GetGame().GetWorld().GetWorldTime() + 400;
		
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!spectatorMenu)
			return;
		
		if (button == 1)
		{
			spectatorMenu.SetLookTarget(null);
			attachComponent.Detach();
			
			PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
			if (camera)
				camera.SetCharacterEntity(m_eChimeraCharacter)
		}
		
		if (button == 0)
		{
			if (attachComponent.GetTarget() != m_eChimeraCharacter)
			{
				SCR_TeleportToCursorManualCameraComponent teleportToCursorManualCameraComponent = SCR_TeleportToCursorManualCameraComponent.s_PS_Instance;
				if (teleportToCursorManualCameraComponent)
				{
					teleportToCursorManualCameraComponent.TeleportCamera(m_eChimeraCharacter.GetOrigin(), false, false, false, false, 2);
				}
				attachComponent.AttachTo(m_eChimeraCharacter);
			}
			else
				attachComponent.Detach();
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	override void SetEntity(IEntity entity, string boneName)
	{
		super.SetEntity(entity, boneName);
		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(entity);
		m_cPlayableComponent = PS_PlayableComponent.Cast(m_eChimeraCharacter.FindComponent(PS_PlayableComponent));
		m_ControllerComponent = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
		m_EditableCharacterComponent = SCR_EditableCharacterComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_EditableCharacterComponent));
		
		if (m_PPI_PlayerInfo) {
			m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Remove(PPI_OnPlayerInfoChanged);
			m_PPI_PlayerInfo = null;
		};
		
		if (m_PPI_CharacterComponent) {
			m_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Remove(PPI_OnPlayerInfoIdChanged);
			m_PPI_CharacterComponent = null;
		};

		m_PPI_CharacterComponent = PS_PPI_CharacterComponent.GetInstance(entity);
		if (m_PPI_CharacterComponent) {
			m_PPI_CharacterComponent.GetOnPlayerInfoIdChanged().Insert(PPI_OnPlayerInfoIdChanged);
			
			auto playerInfoId = m_PPI_CharacterComponent.GetPlayerInfoId();
			if (playerInfoId != -1) {
				m_PPI_PlayerInfo = m_PPI_GameModeComponent.GetPlayerInfo(playerInfoId);
				m_PPI_PlayerInfo.GetOnPlayerInfoChanged().Insert(PPI_OnPlayerInfoChanged);
			};

			PPI_UpdateName();
		};
		
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop.GetFriendliesSpectatorOnly())
		{
			if (m_cPlayableComponent.GetFactionKey() != m_PlayableManager.GetPlayerFactionKeyRemembered(GetGame().GetPlayerController().GetPlayerId()))
			{
				m_wRoot.SetVisible(false);
				return;
			}
		}
		
		SCR_Faction faction = SCR_Faction.Cast(m_eChimeraCharacter.GetFaction());
		if (faction)
		{
			m_wSpectatorLabelIcon.SetColor(faction.GetOutlineFactionColor());
			m_wSpectatorLabelIconBackground.SetColor(faction.GetFactionColor());
			m_wSpectatorLabelIconCircle.SetColor(faction.GetOutlineFactionColor());
			m_wSpectatorLabelIconCircleSmall.SetColor(faction.GetFactionColor());
			
			m_cDeadColor = PS_ColorHelper.DesaturateColor(faction.GetFactionColor(), 0.75);
			m_cDeadColor = PS_ColorHelper.ChangeLightColor(m_cDeadColor, 0.9);
		}
		
		SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
		uiInfo.SetIconTo(m_wSpectatorLabelIcon);
	}
	
	override void UpdateLabel()
	{
		if (m_cPlayableComponent && !m_PPI_CharacterComponent)
		{
			int playerId = m_PlayableManager.GetPlayerByPlayableRemembered(m_cPlayableComponent.GetRplId());
			if (playerId > 0)
			{
				string playerName = m_PlayableManager.GetPlayerName(playerId);
				if (playerName != "")
					m_wSpectatorLabelText.SetText(playerName);
			} else {
				m_wSpectatorLabelText.SetText(m_cPlayableComponent.GetName());
			}
		}
		
		if (m_fDistanceToIcon > 120 && !m_bSelected)
		{
			m_wOverlayCircle.SetVisible(false);
			m_wSpectatorLabelIcon.SetVisible(false);
			m_wSpectatorLabelIconSelected.SetVisible(false);
		}
		else
		{
			m_wOverlayCircle.SetVisible(true);
			m_wSpectatorLabelIcon.SetVisible(true);
			m_wSpectatorLabelIconSelected.SetVisible(true);
		}
		
		if (!m_bDead)
		{
			if (!m_bWounded && m_ControllerComponent.IsUnconscious()) {
				//m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Wounded");
				m_wSpectatorLabelIconWounded.SetVisible(true);
				m_bWounded = true;
			}
			if (m_bWounded && !m_ControllerComponent.IsUnconscious()) {
				SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
				uiInfo.SetIconTo(m_wSpectatorLabelIcon);
				m_wSpectatorLabelIconWounded.SetVisible(false);
				m_bWounded = false;
			}
			
			if (m_ControllerComponent.IsDead()) {
				//m_wSpectatorLabelIcon.LoadImageFromSet(0, m_rIconImageSet, "Dead");
				m_wSpectatorLabelIcon.SetOpacity(0.9);
				m_wSpectatorLabelBackground.SetOpacity(0.9);
				m_wSpectatorLabelIconBackground.SetColor(m_cDeadColor);
				m_wSpectatorLabelIconCircleSmall.SetColor(m_cDeadColor);
				m_wSpectatorLabelIconBackground.SetOpacity(0.2);
				m_wSpectatorLabelIconWounded.SetVisible(false);
				//m_wSpectatorLabelIconCircleSmall.SetColor(Color.Gray);
				m_wSpectatorLabelIconCircleSmall.SetOpacity(0.95);
				m_bDead = true;
			}
		}
		else 
		{
			m_wRoot.SetOpacity(0.6);
		}
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

		m_wSpectatorLabelText.SetText(playerName);
	};
}