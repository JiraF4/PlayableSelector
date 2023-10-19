//------------------------------------------------------------------------------------------------



class PS_PlayableSelectorMenu: MenuBase
{
	protected ResourceName m_sTilePrefab = "{CFA71C83A7ECC9CE}UI/PlayableMenuTile.layout";
	protected bool locked;
	
	override void OnMenuOpen()
	{
		locked = false;
		
		Widget gallery = GetRootWidget().FindAnyWidget("Tiles");
		if (gallery == null) return;
		SCR_GalleryComponent gallery_component = SCR_GalleryComponent.Cast(gallery.GetHandler(0));
		
		gallery_component.ClearAll();
		
		
		int itemsCount = 0;
		
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			PS_PlayableComponent playable = playables.GetElement(i);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			character.SetFixedLOD(0);
			
			if (!character.GetDamageManager().IsDestroyed()) {
				itemsCount++;
				
				Widget tile = GetGame().GetWorkspace().CreateWidgets(m_sTilePrefab);
				PS_PlayableMenuTile handler = PS_PlayableMenuTile.Cast(tile.FindHandler(PS_PlayableMenuTile));
				
				SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
				
				auto m_PreviewManager = GetGame().GetItemPreviewManager();
				auto m_wPreview = ItemPreviewWidget.Cast(tile.FindAnyWidget("Preview"));
				
				m_PreviewManager.SetPreviewItem(m_wPreview, character);
				
				handler.m_OnClicked.Insert(TileClick);
				
				handler.SetFaction(faction);
				handler.SetPlayableId(playable.GetId());
				
				gallery_component.AddItem(tile);
			}
		}
		
		UpdateList();
		
		gallery_component.SetCurrentItem(3);
		gallery_component.SetFocusedItem(3);
		
		GetGame().GetInputManager().ResetAction("MenuBack");
		#ifdef WORKBENCH
		        GetGame().GetInputManager().AddActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
		#else
		        GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Close);
		#endif
	}
	
	void UpdateList() 
	{
		Widget gallery = GetRootWidget().FindAnyWidget("Tiles");
		SCR_GalleryComponent gallery_component = SCR_GalleryComponent.Cast(gallery.GetHandler(0));
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		
		array<Widget> widgets = new array<Widget>();
		int count = gallery_component.GetWidgets(widgets);
		
		
		for (int i = 0; i < count; i++) {
			PS_PlayableMenuTile handler = PS_PlayableMenuTile.Cast(widgets[i].FindHandler(PS_PlayableMenuTile));
			PS_PlayableComponent playable = playables.Get(handler.GetPlayableId());
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
			
			
			if (character.GetDamageManager().IsDestroyed()) 
			{
				handler.SetDead();
			}
			else 
			{
				handler.SetNameText(playable.GetName());
				int playerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(character);
				handler.SetPlayer(playerId);
			}
		}
	}
	
	void Unlock() 
	{
		locked = false;
	}
	
	override void OnMenuClose()
	{
		map<int, PS_PlayableComponent> playables = PS_PlayableComponent.GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			IEntity entity = playables.GetElement(i).GetOwner();
			entity.SetFixedLOD(-1);
		}
	}
	
	protected void TileClick(SCR_ButtonBaseComponent button)
	{
		if (locked) return;
		
		PS_PlayableMenuTile handler = PS_PlayableMenuTile.Cast(button.GetRootWidget().FindHandler(PS_PlayableMenuTile));
		
		PlayerController playerController = GetGame().GetPlayerController();
		
		PS_PlayableControllerComponent playable = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		
		locked = true;
	
		playable.TakePossession(playerController.GetPlayerId(), handler.GetPlayableId());
	}
};