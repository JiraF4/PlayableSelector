//------------------------------------------------------------------------------------------------



class SCR_PlayableSelectorMenu: MenuBase
{
	protected ResourceName m_sTilePrefab = "{CFA71C83A7ECC9CE}UI/PlayableMenuTile.layout";
	
	override void OnMenuOpen()
	{
		Widget gallery = GetRootWidget().FindAnyWidget("Tiles");
		SCR_GalleryComponent gallery_component = SCR_GalleryComponent.Cast(gallery.GetHandler(0));
		
		gallery_component.ClearAll();
		
		
		int itemsCount = 0;
		
		array<SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playables[i].GetOwner());
			
			if (!character.GetDamageManager().IsDestroyed()) {
				itemsCount++;
				
				Widget tile = GetGame().GetWorkspace().CreateWidgets(m_sTilePrefab);
				SCR_PlayableMenuTile handler = SCR_PlayableMenuTile.Cast(tile.FindHandler(SCR_PlayableMenuTile));
				
				SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
				
				auto m_PreviewManager = GetGame().GetItemPreviewManager();
				auto m_wPreview = ItemPreviewWidget.Cast(tile.FindAnyWidget("Preview"));
				
				m_PreviewManager.SetPreviewItem(m_wPreview, character);
				
				handler.m_OnClicked.Insert(TileClick);
				
				int playerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(character);
				if (playerId == 0) {
					handler.SetText(playables[i].GetName());
				} else {
					handler.SetText(GetGame().GetPlayerManager().GetPlayerName(playerId));
				}
				handler.SetFaction(faction);
				handler.SetPlayableId(i);
				
				gallery_component.AddItem(tile);
			}
		}
		
		gallery_component.SetCurrentItem(3);
		gallery_component.SetFocusedItem(3);
		
		//if (itemsCount == 0)
		//	GameStateTransitions.RequestGameplayEndTransition();
		
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
		array<SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		
		array<Widget> widgets = new array<Widget>();
		int count = gallery_component.GetWidgets(widgets);
		
		
		for (int i = 0; i < count; i++) {
			
			SCR_PlayableMenuTile handler = SCR_PlayableMenuTile.Cast(widgets[i].FindHandler(SCR_PlayableMenuTile));
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playables[handler.GetPlayableId()].GetOwner());
			
			
			if (character.GetDamageManager().IsDestroyed()) 
				gallery_component.RemoveItem(i);
			else 
			{
				int playerId = SCR_PossessingManagerComponent.GetInstance().GetPlayerIdFromControlledEntity(character);
				if (playerId == 0) {
					handler.SetText(playables[i].GetName());
				} else {
					handler.SetText(GetGame().GetPlayerManager().GetPlayerName(playerId));
				}
			}
		}
	}
	
	override void OnMenuClose()
	{
		array<SCR_PlayableComponent> playables = SCR_PlayableComponent.GetPlayables();
		for (int i = 0; i < playables.Count(); i++) {
			IEntity entity = playables[i].GetOwner();
			entity.SetFixedLOD(-1);
		}
	}
	
	protected void TileClick(SCR_ButtonBaseComponent button)
	{
		SCR_PlayableMenuTile handler = SCR_PlayableMenuTile.Cast(button.GetRootWidget().FindHandler(SCR_PlayableMenuTile));
		
		PlayerController playerController = GetGame().GetPlayerController();
		
		SCR_PlayableComponent playable = SCR_PlayableComponent.Cast(playerController.FindComponent(SCR_PlayableComponent));
		
		Close();
	
		playable.TakePossession(playerController.GetPlayerId(), handler.GetPlayableId());
	}
};