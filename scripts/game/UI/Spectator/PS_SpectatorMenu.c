modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	SpectatorMenu
}

class PS_SpectatorMenu: MenuBase
{
	static const ResourceName SPECTATOR_PING_ENTITY = "{142A242B49F28F6A}Prefabs/Spectator/SpectatorPingEntity.et";
	static const ResourceName SPECTATOR_PING_SOUND = "{C64C37176ED1AD1D}Sounds/Spectator/HonkSounds.acp";
	
	protected SCR_ChatPanel m_ChatPanel;
	
	InputManager m_InputManager;
	PS_GameModeCoop m_GameMode;
	PS_PlayableManager m_PlayableManager;
	
	static PS_SpectatorMenu s_SpectatorMenu;
	
	// Voice chat menu
	protected Widget m_wChat;
	protected Widget m_wVoiceChatList;
	protected Widget m_wOverlayFooter;
	protected Widget m_wEarlyAccessRoot;
	protected Widget m_wAlivePlayerList;
	protected Widget m_wSidesRatio;
	protected Widget m_wSidesRatioFrame;
	protected TextWidget m_wGameTimerText;
	protected PS_VoiceChatList m_hVoiceChatList;
	protected SCR_ButtonBaseComponent m_hVoiceChatListPinButton;
	protected PS_AlivePlayerList m_hAlivePlayerList;
	protected SCR_ButtonBaseComponent m_hAlivePlayerListPinButton;
	
	protected PS_SpectatorLabelIcon m_LookTarget;
	
	SCR_InputButtonComponent m_bNavigationSwitchSpectatorUI;

	protected ResourceName m_rPlayableIconPath = "{EE28CD1CBCAED99D}UI/Spectator/SpectatorPlayableIcon.layout";
	protected FrameWidget m_wIconsFrame;
	
	protected FrameWidget m_wMapFrame;
	
	protected SCR_MapEntity m_MapEntity;
	
	protected ref map<PS_SpectatorLabel, PS_SpectatorLabel> m_mIconsList = new map<PS_SpectatorLabel, PS_SpectatorLabel>();
	
	protected vector m_vLastPingPosition;
	
	
	protected PS_SpectatorLabelIcon m_SelectedLabel;
	protected vector m_vSelectedPosition;
	
