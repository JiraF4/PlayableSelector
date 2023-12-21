modded class SCR_InventoryMenuUI
{
	
	SCR_InventorySlotUI m_pFocusedSlotUIDrag;
	
	override void Action_DragDown()
	{
		if ( m_bDraggingEnabled )
			return;
		if ( !m_pFocusedSlotUI )
			return;
		if (!m_pFocusedSlotUI.IsDraggable())
			return;

		if ( m_pFocusedSlotUI && WidgetManager.GetWidgetUnderCursor() != m_pFocusedSlotUI.GetButtonWidget() )
			return;

		if 	(m_pFocusedSlotUI.Type() != SCR_InventorySlotUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotLBSUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotStorageUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotStorageEmbeddedUI &&
			m_pFocusedSlotUI.Type() != SCR_InventoryStorageWeaponsUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotWeaponUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotQuickSlotUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotWeaponSlotsUI &&
			m_pFocusedSlotUI.Type() != SCR_InventorySlotGearInspectionUI &&
			m_pFocusedSlotUI.Type() != SCR_ArsenalInventorySlotUI &&
			m_pFocusedSlotUI.Type() != SCR_SupplyInventorySlotUI)
			return;
		
		m_pFocusedSlotUIDrag = m_pFocusedSlotUI;
		WidgetManager.GetMousePos( m_iMouseX, m_iMouseY );
		GetGame().GetInputManager().AddActionListener( "Inventory_Drag", EActionTrigger.PRESSED, Action_OnDrag );
	}
	
	override void Action_OnDrag()
	{
		int iMouseX, iMouseY;
		WidgetManager.GetMousePos( iMouseX, iMouseY );
		if ( !m_bDraggingEnabled )
		{
			int dX = Math.AbsInt( iMouseX - m_iMouseX );
			int dY = Math.AbsInt( iMouseY - m_iMouseY );
			if ( ( dX < DRAG_THRESHOLD ) && ( dY < DRAG_THRESHOLD ) )
				return;
			if ( !m_pFocusedSlotUIDrag )
				return;
			if (!m_pFocusedSlotUIDrag.IsDraggable())
				return;
			InventoryItemComponent focusedItemComp = m_pFocusedSlotUIDrag.GetInventoryItemComponent();
			if (!focusedItemComp)
				return;
			
			InventoryStorageSlot slot = focusedItemComp.GetParentSlot();
			if (slot)
			{
				IEntity owner = slot.GetOwner();
				while (owner)
				{
					if (GetGame().GetPlayerController().GetControlledEntity() == owner)
					{
						SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_CONTAINER_DRAG);
						break;
					}
					
					owner = owner.GetParent();
					if (!owner)
						SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_VICINITY_DRAG);
				}
			}
			else
			{
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_VICINITY_DRAG);
			}
			
			m_bDraggingEnabled = true;
			SimpleFSM( EMenuAction.ACTION_DRAGGED );
			m_wDragDropContainer.SetVisible( true );

			if ( m_pPreviewManager && m_pFocusedSlotUIDrag )
			{
				HighlightAvailableStorages(m_pFocusedSlotUIDrag);
				ItemPreviewWidget renderPreview = ItemPreviewWidget.Cast( m_pDragDropPreviewImage );
				IEntity previewEntity = null;
				InventoryItemComponent pComp = m_pFocusedSlotUIDrag.GetInventoryItemComponent();
				if ( pComp )
				{
					previewEntity = pComp.GetOwner();
					if (renderPreview)
						m_pPreviewManager.SetPreviewItem(renderPreview, previewEntity);
				}
			}
			ContainerSetPos( iMouseX, iMouseY );
			GetGame().GetInputManager().AddActionListener( "Inventory_Drag", EActionTrigger.UP, Action_DragUp );
		}
		else
		{
			ContainerSetPos( iMouseX, iMouseY );
		}
	}
}