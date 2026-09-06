modded class SCR_PlayerArsenalLoadout
{
	//------------------------------------------------------------------------------------------------
	//! Replaces the vanilla GetSlotItems which fails to compile due to SCR_SortableItem template
	//! usage ("Error in parameters" on array<ref SCR_SortableItem<...>> in some game builds).
	//! This implementation performs the same attachment-requirement sorting manually.
	//------------------------------------------------------------------------------------------------
	protected static override array<InventoryItemComponent> GetSlotItems(const BaseInventoryStorageComponent storage)
	{
		array<InventoryItemComponent> outItemsComponents = {};
		storage.GetOwnedItems(outItemsComponents, false);

		const SCR_WeaponAttachmentsStorageComponent weaponAttachments = SCR_WeaponAttachmentsStorageComponent.Cast(storage);
		if (weaponAttachments)
		{
			// Manual insertion sort by attachment requirement count (ascending).
			// Attachments with fewer prerequisites must be processed first so that
			// dependent slots (e.g. bayonet requiring a muzzle flash hider) have
			// their requirements already satisfied when the loadout is applied.
			for (int i = 1; i < outItemsComponents.Count(); i++)
			{
				InventoryItemComponent keyItem = outItemsComponents[i];
				int keyReq = PS_M_GetAttachmentRequirementCount(keyItem);

				int j = i - 1;
				while (j >= 0 && PS_M_GetAttachmentRequirementCount(outItemsComponents[j]) > keyReq)
				{
					outItemsComponents[j + 1] = outItemsComponents[j];
					j--;
				}
				outItemsComponents[j + 1] = keyItem;
			}
		}

		return outItemsComponents;
	}

	//------------------------------------------------------------------------------------------------
	//! Helper: count how many other attachments an item requires.
	//------------------------------------------------------------------------------------------------
	private static int PS_M_GetAttachmentRequirementCount(InventoryItemComponent item)
	{
		int numRequirements = 0;
		const SCR_WeaponAttachmentObstructionAttributes obstructionAttributes = SCR_WeaponAttachmentObstructionAttributes.Cast(item.FindAttribute(SCR_WeaponAttachmentObstructionAttributes));
		if (obstructionAttributes)
			numRequirements = obstructionAttributes.GetRequiredAttachmentTypes().Count();
		return numRequirements;
	}
}