	static void ResetTarget()
	{
		if (s_SpectatorMenu)
			s_SpectatorMenu.SetLookTarget(null);
	}
	
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu();
	}
	
	void SetLookTarget(PS_SpectatorLabelIcon target)
	{
		m_LookTarget = target;
	}
	
	void SetSelectedTarget(PS_SpectatorLabelIcon target)
	{
		if (m_SelectedLabel)
			m_SelectedLabel.SetSelected(false);
		m_SelectedLabel = target;
		if (m_SelectedLabel)
			m_SelectedLabel.SetSelected(true);
	}
	
	bool SetCameraCharacter(RplId rplId)
	{
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(rplId));
		if (!rplComponent)
			return false;
		IEntity characterEntity = rplComponent.GetEntity();
		if (m_GameMode.GetFriendliesSpectatorOnly())
		{
			PS_PlayableContainer playableContainer = m_PlayableManager.GetPlayableById(rplId);
			FactionKey playableFactionKey = playableContainer.GetFactionKey();
			FactionKey lastPlayerFaction = m_PlayableManager.GetPlayerFactionKeyRemembered(GetGame().GetPlayerController().GetPlayerId());
			if (playableFactionKey != lastPlayerFaction)
				return false;
		}
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			if (m_GameMode.GetFriendliesSpectatorOnly())
				camera.SetCharacterEntityMove(characterEntity);
			else
				camera.SetCharacterEntity(characterEntity);
		return true;
	}
	
	bool SetCameraMoveCharacter(RplId rplId)
	{
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(rplId));
		if (!rplComponent)
			return false;
		IEntity characterEntity = rplComponent.GetEntity();
		if (m_GameMode.GetFriendliesSpectatorOnly())
		{
			PS_PlayableContainer playableContainer = m_PlayableManager.GetPlayableById(rplId);
			FactionKey playableFactionKey = playableContainer.GetFactionKey();
			FactionKey lastPlayerFaction = m_PlayableManager.GetPlayerFactionKeyRemembered(GetGame().GetPlayerController().GetPlayerId());
			if (playableFactionKey != lastPlayerFaction)
				return false;
		}
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetCharacterEntityMove(characterEntity);
		return true;
	}
	
	void ReceivePing(int reporterId, vector position, RplId targetID)
	{
		SCR_NotificationsComponent.SendLocal(ENotification.EDITOR_PING_PLAYER, position, reporterId);
		SCR_UISoundEntity.SoundEvent("SOUND_LOBBY_HONK");
		
		m_vLastPingPosition = position;
		EntitySpawnParams params = new EntitySpawnParams();
		Math3D.MatrixIdentity3(params.Transform);
		params.Transform[3] = position;
		GetGame().SpawnEntityPrefabLocal(Resource.Load(SPECTATOR_PING_ENTITY), GetGame().GetWorld(), params);
	}
	
	override void OnMenuOpen()
	{
		s_SpectatorMenu = this;
		
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		m_GameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		
		m_wVoiceChatList = GetRootWidget().FindAnyWidget("VoiceChatFrame");
		m_hVoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatList.FindHandler(PS_VoiceChatList));
		m_hVoiceChatListPinButton = SCR_ButtonBaseComponent.Cast(m_wVoiceChatList.FindAnyWidget("PinButton").FindHandler(SCR_ButtonBaseComponent));
		
		m_wAlivePlayerList = GetRootWidget().FindAnyWidget("AlivePlayersList");
		m_hAlivePlayerList = PS_AlivePlayerList.Cast(m_wAlivePlayerList.FindHandler(PS_AlivePlayerList));
		m_hAlivePlayerListPinButton = SCR_ButtonBaseComponent.Cast(m_wAlivePlayerList.FindAnyWidget("PinButton").FindHandler(SCR_ButtonBaseComponent));
		
		m_hAlivePlayerList.SetSpectatorMenu(this);
		m_wOverlayFooter = GetRootWidget().FindAnyWidget("OverlayFooter");
		m_wEarlyAccessRoot = GetRootWidget().FindAnyWidget("EarlyAccessRoot");
		m_wIconsFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"));
		m_wMapFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("MapFrame"));
		m_wSidesRatioFrame = GetRootWidget().FindAnyWidget("SidesRatioFrame");
		m_wSidesRatio = GetRootWidget().FindAnyWidget("SidesRatio");
		m_wGameTimerText = TextWidget.Cast(GetRootWidget().FindAnyWidget("GameTimerText"));
		
		m_bNavigationSwitchSpectatorUI = SCR_InputButtonComponent.Cast(GetRootWidget().FindAnyWidget("NavigationSwitchSpectatorUI").FindHandler(SCR_InputButtonComponent));
		m_bNavigationSwitchSpectatorUI.m_OnClicked.Insert(Action_SwitchSpectatorUI);
		
		super.OnMenuOpen();
		InitChat();
		
		GetGame().GetCallqueue().CallLater(RoomSwitchToGlobal, 0);
	}
	
	void UpdateCursorTarget()
	{
		int xs, ys;
		WidgetManager.GetMousePos(xs, ys);
		
		
		float xr = GetGame().GetWorkspace().DPIUnscale(xs);
		float yr = GetGame().GetWorkspace().DPIUnscale(ys);
		vector outDir;
		vector origin = GetGame().GetWorkspace().ProjScreenToWorld(xr, yr, outDir, GetGame().GetWorld());
		TraceParam trace = new TraceParam();
		trace.Start = origin;
		trace.End = origin + outDir * 1000;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN; 
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		float traceCursor = GetGame().GetWorld().TraceMove(trace, null);
		m_vSelectedPosition = trace.Start + outDir * 1000 * traceCursor;
		
		// Check icon under cursor
		array<Widget> outWidgets = {};
		array<Widget> outWidgetsOut = {};
		WidgetManager.TraceWidgets(xs, ys, GetRootWidget(), outWidgets);
		foreach (Widget widget : outWidgets)
		{
			if (!(widget.GetFlags() & WidgetFlags.IGNORE_CURSOR))
				outWidgetsOut.Insert(widget)
		}
		PS_SpectatorLabelIcon spectatorLabelIcon; 
		if (outWidgetsOut.Count() > 0)
		{
			Widget widget = outWidgetsOut[0];
			while (!spectatorLabelIcon && widget)
			{
				spectatorLabelIcon = PS_SpectatorLabelIcon.Cast(widget.FindHandler(PS_SpectatorLabelIcon));
				widget = widget.GetParent();
			}
			if (spectatorLabelIcon)
			{
				SetSelectedTarget(spectatorLabelIcon);
				return;
			}
			SetSelectedTarget(null);
			return;
		}
		// Check physic trace
		else
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(trace.TraceEnt);
			if (character)
			{
				PS_SpectatorLabel spectatorLabel = PS_SpectatorLabel.Cast(character.FindComponent(PS_SpectatorLabel));
				if (spectatorLabel)
				{
					SetSelectedTarget(spectatorLabel.GetLabelIcon());
					return;
				}
			}
		}
		SetSelectedTarget(null);
	}
	
	void OpenContextClick()
	{
		if (!m_SelectedLabel)
		{
			OpenContext(null);
			return;
		}
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_SelectedLabel.GetEntity());
		OpenContext(character);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Context menu
	void OpenContext(SCR_ChimeraCharacter character)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || menu != this)
			return;
		
		if (!character)
		{
			array<Widget> outWidgets = {};
			array<Widget> outWidgetsOut = {};
			int xs, ys;
			WidgetManager.GetMousePos(xs, ys);
			WidgetManager.TraceWidgets(xs, ys, GetRootWidget(), outWidgets);
			foreach (Widget widget : outWidgets)
			{
				if (!(widget.GetFlags() & WidgetFlags.IGNORE_CURSOR))
					outWidgetsOut.Insert(widget)
			}
			if (outWidgetsOut.Count() > 0)
				return;
			if (!PS_PlayersHelper.IsAdminOrServer())
				return;
			
			PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(menu.GetRootWidget(), "");
			contextMenu.ActionCreatePrefab(m_vSelectedPosition);
			contextMenu.ActionCreateAdministrator(m_vSelectedPosition);
			
			return;
		}
		
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
			contextMenu.ActionKick(playerId);
			contextMenu.ActionDirectMessage(playerId);
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
		SetLookTarget(spectatorLabel.GetLabelIcon());
	}
	void OnActionFirstPersonView(PS_ContextAction contextAction, PS_ContextActionDataCharacter contextActionDataCharacter)
	{
		PS_AttachManualCameraObserverComponent attachComponent = PS_AttachManualCameraObserverComponent.s_Instance;
		if (!attachComponent)
			return;
		
		SetLookTarget(null);
		attachComponent.Detach();
		
		SCR_ChimeraCharacter character = contextActionDataCharacter.GetCharacter();
		PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			if (m_GameMode.GetFriendliesSpectatorOnly())
				camera.SetCharacterEntityMove(character);
			else
				camera.SetCharacterEntity(character);
	}
	
	void RoomSwitchToGlobal()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.MoveToVoNRoom(playerController.GetPlayerId(), "", "#PS-VoNRoom_Global");
	}
	
	void InitChat()
	{
		// Check if this menu has chat
		m_wChat = GetRootWidget().FindAnyWidget("ChatPanel");
		if (!m_wChat)
			return;
		
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChat.FindHandler(SCR_ChatPanel));
	}
	
	override void OnMenuInit()
	{
		super.OnMenuInit();
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			
		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			m_InputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			m_InputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, Action_SwitchSpectatorUI);
			m_InputManager.AddActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			m_InputManager.AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
			m_InputManager.AddActionListener("EditorLastNotificationTeleport", EActionTrigger.DOWN, Action_EditorLastNotificationTeleport);
