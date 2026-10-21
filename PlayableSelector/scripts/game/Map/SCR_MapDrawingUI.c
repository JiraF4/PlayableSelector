//! Null guards for the map drawing tool on screens that share its tool-menu entry.
//!
//! SCR_MapDrawingUI.UpdateLineCount and SetDrawMode dereference m_ToolMenuEntry.m_ButtonComp
//! unconditionally, but that button is created by SCR_MapToolMenuUI.PopulateToolMenu and dies
//! with the screen it was created on. This lobby hosts four fullscreen maps over one
//! SCR_MapEntity, so the entry outlives its widget. It is reached from UpdateLineCount — a line
//! cached by a previous session is rebuilt on the next map open — and from SetDrawMode, which
//! also runs from OnMapClose after the tool menu has already dropped the button.

modded class SCR_MapDrawingUI
{
	protected SCR_MapToolMenuUI PS_GetToolMenu()
	{
		if (!m_MapEntity)
			return null;

		return SCR_MapToolMenuUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapToolMenuUI));
	}

	protected void PS_RestoreToolButton()
	{
		if (!m_ToolMenuEntry || m_ToolMenuEntry.m_ButtonComp)
			return;

		SCR_MapToolMenuUI toolMenu = PS_GetToolMenu();
		if (toolMenu)
			toolMenu.PS_EnsureEntryButtons();
	}

	//! `allowRestore = false` is for teardown paths: restoring there would repopulate the
	//! tool menu right after SCR_MapToolMenuUI.OnMapClose already dropped the buttons, and
	//! the rebuilt set keeps living in the hidden layer until the next open. Runtime toggles
	//! restore; widget teardown paths do not.
	protected bool PS_HasToolButton(bool allowRestore = true)
	{
		if (allowRestore)
			PS_RestoreToolButton();

		return m_ToolMenuEntry && m_ToolMenuEntry.m_ButtonComp;
	}

	//! Runs only when the entry has no button at all, i.e. the current screen carries no tool
	//! bar. Draw mode must still be left inactive or its MapSelect listener and the cursor
	//! draw restriction outlive the map. Drawn lines keep their current button visibility: with
	//! no tool bar on screen there is nothing left to drive them, and MapLine's delete button is
	//! not reachable from here.
	//!
	//! `cacheDrawn = true` mirrors the close-time branch of vanilla SetDrawMode: drawn lines
	//! are destroyed while keeping their cache, so the next open resends them. Skipping that
	//! sweep left widgets behind in a screen whose layer is only hidden, and OnMapOpen then
	//! created a duplicate line next to every survivor.
	protected void PS_ForceDrawModeOff(bool cacheDrawn = false)
	{
		if (!m_bIsDrawModeActive)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.RemoveActionListener("MapSelect", EActionTrigger.UP, OnMapClick);
			inputManager.RemoveActionListener("MapContextualMenu", EActionTrigger.UP, OnMapModifierClick);
		}

		if (m_CursorModule)
			m_CursorModule.HandleDraw(false);

		if (m_bIsLineBeingDrawn && m_iLineID >= 0 && m_iLineID < m_aLines.Count())
		{
			m_aLines[m_iLineID].DestroyLine(false);
			m_bIsLineBeingDrawn = false;
			m_iLineID = -1;
		}

		if (cacheDrawn)
		{
			for (int i; i < m_aLines.Count(); i++)
			{
				if (m_aLines[i] && m_aLines[i].m_bIsLineDrawn)
					m_aLines[i].DestroyLine(true);
			}
		}

		m_bIsDrawModeActive = false;
	}

	//! No restore here: vanilla fires UpdateLineCount from MapLine.CreateLine and
	//! DestroyLine, the latter running inside close-time teardown, where restoring would
	//! repopulate the tool menu of a screen that is already closing.
	//! Skipping is harmless: the next SetDrawMode refreshes the counter.
	override void UpdateLineCount()
	{
		if (!PS_HasToolButton(false))
			return;

		super.UpdateLineCount();
	}

	//! `cacheDrawn` means OnMapClose: neither restore nor repopulate — leave cleanly and
	//! mirror the vanilla close sweep in PS_ForceDrawModeOff.
	override protected void SetDrawMode(bool state, bool cacheDrawn = false)
	{
		if (!PS_HasToolButton(!cacheDrawn))
		{
			PS_ForceDrawModeOff(cacheDrawn);
			return;
		}

		super.SetDrawMode(state, cacheDrawn);
	}
}