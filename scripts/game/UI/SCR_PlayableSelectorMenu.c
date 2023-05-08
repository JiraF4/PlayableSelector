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
				
				handler.SetText(playables[i].GetName());
				handler.SetFaction(faction);
				handler.SetPlayableId(i);
				
				gallery_component.AddItem(tile);
			}
		}
		
		gallery_component.SetCurrentItem(3);
		gallery_component.SetFocusedItem(3);
		
		if (itemsCount == 0)
			GameStateTransitions.RequestGameplayEndTransition();
	}
	
	override void OnMenuClose()
	{
		
	}
	
	protected void TileClick(SCR_ButtonBaseComponent button)
	{
		SCR_PlayableMenuTile handler = SCR_PlayableMenuTile.Cast(button.GetRootWidget().FindHandler(SCR_PlayableMenuTile));
		
		PlayerController playerController = GetGame().GetPlayerController();
		
		SCR_PlayableComponent playable = SCR_PlayableComponent.Cast(playerController.FindComponent(SCR_PlayableComponent));
		playable.TakePossession(playerController.GetPlayerId(), handler.GetPlayableId());
		
		//SCR_HintManagerComponent.ShowCustomHint(GetGame().GetPlayerController().GetPlayerId().ToString(), "!!!", 1000, true);
		//playables[handler.GetPlayableId()].TakePossession(GetGame().GetPlayerController().GetPlayerId());
		
		
		
		
		//SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetPossessedEntity(playables[handler.GetPlayableId()].GetOwner());
		//SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetControlledEntity(playables[handler.GetPlayableId()].GetOwner());
		
		//GetGame().GetPlayerController().Rpc(SCR_PlayableSelectorMenu.test, "Suck");
		
		Close();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	static void test(string test)
	{
		Print(test)
	}
};