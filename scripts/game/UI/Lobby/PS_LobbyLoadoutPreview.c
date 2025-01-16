// Widget displays detailed info about playable character with 3D model and inventory info.
// Path: {FE42694A25ACA734}UI/Lobby/LoadoutPreview.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_LobbyLoadoutPreview : SCR_WLibComponentBase
{
	protected ImageWidget m_wStateBackgroundImage;
	protected ImageWidget m_wLoadoutBackgroundImage;
	protected OverlayWidget m_WStateOverlay;
	protected OverlayWidget m_WLoadoutOverlay;
	protected TextWidget m_wStateText;
	protected TextWidget m_wLoadoutText;
	protected FrameWidget m_wLoadoutPreviewDescription;
	
	protected SCR_LoadoutPreviewComponent m_Preview;
	protected SCR_ButtonBaseComponent m_hOpenInventoryButton;
	
	protected RplId m_iPlayableId;
	protected RplId m_iPlayableIdInventory;
	protected ResourceName m_sPrefabName;
	
	protected PS_ImportantItemsDisplay m_hImportantItemsDisplay;
	
	protected PS_LittleInventory m_hLittleInventory;
	protected PS_PlayableVehicleContainer m_PlayableVehicleContainer;
	protected PS_PlayableVehicleContainer m_PlayableVehicleContainerInventory;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wStateBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("StateBackgroundImage"));
		m_wLoadoutBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("LoadoutBackgroundImage"));
		m_wStateText = TextWidget.Cast(w.FindAnyWidget("StateText"));
		m_wLoadoutText = TextWidget.Cast(w.FindAnyWidget("LoadoutText"));
		m_WStateOverlay = OverlayWidget.Cast(w.FindAnyWidget("StateOverlay"));
		m_WLoadoutOverlay = OverlayWidget.Cast(w.FindAnyWidget("LoadoutOverlay"));
		m_wLoadoutPreviewDescription = FrameWidget.Cast(w.FindAnyWidget("LoadoutPreviewDescription"));
		
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
		
		Widget widgetOpenInventory = w.FindAnyWidget("OpenInventoryButton");
		m_hOpenInventoryButton = SCR_ButtonBaseComponent.Cast(widgetOpenInventory.FindHandler(SCR_ButtonBaseComponent));
		m_hOpenInventoryButton.m_OnClicked.Insert(OpenInventoryButton);
		
		Widget widgetLittleInventory = w.FindAnyWidget("LittleInventory");
		m_hLittleInventory = PS_LittleInventory.Cast(widgetLittleInventory.FindHandler(PS_LittleInventory));
		m_hLittleInventory.m_iOnEntityRemoved.Insert(EntityRemoved);
		
		widget = w.FindAnyWidget("ImportantItemsDisplay");
		m_hImportantItemsDisplay = PS_ImportantItemsDisplay.Cast(widget.FindHandler(PS_ImportantItemsDisplay));
	}
	
	void SetItemInfoWidget(Widget itemInfoWidget)
	{
		m_hLittleInventory.SetExternalItemInfoWidget(itemInfoWidget)
	}
	
	void SetPreviewPlayableVehicle(PS_PlayableVehicleContainer playableVehicleContainer, bool openInventory)
	{
		if (m_hLittleInventory.GetRootWidget().IsVisible() && !openInventory)
			return;
		SetPreviewPlayable(RplId.Invalid(), "", false);
		m_PlayableVehicleContainer = playableVehicleContainer;
		
		ItemPreviewManagerEntity previewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		previewManager.SetPreviewItemFromPrefab(m_Preview.GetItemPreviewWidget(), playableVehicleContainer.m_sPrefabName);
		IEntity entity = previewManager.ResolvePreviewEntityForPrefab(playableVehicleContainer.m_sPrefabName);
		
		m_WLoadoutOverlay.SetVisible(true);
		SCR_Faction faction = playableVehicleContainer.GetFaction();
		SCR_EditableVehicleComponent editableVehicleComponent = SCR_EditableVehicleComponent.Cast(entity.FindComponent(SCR_EditableVehicleComponent));
		SCR_UIInfo uIInfo = editableVehicleComponent.GetInfo();
		m_wLoadoutBackgroundImage.SetColor(faction.GetOutlineFactionColor());
		m_wLoadoutText.SetText(uIInfo.GetName());
		//m_wLoadoutPreviewDescription.SetVisible(false);
		
		if (openInventory)
		{
			if (m_PlayableVehicleContainerInventory == playableVehicleContainer)
			{
				CloseInventory();
				m_PlayableVehicleContainerInventory = null;
			}
			else {
				CloseInventory();
				m_hLittleInventory.GetRootWidget().SetVisible(true);
				m_hLittleInventory.OpenEntity(entity);
				m_PlayableVehicleContainerInventory = playableVehicleContainer;
			}
		}
	}
	
	void SetPreviewPlayable(RplId playableId, ResourceName prefabName, bool openInventory)
	{
		if (m_hLittleInventory.GetRootWidget().IsVisible() && !openInventory)
			return;
		if (prefabName != "")
		{
			m_wLoadoutPreviewDescription.SetVisible(true);
			m_PlayableVehicleContainer = null;
		}
		m_iPlayableId = playableId;
		m_sPrefabName = prefabName;
		UpdatePreviewInfo();
		IEntity entity = null;
		ItemPreviewManagerEntity previewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		if (prefabName != "") entity = previewManager.ResolvePreviewEntityForPrefab(prefabName);
		m_hImportantItemsDisplay.SetEntity(entity);
		
		if (openInventory)
		{
			if (playableId == m_iPlayableIdInventory)
			{
				CloseInventory();
				m_iPlayableIdInventory = RplId.Invalid();
			}
			else {
				CloseInventory();
				m_hLittleInventory.GetRootWidget().SetVisible(true);
				m_hLittleInventory.OpenEntity(entity);
				m_iPlayableIdInventory = playableId;
			}
		}
	}
	
	// Return all items recursive (If item is inventory)
	void GetAllItems(out notnull array<IEntity> items)
	{
		for (int i = 0;i < items.Count(); i++)
		{
			IEntity item = items[i];
			BaseInventoryStorageComponent subInventory = BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent));
			if (subInventory != null) {
				array<IEntity> subItems = new array<IEntity>();
				subInventory.GetAll(subItems);
				items.InsertAll(subItems);
			}
		}
	}
	
	void EntityRemoved(PS_LittleInventoryEntity inventoryEntity)
	{
		if (m_hLittleInventory.IsAllClosed())
		{
			CloseInventory();
		}
	}
	
	void CloseInventory()
	{
		m_PlayableVehicleContainerInventory = null;
		m_iPlayableIdInventory = null;
		m_hLittleInventory.Clear();
		m_hLittleInventory.GetRootWidget().SetVisible(false);
	}
	
	void OpenInventory()
	{
		m_hLittleInventory.GetRootWidget().SetVisible(true);
		IEntity entity = null;
		ItemPreviewManagerEntity previewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		if (m_sPrefabName != "") entity = previewManager.ResolvePreviewEntityForPrefab(m_sPrefabName);
		if (m_PlayableVehicleContainer != null) entity = previewManager.ResolvePreviewEntityForPrefab(m_PlayableVehicleContainer.m_sPrefabName);
		if (entity) m_hLittleInventory.OpenEntity(entity);
	}
	
	void OpenInventoryButton(SCR_ButtonBaseComponent button)
	{
		if (m_hLittleInventory.GetRootWidget().IsVisible())
			CloseInventory();
		else
			OpenInventory();
	}
	
	void UpdatePreviewInfo()
	{
		ItemPreviewManagerEntity previewManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		
		// Clear all data if playable is null and stop update
		if (m_sPrefabName == "") {
			m_WStateOverlay.SetVisible(false);
			m_WLoadoutOverlay.SetVisible(false);
			previewManager.SetPreviewItemFromPrefab(m_Preview.GetItemPreviewWidget(), m_sPrefabName);
			return;
		}
		
		// Show preview
		m_WLoadoutOverlay.SetVisible(true);
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(previewManager.ResolvePreviewEntityForPrefab(m_sPrefabName));
		SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
		
		// Extract all? items
		SCR_EditableCharacterComponent editableCharacterComponent = SCR_EditableCharacterComponent.Cast(character.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo uiInfo = editableCharacterComponent.GetInfo();
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		array<IEntity> items = new array<IEntity>();
		BaseInventoryStorageComponent weaponsStorage = BaseInventoryStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));
		array<IEntity> weaponsItems = new array<IEntity>();
		items.Insert(inventory.GetOwner());
		GetAllItems(items);
		items.InsertAll(weaponsItems);
		
		// maps for group items by name
		map<string, int> weaponsCount = new map<string, int>();
		map<string, int> magazinessCount = new map<string, int>();
		
		// Extract items names and group by it
		foreach (IEntity item : items)
		{
			MagazineComponent magazine = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			if (magazine != null)
			{
				MagazineUIInfo magazineUIInfo = MagazineUIInfo.Cast(magazine.GetUIInfo());
				string name = magazineUIInfo.GetAmmoType();
				if (name != "") {
					if (magazinessCount.Contains(name)){
						magazinessCount[name] = weaponsCount[name] + 1;
					}else{
						magazinessCount[name] = 1;
					}
				}
			}
			
			
			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
			if (weapon != null)
			{
				UIInfo weaponUIInfo = weapon.GetUIInfo();
				GrenadeUIInfo grenadeInfo = GrenadeUIInfo.Cast(weaponUIInfo);
				string name = "";
				if (grenadeInfo){
					name = grenadeInfo.GetAmmoType();
				}else{
					if (weaponUIInfo) name = weaponUIInfo.GetName();
				}
				if (name != "") {
					if (weaponsCount.Contains(name)){
						weaponsCount[name] = weaponsCount[name] + 1;
					}else{
						weaponsCount[name] = 1;
					}
				}
			}
		}
		
		// Concatinate all weapons to string and update widget text
		string weapons = "";
		for (int i = 0; i < weaponsCount.Count(); i++)
		{
			
			string line = WidgetManager.Translate(weaponsCount.GetKey(i));
			int count = weaponsCount.GetElement(i);
			weapons = weapons + line + "\n";
		}
		
		// Concatinate all magazines to string and update widget text
		string magazines = "";
		for (int i = 0; i < magazinessCount.Count(); i++)
		{
			string line = WidgetManager.Translate(magazinessCount.GetKey(i));
			int count = magazinessCount.GetElement(i);
			magazines = magazines + line + "\n";
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		// Set character preview. 
		// LODs for some reason start rave party if you to far from character. 
		// Yes it's performance issue but then why it switch between LODs? :<
		previewManager.SetPreviewItemFromPrefab(m_Preview.GetItemPreviewWidget(), m_sPrefabName);
		m_wLoadoutText.SetText(playableManager.GetPlayableName(m_iPlayableId));
		
		// Faction data
		m_wLoadoutBackgroundImage.SetColor(faction.GetOutlineFactionColor());
		
		// Current playable player, or dead if playable already dead.
		int playerId = playableManager.GetPlayerByPlayable(m_iPlayableId);
		if (character.GetDamageManager().IsDestroyed()) 
		{
			m_WStateOverlay.SetVisible(true);
			m_wStateText.SetText("Dead");
			m_wStateBackgroundImage.SetColor(Color.FromInt(0xFF303031));
		}
		else 
		{
			if (playerId != -1) 
			{
				PlayerManager playerManager = GetGame().GetPlayerManager();
				m_WStateOverlay.SetVisible(true);
				m_wStateText.SetText(playerManager.GetPlayerName(playerId));
				m_wStateBackgroundImage.SetColor(Color.FromInt(0xFF19322e));
			}else{
				m_WStateOverlay.SetVisible(false);
			}
		}
	}
	
}