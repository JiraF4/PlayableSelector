// Widget displays detailed info about playable character with 3D model and inventory info.
// Path: {FE42694A25ACA734}UI/Lobby/LoadoutPreview.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_LobbyLoadoutPreview : SCR_WLibComponentBase
{
	protected ImageWidget m_wFlagImage;
	protected ImageWidget m_wStateBackgroundImage;
	protected ImageWidget m_wLoadoutBackgroundImage;
	protected OverlayWidget m_WStateOverlay;
	protected OverlayWidget m_WLoadoutOverlay;
	protected TextWidget m_wStateText;
	protected TextWidget m_wLoadoutText;
	protected TextWidget m_wDescriptionText;
	protected TextWidget m_wDescriptionMagazinesText;
	protected SCR_LoadoutPreviewComponent m_Preview;
	
	protected PS_PlayableComponent m_playable;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wFlagImage = ImageWidget.Cast(w.FindAnyWidget("FlagImage"));
		m_wStateBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("StateBackgroundImage"));
		m_wLoadoutBackgroundImage = ImageWidget.Cast(w.FindAnyWidget("LoadoutBackgroundImage"));
		m_wStateText = TextWidget.Cast(w.FindAnyWidget("StateText"));
		m_wLoadoutText = TextWidget.Cast(w.FindAnyWidget("LoadoutText"));
		m_wDescriptionText = TextWidget.Cast(w.FindAnyWidget("DescriptionText"));
		m_wDescriptionMagazinesText = TextWidget.Cast(w.FindAnyWidget("DescriptionMagazinesText"));
		m_WStateOverlay = OverlayWidget.Cast(w.FindAnyWidget("StateOverlay"));
		m_WLoadoutOverlay = OverlayWidget.Cast(w.FindAnyWidget("LoadoutOverlay"));
		
		Widget widget = w.FindAnyWidget("LoadoutPreview");
		m_Preview = SCR_LoadoutPreviewComponent.Cast(widget.FindHandler(SCR_LoadoutPreviewComponent));
	}
	
	void SetPlayable(PS_PlayableComponent playable)
	{
		m_playable = playable;
		UpdatePreviewInfo();
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
	
	void UpdatePreviewInfo()
	{
		ItemPreviewManagerEntity m_PreviewManager = GetGame().GetItemPreviewManager();
		
		// Clear all data if playable is null and stop update
		if (m_playable == null) {
			m_wFlagImage.SetVisible(false);
			m_WStateOverlay.SetVisible(false);
			m_WLoadoutOverlay.SetVisible(false);
			m_PreviewManager.SetPreviewItem(m_Preview.GetItemPreviewWidget(), null);
			m_wDescriptionText.SetText("");
			m_wDescriptionMagazinesText.SetText("");
			return;
		}
		
		// Show preview
		m_wFlagImage.SetVisible(true);
		m_WLoadoutOverlay.SetVisible(true);
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_playable.GetOwner());
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
		m_wDescriptionText.SetText(weapons);
		
		// Concatinate all magazines to string and update widget text
		string magazines = "";
		for (int i = 0; i < magazinessCount.Count(); i++)
		{
			string line = WidgetManager.Translate(magazinessCount.GetKey(i));
			int count = magazinessCount.GetElement(i);
			magazines = magazines + line + "\n";
		}
		m_wDescriptionMagazinesText.SetText(magazines);
		
		// Set character preview. 
		// LODs for some reason start rave party if you to far from character. 
		// Yes it's performance issue but then why it switch between LODs? :<
		m_PreviewManager.SetPreviewItem(m_Preview.GetItemPreviewWidget(), character);
		m_wLoadoutText.SetText(m_playable.GetName());
		
		// Faction data
		m_wLoadoutBackgroundImage.SetColor(faction.GetOutlineFactionColor());
		m_wFlagImage.LoadImageTexture(0, faction.GetFactionFlag());	
		
		// Current playable player, or dead if playable already dead.
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		int playerId = playableManager.GetPlayerByPlayable(m_playable.GetId());
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