#ifdef WORKBENCH
			m_InputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
			m_InputManager.AddActionListener("MouseLeft", EActionTrigger.UP, OpenContextClick);
		}
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			m_InputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OpenPauseMenu);
			m_InputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, ChatToggle);
			m_InputManager.RemoveActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.RemoveActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
			m_InputManager.RemoveActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, Action_SwitchSpectatorUI);
			m_InputManager.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			m_InputManager.RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
			m_InputManager.RemoveActionListener("EditorLastNotificationTeleport", EActionTrigger.DOWN, Action_EditorLastNotificationTeleport);
#ifdef WORKBENCH
			m_InputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, OpenPauseMenu);
#endif
			m_InputManager.RemoveActionListener("MouseLeft", EActionTrigger.UP, OpenContextClick);
		}
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			m_wGameTimerText.SetVisible(true);
			
			PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			float timeSeconds = gameModeCoop.GetElapsedTime() - gameModeCoop.GetGameStartTime()/1000;
			if (timeSeconds < 0 || gameModeCoop.GetGameStartTime() == 0)
				timeSeconds = 0;
			int seconds = Math.Mod(timeSeconds, 60);
			int minutes = (timeSeconds / 60);
			int hours = (minutes / 60);
			minutes = Math.Mod(minutes, 60);
			m_wGameTimerText.SetTextFormat("%1:%2:%3", hours.ToString(2), minutes.ToString(2), seconds.ToString(2));
		}
		else
			m_wGameTimerText.SetVisible(false);
		
		UpdateCursorTarget();
		
		if (m_GameMode.GetFriendliesSpectatorOnly())
		{
			PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
			if (!camera.GetCharacterEntity())
			{
				array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
				foreach (PS_PlayableContainer playable : playables)
				{
					if (playable.GetDamageState() != EDamageState.DESTROYED)
						SetCameraCharacter(playable.GetRplId());
				}
			}
		}
		
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		if (m_MapEntity && m_MapEntity.IsOpen())
			m_InputManager.ActivateContext("MapContext");
		
		UpdateIcons();
		
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		while (cursorWidget)
		{
			
			if (cursorWidget == m_wAlivePlayerList)
			{
				break;
			}
			
			if (cursorWidget == m_wVoiceChatList)
			{
				break;
			}
			
			cursorWidget = cursorWidget.GetParent();
		}
		
		float alivePlayerListX = FrameSlot.GetPosX(m_wAlivePlayerList);
		float voiceChatListX = FrameSlot.GetPosX(m_wVoiceChatList);
		if (cursorWidget == m_wAlivePlayerList || m_hAlivePlayerListPinButton.IsToggled())
		{
			alivePlayerListX += tDelta * 1200.0;
			if (alivePlayerListX > 0)
				alivePlayerListX = 0;
		}
		else
		{
			alivePlayerListX -= tDelta * 1200.0;
			if (alivePlayerListX < -315)
				alivePlayerListX = -315;
		}
		FrameSlot.SetPosX(m_wAlivePlayerList, alivePlayerListX);
		
		if (cursorWidget == m_wVoiceChatList || m_hVoiceChatListPinButton.IsToggled())
		{
			voiceChatListX -= tDelta * 1200.0;
			if (voiceChatListX < -320)
				voiceChatListX = -320;
		}
		else
		{
			voiceChatListX += tDelta * 1200.0;
			if (voiceChatListX > -5)
				voiceChatListX = -5;
		}
		FrameSlot.SetPosX(m_wVoiceChatList, voiceChatListX);
		
		/* VoN Magic
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.VoNPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin() - vector.Up * 1.7;
		*/
		
		//GetGame().Getm_InputManager().ActivateContext("SpectatorContext", 1);
		
		if (m_LookTarget)
		{
			PS_ManualCameraSpectator camera = PS_ManualCameraSpectator.Cast(GetGame().GetCameraManager().CurrentCamera());
			if (camera)
			{
				vector cameraOrigin = camera.GetOrigin();
				vector characterOrigin = m_LookTarget.GetWorldPosition();
				vector rotation = characterOrigin - cameraOrigin;
				float quat[4];
				rotation.VectorToAngles().QuatFromAngles(quat);
				
				float currentQuat[4];
				vector currentMat[4];
				camera.GetTransform(currentMat);
				Math3D.MatrixToQuat(currentMat, currentQuat);
				Math3D.QuatLerp(quat, currentQuat, quat, 10*tDelta);
				
				vector mat[4];
				Math3D.QuatToMatrix(quat, mat);
				mat[3] = cameraOrigin;
				camera.SetTransform(mat);
			}
		}
	}
	
	void UpdateIcons()
	{
		PS_SpectatorLabelsManager spectatorLabelsManager = PS_SpectatorLabelsManager.GetInstance();
		foreach (PS_SpectatorLabel spectatorLabel : spectatorLabelsManager.m_aSpectatorLabels)
		{
			if (!m_mIconsList.Contains(spectatorLabel))
			{
				spectatorLabel.CreateLabel(m_wIconsFrame);
				m_mIconsList.Insert(spectatorLabel, spectatorLabel);
			}
		}
		foreach (PS_SpectatorLabel spectatorLabel, PS_SpectatorLabel spectatorLabel2 : m_mIconsList)
		{
			if (!spectatorLabel)
				continue;
			spectatorLabel.UpdateLabel();
			spectatorLabel.UpdateMarker();
		}
	}
	
	void ChatToggle()
	{
		if (!m_ChatPanel)
			return;
		
		if (!m_ChatPanel.IsOpen())
			GetGame().GetCallqueue().CallLater(ChatWrap, 0);
	}
	void ChatWrap()
	{
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	void OpenChat(string msg)
	{
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 0, false, msg);
	}
	void OpenChatWrap(string msg)
	{
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		EditBoxWidget editBoxWidget = m_ChatPanel.PS_GetWidgets().m_MessageEditBox;
		editBoxWidget.SetText(msg);
	}
	
	void Action_LobbyVoNOn()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNRadioEnable();
	}
	
	void Action_LobbyVoNOff()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.LobbyVoNDisable();
	}
	
	void Action_SwitchSpectatorUI()
	{
		if (m_wVoiceChatList.IsVisible())
		{
			//m_wChat.SetVisible(false);
			m_wVoiceChatList.SetVisible(false);
			m_wOverlayFooter.SetVisible(false);
			m_wEarlyAccessRoot.SetVisible(false);
			m_wAlivePlayerList.SetVisible(false);
			m_wIconsFrame.SetVisible(false);
			m_wSidesRatioFrame.SetVisible(false);
		} else {
			//m_wChat.SetVisible(true);
			m_wVoiceChatList.SetVisible(true);
			m_wOverlayFooter.SetVisible(true);
			m_wEarlyAccessRoot.SetVisible(true);
			m_wAlivePlayerList.SetVisible(true);
			m_wIconsFrame.SetVisible(true);
			m_wSidesRatioFrame.SetVisible(true);
		}
	}
	
	bool IsUIVisible()
	{
		return m_wVoiceChatList.IsVisible();
	}
	
	void Action_ToggleMap()
	{
		if (!m_MapEntity.IsOpen()) OpenMap();
		else CloseMap();
	}
	
	void Action_EditorLastNotificationTeleport()
	{
		if (m_vLastPingPosition != vector.Zero)
			MoveCamera(m_vLastPingPosition);
	}
	
	void Action_ManualCameraTeleport()
	{
		MoveCamera(GetCursorWorldPos());
	}
	
	void MoveCamera(vector worldPosition)
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
			if (teleportComponent)
			{
				teleportComponent.TeleportCamera(worldPosition, true, false);
			}
		}
	}
	
	vector GetCursorWorldPos()
	{
		ArmaReforgerScripted game = GetGame();
		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		
		vector worldPos = "0 0 0";
		
		// If map is open return map cursor world position
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			float worldX, worldY;
			mapEntity.GetMapCursorWorldPosition(worldX, worldY);
			worldPos[0] = worldX;
			worldPos[2] = worldY;
			worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
			
			return worldPos;
		}
		
		
		vector cursorPos = GetCursorPos();
		vector outDir;
		vector startPos = workspace.ProjScreenToWorld(cursorPos[0], cursorPos[1], outDir, world, -1);
		outDir *= 1000.0;
	
		autoptr TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = startPos + outDir;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		
		float traceDis = world.TraceMove(trace, null);
		worldPos = startPos + outDir * traceDis;
		
		return worldPos;
	}
	
	vector GetCursorPos()
	{
		int cursorX, cursorY;
		if (m_InputManager && m_InputManager.IsUsingMouseAndKeyboard())
		{
			WidgetManager.GetMousePos(cursorX, cursorY);
			return Vector(GetGame().GetWorkspace().DPIUnscale(cursorX), GetGame().GetWorkspace().DPIUnscale(cursorY), 0);
		} else {
			return Vector(GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetWidth()/2.0), GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetHeight()/2.0), 0);
		}
	}
	
	void OpenMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(false);
		
		m_wMapFrame.SetVisible(true);
		
		// WUT?
		SCR_WidgetHelper.RemoveAllChildren(m_wMapFrame.FindAnyWidget("ToolMenuHoriz"));
		
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configComp.GetEditorMapConfig(), m_wMapFrame); // MAP MADED BY RETARD, so reload it twice
		mapConfigFullscreen.MapEntityMode = EMapEntityMode.PLAIN;
		m_MapEntity.OpenMap(mapConfigFullscreen);
		m_MapEntity.CloseMap();
		mapConfigFullscreen.MapEntityMode = EMapEntityMode.FULLSCREEN;
		m_MapEntity.OpenMap(mapConfigFullscreen);
	}
	
	void CloseMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(true);
		
		m_wMapFrame.SetVisible(false);
		
		m_MapEntity.CloseMap();
	}
}