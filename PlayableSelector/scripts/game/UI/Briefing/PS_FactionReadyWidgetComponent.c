class PS_FactionReadyWidgetComponent : PS_HideableButton
{
	ResourceName m_sImageSet = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	
	FactionManager m_FactionManager;
	PS_PlayableManager m_PlayableManager;
	
	PlayerController m_CurrentPlayerController;
	PS_PlayableControllerComponent m_CurrentPlayableControllerComponent;
	int m_iCurrentPlayerId;
	FactionKey m_sCurrentFactionKey;
	
	PS_FactionReadyListWidgetComponent m_FactionReadyListWidgetComponent;
	FactionKey m_sFactionKey;
	SCR_Faction m_faction;
	
	ImageWidget m_wBackgroundFaction;
	TextWidget m_wFactionName;
	TextWidget m_wCommanderNames;
	int m_iFactionReady;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_FactionManager = GetGame().GetFactionManager();
		m_PlayableManager = PS_PlayableManager.GetInstance();
		
		m_wBackgroundFaction = ImageWidget.Cast(w.FindAnyWidget("BackgroundFaction"));
		m_wFactionName = TextWidget.Cast(w.FindAnyWidget("FactionName"));
		m_wCommanderNames = TextWidget.Cast(w.FindAnyWidget("CommanderNames"));
		
		m_wButtonHandler.m_OnClicked.Insert(OnFactionClicked);
		
		m_CurrentPlayerController = GetGame().GetPlayerController();
		m_iCurrentPlayerId = m_CurrentPlayerController.GetPlayerId();
		m_sCurrentFactionKey = m_PlayableManager.GetPlayerFactionKey(m_iCurrentPlayerId);
		m_CurrentPlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_CurrentPlayerController.FindComponent(PS_PlayableControllerComponent));
		
		m_PlayableManager.m_eFactionReadyChanged.Insert(UpdateFaction);
		m_PlayableManager.m_eOnPlayerPlayableChange.Insert(OnPlayerPlayableChanged);
		// Keep the commander nickname current when the top slot's occupant joins/leaves: a held-slot reconnect is a
		// connect (slot may not "change"), and a disconnect of the commander promotes the next connected slot. Both
		// invokers fire on clients via Broadcast RPCs.
		m_PlayableManager.GetOnPlayerConnected().Insert(OnPlayerConnectedRefresh);
		m_PlayableManager.GetOnPlayerDisconnected().Insert(OnPlayerDisconnectedRefresh);

		m_wButton.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	void Init(PS_FactionReadyListWidgetComponent factionReadyListWidgetComponent, FactionKey factionKey)
	{
		m_FactionReadyListWidgetComponent = factionReadyListWidgetComponent;
		m_sFactionKey = factionKey;
		
		m_faction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));
		
		if (m_faction)
		{
			// Primary: faction flag texture; fallback to UIInfo icon (used by some custom factions)
			ResourceName flagTex = m_faction.GetFactionFlag();
			if (flagTex == "")
			{
				UIInfo uiInfo = m_faction.GetUIInfo();
				if (uiInfo)
					flagTex = uiInfo.GetIconPath();
				Print("[PS_FactionReady] GetFactionFlag() was empty, trying GetIconPath(): " + flagTex);
			}
			if (flagTex != "")
			{
				m_wBackgroundFaction.LoadImageTexture(0, flagTex);
				Print("[PS_FactionReady] Loaded flag: " + flagTex + " for faction " + m_sFactionKey);
			}
			else
				Print("[PS_FactionReady] WARNING: No flag texture found for faction " + m_sFactionKey);
			m_wBackgroundFaction.SetColor(Color.White);
			m_wBackgroundFaction.SetVisible(true);
			
			// Faction commander nickname = the FIRST occupied+connected playable slot of this faction (the top
			// slot). Kept current on slot/connect/disconnect changes via RefreshCommander. The old code set this
			// once here to the first player in the PlayerManager LIST (not slot order) and never refreshed it.
			UpdateCommanderName();
		}
		
		UpdateFaction(m_sFactionKey, m_PlayableManager.GetFactionReady(m_sFactionKey));
		// Defer commander/button refresh to end of frame: during an RPC burst (SetPlayerPlayable
		// + SetPlayerFactionKey arriving in the same frame), FillFactions destroys old widgets and
		// recreates them — synchronous calls here would read stale slot state. ScheduleRefresh coalesces
		// to one deferred call after all RPCs have been processed.
		ScheduleRefresh();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateButtonVisibility()
	{
		if (m_sFactionKey != m_sCurrentFactionKey)
		{
			m_wButton.SetVisible(false);
			return;
		}
		m_wButton.SetVisible(m_PlayableManager.IsPlayerFactionCommander(m_iCurrentPlayerId));
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateCommanderNames()
	{
		if (!m_wCommanderNames)
			return;
		
		array<int> commanderIds = {};
		m_PlayableManager.GetFactionCommanders(m_sFactionKey, commanderIds);
		
		if (commanderIds.IsEmpty())
		{
			m_wCommanderNames.SetText("");
			m_wCommanderNames.SetVisible(false);
			return;
		}
		
		// Use the raw engine name (Steam/Xbox) instead of the cached name from
		// PS_PlayableManager, which may already contain a squad-prefix injected by
		// PodvalPatches or a similar addon (e.g. "Atlas Red 1 NickName").
		PlayerManager pm = GetGame().GetPlayerManager();
		array<string> nameLines = {};
		foreach (int commanderId : commanderIds)
		{
			string playerName = "";
			if (pm)
				playerName = pm.GetPlayerName(commanderId);
			if (playerName == "")
				playerName = m_PlayableManager.GetPlayerName(commanderId);
			if (playerName == "")
				continue;
			
			string strippedName = StripMarkup(playerName);
			nameLines.Insert(strippedName);
		}
		
		string result = "";
		for (int i = 0; i < nameLines.Count(); i++)
		{
			if (i > 0)
				result += "\n";
			result += nameLines[i];
		}
		m_wCommanderNames.SetText(result);
		m_wCommanderNames.SetVisible(!nameLines.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	// Set m_wFactionName to the faction COMMANDER = the first occupied+connected playable slot of this faction.
	// GetFactionCommanders walks GetPlayablesSorted() and returns exactly that top slot, so this is always the
	// "highest" playable, recomputed live (replaces the old once-in-Init "first player in the PlayerManager list").
	void UpdateCommanderName()
	{
		if (!m_wFactionName)
			return;
		string nick = "";
		array<int> commanderIds = {};
		m_PlayableManager.GetFactionCommanders(m_sFactionKey, commanderIds);
		if (!commanderIds.IsEmpty())
		{
			int topId = commanderIds[0];
			// Raw engine name first (avoids a squad-prefix some addons inject into the cached name).
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
				nick = pm.GetPlayerName(topId);
			if (nick == "")
				nick = m_PlayableManager.GetPlayerName(topId);
		}
		if (nick != "")
		{
			m_wFactionName.SetText(StripMarkup(nick));
			m_wFactionName.SetColor(Color.White);
			m_wFactionName.SetVisible(true);
		}
		else
		{
			m_wFactionName.SetText("");
			m_wFactionName.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Re-evaluate everything that depends on slot occupancy: the commander nickname, the commander list, and the
	// ready-button visibility. Deferred + coalesced (ScheduleRefresh) so it runs once per frame and AFTER the
	// player-manager connection state settles on a disconnect (the leaver can still read connected at event time).
	void RefreshCommander()
	{
		UpdateButtonVisibility();
		UpdateCommanderName();
		UpdateCommanderNames();
	}
	void ScheduleRefresh()
	{
		GetGame().GetCallqueue().Remove(RefreshCommander);
		GetGame().GetCallqueue().CallLater(RefreshCommander, 0, false);
	}
	void OnPlayerConnectedRefresh(int playerId)
	{
		ScheduleRefresh();
	}
	void OnPlayerDisconnectedRefresh(int playerId, KickCauseCode cause, int timeout)
	{
		ScheduleRefresh();
	}

	//------------------------------------------------------------------------------------------------
	static string StripMarkup(string value)
	{
		if (!value.Contains("<"))
			return value;
		string result = "";
		bool inTag = false;
		int len = value.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = value.Get(i);
			if (ch == "<")
				inTag = true;
			else if (ch == ">")
				inTag = false;
			else if (!inTag)
				result += ch;
		}
		return result;
	}
	
	//------------------------------------------------------------------------------------------------
	void OnFactionClicked(SCR_ButtonBaseComponent button)
	{
		if (m_sFactionKey != m_sCurrentFactionKey)
			return;
		
		if (!m_PlayableManager.IsPlayerFactionCommander(m_iCurrentPlayerId))
			return;
		
		if (m_iFactionReady == 0)
			m_CurrentPlayableControllerComponent.SetFactionReady(m_sFactionKey, 1);
		else
			m_CurrentPlayableControllerComponent.SetFactionReady(m_sFactionKey, 0);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnPlayerPlayableChanged(int playerId, RplId playableId)
	{
		if (m_sFactionKey == "")
			return;

		// Recompute unconditionally: a slot change for ANY player can change THIS faction's top slot - including
		// the player who just LEFT it (whose faction key is now "" and would fail a same-faction filter). Manual
		// placement / deselect / reconnect-restore all arrive here. Cheap, and only on the briefing screen.
		ScheduleRefresh();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateFaction(FactionKey factionKey, int readyValue)
	{
		if (factionKey != m_sFactionKey)
			return;
		
		m_iFactionReady = readyValue;
		if (m_iFactionReady == 1)
			m_wImage.LoadImageFromSet(0, m_sImageSet, "check");
		else
			m_wImage.LoadImageFromSet(0, m_sImageSet, "disable");
	}
	
	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(RefreshCommander); // drop any pending deferred refresh
		if (m_PlayableManager)
		{
			m_PlayableManager.m_eFactionReadyChanged.Remove(UpdateFaction);
			m_PlayableManager.m_eOnPlayerPlayableChange.Remove(OnPlayerPlayableChanged);
			m_PlayableManager.GetOnPlayerConnected().Remove(OnPlayerConnectedRefresh);
			m_PlayableManager.GetOnPlayerDisconnected().Remove(OnPlayerDisconnectedRefresh);
		}
	}
}
