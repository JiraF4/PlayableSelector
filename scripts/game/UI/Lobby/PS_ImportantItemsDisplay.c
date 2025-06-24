class PS_ImportantItemsDisplay : SCR_ScriptedWidgetComponent
{
	PS_ImportantSlot m_hPrimaryWeapon;
	PS_ImportantSlot m_hSecondaryWeapon;
	PS_ImportantSlot m_hHandWeapon;
	PS_ImportantSlot m_hGrenade;
	PS_ImportantSlot m_hMap;
	PS_ImportantSlot m_hBinocular;
	PS_ImportantSlot m_hCompas;
	PS_ImportantSlot m_hFlashlight;
	PS_ImportantSlot m_hRadio;
	PS_ImportantSlot m_hWatch;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_hPrimaryWeapon = PS_ImportantSlot.Cast(w.FindAnyWidget("PrimaryWeapon").FindHandler(PS_ImportantSlot));
		m_hSecondaryWeapon = PS_ImportantSlot.Cast(w.FindAnyWidget("SecondaryWeapon").FindHandler(PS_ImportantSlot));
		m_hHandWeapon = PS_ImportantSlot.Cast(w.FindAnyWidget("HandWeapon").FindHandler(PS_ImportantSlot));
		m_hGrenade = PS_ImportantSlot.Cast(w.FindAnyWidget("Grenade").FindHandler(PS_ImportantSlot));
		m_hMap = PS_ImportantSlot.Cast(w.FindAnyWidget("Map").FindHandler(PS_ImportantSlot));
		m_hBinocular = PS_ImportantSlot.Cast(w.FindAnyWidget("Binocular").FindHandler(PS_ImportantSlot));
		m_hCompas = PS_ImportantSlot.Cast(w.FindAnyWidget("Compas").FindHandler(PS_ImportantSlot));
		m_hFlashlight = PS_ImportantSlot.Cast(w.FindAnyWidget("Flashlight").FindHandler(PS_ImportantSlot));
		m_hRadio = PS_ImportantSlot.Cast(w.FindAnyWidget("Radio").FindHandler(PS_ImportantSlot));
		m_hWatch = PS_ImportantSlot.Cast(w.FindAnyWidget("Watch").FindHandler(PS_ImportantSlot));
	}
	
	void ResetPreview()
	{
		m_hPrimaryWeapon.SetItemEntity(null);
		m_hSecondaryWeapon.SetItemEntity(null);
		m_hHandWeapon.SetItemEntity(null);
		m_hGrenade.SetItemEntity(null);
		m_hMap.SetItemEntity(null);
		m_hBinocular.SetItemEntity(null);
		m_hCompas.SetItemEntity(null);
		m_hFlashlight.SetItemEntity(null);
		m_hRadio.SetItemEntity(null);
		m_hWatch.SetItemEntity(null);
	}
	
	void SetEntity(IEntity entity)
	{
		if (!entity) {
			ResetPreview();
			return;
		}
			
		ItemPreviewManagerEntity m_PreviewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		CharacterWeaponManagerComponent weaponsManager = CharacterWeaponManagerComponent.Cast(entity.FindComponent(CharacterWeaponManagerComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		
		array<WeaponSlotComponent> outSlots = new array<WeaponSlotComponent>();
		weaponsManager.GetWeaponsSlots(outSlots);
		m_hPrimaryWeapon.SetItemEntity(outSlots[2].GetWeaponEntity());
		m_hSecondaryWeapon.SetItemEntity(outSlots[3].GetWeaponEntity());
		m_hHandWeapon.SetItemEntity(outSlots[4].GetWeaponEntity());
		m_hGrenade.SetItemEntity(outSlots[0].GetWeaponEntity());
		
		m_hMap.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_MapGadgetComponent}, EStoragePurpose.PURPOSE_ANY));
		m_hBinocular.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_BinocularsComponent}, EStoragePurpose.PURPOSE_ANY));
		m_hCompas.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_CompassComponent}, EStoragePurpose.PURPOSE_ANY));
		m_hFlashlight.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_FlashlightComponent}, EStoragePurpose.PURPOSE_ANY));
		m_hRadio.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_RadioComponent}, EStoragePurpose.PURPOSE_ANY));
		m_hWatch.SetItemEntity(inventoryManager.FindItemWithComponents({SCR_WristwatchComponent}, EStoragePurpose.PURPOSE_ANY));
		
	}
}