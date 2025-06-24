class PS_AlivePlayerSelector : SCR_ButtonBaseComponent
{
	// Const
	protected const ResourceName m_sImageSet = "{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset";
	protected ref Color m_DeathColor = Color.FromInt(0xFF2c2c2c);
	
	// Cache global
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	
	// Parameters
	protected PS_AlivePlayerGroup m_AliveGroup;
	protected PS_SpectatorMenu m_mSpectatorMenu;
	protected RplId m_iPlayableId;
	
	// Cache parameters
	protected PS_AlivePlayerList m_AlivePlayerList;
	protected PS_PlayableContainer m_PlayableContainer;
	protected SCR_CharacterDamageManagerComponent m_CharacterDamageManagerComponent;
	protected bool m_bDead;
	
	// Widgets
	protected ImageWidget m_wPlayerFactionColor;
	protected ImageWidget m_wUnitIcon;
	protected ImageWidget m_wDeadIcon;
	protected RichTextWidget m_wPlayerName;
	
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Widgets
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wDeadIcon = ImageWidget.Cast(w.FindAnyWidget("DeadIcon"));
		m_wPlayerName = RichTextWidget.Cast(w.FindAnyWidget("PlayerName"));
		m_wPlayerFactionColor = ImageWidget.Cast(w.FindAnyWidget("PlayerFactionColor"));
		
		// Cache global
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		GetGame().GetCallqueue().CallLater(AddOnClick, 0);
	}
	
	void AddOnClick()
	{
		m_OnClicked.Insert(AlivePlayerButtonClicked);
	}
	
	// Set parameters
	void SetPlayable(int playableId)
	{
		m_iPlayableId = playableId;
		
		// Cache parameters
		m_PlayableContainer = m_PlayableManager.GetPlayableById(playableId);
		
		// Temp
		Faction faction = m_PlayableContainer.GetFaction();
		
		// Initial setup
		m_wPlayerFactionColor.SetColor(faction.GetFactionColor());
		EDamageState damageState = m_PlayableContainer.GetDamageState();
		UpdateDammage(damageState);
		UpdatePlayer(m_PlayableManager.GetPlayerByPlayableRemembered(m_iPlayableId));
		UpdateShowDead(m_AlivePlayerList.IsShowDead());
		
		// Events
		m_PlayableContainer.GetOnPlayerChange().Insert(UpdatePlayerWrap);
		m_PlayableContainer.GetOnDamageStateChanged().Insert(UpdateDammage);
		m_PlayableContainer.GetOnUnregister().Insert(RemoveSelf);
	}
	
	void SetSpectatorMenu(PS_SpectatorMenu spectatorMenu)
	{
		m_mSpectatorMenu = spectatorMenu;
	}
	
	void SetAlivePlayerList(PS_AlivePlayerList alivePlayerList)
	{
		m_AlivePlayerList = alivePlayerList;
		m_AlivePlayerList.GetOnShowDead().Insert(UpdateShowDead);
	}
	
	void SetAliveGroup(PS_AlivePlayerGroup alivePlayerGroup)
	{
		m_AliveGroup = alivePlayerGroup;
	}
	
	// Updates
	void UpdateDammage(EDamageState state)
	{
		m_bDead = state == EDamageState.DESTROYED;
		if (m_bDead)
		{
			m_wRoot.SetVisible(m_AlivePlayerList.IsShowDead());
			m_AlivePlayerList.OnAliveDie(m_PlayableContainer);
			m_wUnitIcon.SetVisible(false);
			m_wDeadIcon.SetVisible(true);
			//m_wUnitIcon.LoadImageFromSet(0, m_sImageSet, "death");
			m_wDeadIcon.SetColor(m_DeathColor);
			m_wPlayerName.SetColor(m_DeathColor);
		}
		else
		{
			m_PlayableContainer.SetIconTo(m_wUnitIcon);
			m_wPlayerName.SetColor(Color.White);
		}
	}
	
	void UpdatePlayerWrap(int oldPlayerId, int playerId)
	{
		UpdatePlayer(m_PlayableManager.GetPlayerByPlayableRemembered(m_PlayableContainer.GetRplId()))
	}
	void UpdatePlayer(int playerId)
	{
		string playerName = m_PlayableManager.GetPlayerName(playerId);
		if (playerName == "") // No player
			playerName = m_PlayableContainer.GetName();
		m_wPlayerName.SetText(playerName);
	}
	
	void RemoveSelf()
	{
		m_wRoot.RemoveFromHierarchy();
		m_AliveGroup.OnAliveRemoved(m_PlayableContainer);
		m_AlivePlayerList.OnAliveRemoved(m_PlayableContainer);
	}
	
	void UpdateShowDead(bool showDead)
	{
		m_wRoot.SetVisible(showDead || !m_bDead);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (button == 1)
		{
			RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(m_iPlayableId));
			if (rplComponent)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(rplComponent.GetEntity());
				OpenContext(character);
			}
			return false;
		}
		if (button != 0)
			return false;
		
		AlivePlayerButtonClicked(this);
		return false;
	}
	// --------------------------------------------------------------------------------------------------------------------------------
	void OpenContext(SCR_ChimeraCharacter character)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return;
		
		PS_PlayableComponent playableComponent = character.PS_GetPlayable();
		
		int playerId = PS_PlayableManager.GetInstance().GetPlayerByPlayable(playableComponent.GetRplId());
		string playerName = PS_PlayableManager.GetInstance().GetPlayerName(playerId);
		if (playerName == "")
		{
			playerName = playableComponent.GetName();
		}
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(menu.GetRootWidget(), playerName);
		
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent.GetTarget())
			contextMenu.ActionAttachTo(character).Insert(OnActionAttachTo);
		else
			contextMenu.ActionDetachFrom(character).Insert(OnActionDetachFrom);
		contextMenu.ActionLookAt(character).Insert(OnActionLookAt);
		contextMenu.ActionFirstPersonView(character).Insert(OnActionFirstPersonView);
		contextMenu.ActionRespawnInPlace(playableComponent.GetId(), playerId);
		if (playerId > 0)
		{
			contextMenu.ActionDirectMessage(playerId);
			contextMenu.ActionKick(playerId);
			
			if (PS_PlayersHelper.IsAdminOrServer())
			{
				contextMenu.ActionGetArmaId(playerId);
			}
		}
	}
	void OnActionAttachTo(PS_ContextAction contextAction, PS_ContextActionDataCharacter contextActionDataCharacter)
	{
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		SCR_ChimeraCharacter character = contextActionDataCharacter.GetCharacter();
		attachComponent.AttachTo(character);
	}
	void OnActionDetachFrom(PS_ContextAction contextAction, PS_ContextActionDataCharacter contextActionDataCharacter)
	{
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		attachComponent.Detach();
	}
	void OnActionLookAt(PS_ContextAction contextAction, PS_ContextActionDataCharacter contextActionDataCharacter)
	{
		SCR_ChimeraCharacter character = contextActionDataCharacter.GetCharacter();
		PS_SpectatorLabel spectatorLabel = PS_SpectatorLabel.Cast(character.FindComponent(PS_SpectatorLabel));
		if (!spectatorLabel)
			return;
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!spectatorMenu)
			return;
		spectatorMenu.SetLookTarget(spectatorLabel.GetLabelIcon());
	}
	void OnActionFirstPersonView(PS_ContextAction contextAction, PS_ContextActionDataCharacter contextActionDataCharacter)
	{
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!spectatorMenu)
			return;
		
		spectatorMenu.SetLookTarget(null);
		attachComponent.Detach();
		
		SCR_ChimeraCharacter character = contextActionDataCharacter.GetCharacter();
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetCharacterEntity(character)
	}
	
	// -------------------- Buttons events --------------------
	void AlivePlayerButtonClicked(SCR_ButtonBaseComponent playerButton)
	{
		m_mSpectatorMenu.SetCameraCharacter(m_PlayableContainer.GetRplId());
	}
	
}