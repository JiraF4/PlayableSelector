class PS_VehicleSelector : SCR_ButtonComponent
{
	// Const
	protected ResourceName m_sUIWrapper = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected PlayerManager m_PlayerManager;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iCurrentPlayerId;
	
	// Widgets
	protected ImageWidget m_wVehicleFactionColor;
	protected ImageWidget m_wUnitIcon;
	protected TextWidget m_wVehicleClassName;
	protected ImageWidget m_wLockedIcon;
	
	// Parameters
	protected PS_CoopLobby m_CoopLobby;
	protected PS_RolesGroup m_RolesGroup;
	protected PS_PlayableVehicleContainer m_PlayableVehicleContainer;
	
	// Cache parameters
	protected SCR_Faction m_Faction;
	protected FactionKey m_sFactionKey;
	
	protected PS_MembersCounter m_MembersCounter;
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		
		// Widgets
		m_wVehicleFactionColor = ImageWidget.Cast(w.FindAnyWidget("VehicleFactionColor"));
		m_wUnitIcon = ImageWidget.Cast(w.FindAnyWidget("UnitIcon"));
		m_wVehicleClassName = TextWidget.Cast(w.FindAnyWidget("VehicleClassName"));
		m_wLockedIcon = ImageWidget.Cast(w.FindAnyWidget("LockedIcon"));
		
		// Buttons
		//m_OnClicked.Insert(OnClicked);
		m_OnHover.Insert(OnHover);
		m_OnHoverLeave.Insert(OnHoverLeave);
	}
	
	override void HandlerDeattached(Widget w)
	{
		
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetLobbyMenu(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	void SetRolesGroup(PS_RolesGroup rolesGroup)
	{
		m_RolesGroup = rolesGroup;
	}
	
	void SetMembersCounter(PS_MembersCounter membersCounter)
	{
		m_MembersCounter = membersCounter;
	}
	
	void SetVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		m_PlayableVehicleContainer = playableVehicleContainer;
		
		// Temp
		m_Faction = m_PlayableVehicleContainer.GetFaction();
		m_sFactionKey = m_Faction.GetFactionKey();
		
		// Initial setup
		m_wVehicleFactionColor.SetColor(m_Faction.GetFactionColor());
		
		ItemPreviewManagerEntity previewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		IEntity entity = previewManager.ResolvePreviewEntityForPrefab(m_PlayableVehicleContainer.m_sPrefabName);
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(entity.FindComponent(SCR_EditableVehicleComponent));
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		m_wUnitIcon.LoadImageTexture(0, uIInfo.GetIconPath());
		m_wVehicleClassName.SetText(uIInfo.GetName());
		
		// Events
		playableVehicleContainer.GetOnLockChange().Insert(OnLockChange);
		OnLockChange(playableVehicleContainer.GetLock());
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void OnLockChange(bool lock)
	{
		if (lock) {
			m_wUnitIcon.SetColorInt(ARGB(255, 32, 32, 32));
			m_wLockedIcon.SetVisible(1);
			m_MembersCounter.Add(-1);
		} else {
			m_wUnitIcon.SetColorInt(ARGB(255, 255, 255, 255));
			m_wLockedIcon.SetVisible(0);
			m_MembersCounter.Add(1);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update character
	void RemoveSelf()
	{
		m_wRoot.RemoveFromHierarchy();
		//m_RolesGroup.OnVehicleRemoved(m_PlayableVehicleContainer);
		//m_CoopLobby.OnVehicleRemoved(m_PlayableVehicleContainer);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (button == 1)
		{
			OpenContext();
			return false;
		}
		if (button != 0)
			return false;
		
		OnClicked(this);
		return false;
	}
	void OpenContext()
	{
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(m_CoopLobby.GetRootWidget(), "");
		contextMenu.ActionOpenInventory(m_PlayableVehicleContainer.GetRplId()).Insert(OnActionOpenInventory);
		
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			bool locked = m_PlayableVehicleContainer.GetLock();
			if (locked)
				contextMenu.ActionUnlock(m_PlayableVehicleContainer.GetRplId()).Insert(UnlockVehicle);
			else
				contextMenu.ActionLock(m_PlayableVehicleContainer.GetRplId()).Insert(LockVehicle);
		}
	}
	void UnlockVehicle(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		m_PlayableControllerComponent.SetPlayableVehicleLocked(contextActionDataPlayable.GetPlayableId(), 0);
	}
	void LockVehicle(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		m_PlayableControllerComponent.SetPlayableVehicleLocked(contextActionDataPlayable.GetPlayableId(), 1);
	}
	void OnActionOpenInventory(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		m_CoopLobby.SetPreviewPlayableVehicle(m_PlayableVehicleContainer, true);
	}
	void OnClicked(SCR_ButtonBaseComponent button)
	{
		m_CoopLobby.SetPreviewPlayableVehicle(m_PlayableVehicleContainer, true);
	}
	
	void OnHover()
	{
		m_CoopLobby.SetPreviewPlayableVehicle(m_PlayableVehicleContainer, false);
	}
	
	void OnHoverLeave()
	{
		m_CoopLobby.SetPreviewPlayable(RplId.Invalid(), false);
	}
}




















