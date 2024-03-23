[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_GroupLateLinkerComponentClass : ScriptComponentClass
{
}

class PS_GroupLateLinkerComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		ClearEventMask(owner, EntityEvent.POSTFRAME);
		if (!Replication.IsServer())
			return;
			
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(owner);
		
		array<IEntity> characters = {};
		IEntity characterChild = owner.GetChildren();
		while (characterChild)
		{
			if (characterChild.IsInherited(SCR_ChimeraCharacter))
				characters.Insert(characterChild);
			characterChild = characterChild.GetSibling();
		}
		
		array<IEntity> charactersSorted = {};
		foreach (IEntity character : characters)
		{
			aiGroup.RemoveChild(character);
			
			bool added = false;
			for (int i = 0; i < charactersSorted.Count(); i++)
			{
				IEntity sortedCharacter = charactersSorted[i];
				if (SCR_CharacterRankComponent.GetCharacterRank(character) > SCR_CharacterRankComponent.GetCharacterRank(sortedCharacter))
				{
					charactersSorted.InsertAt(character, i);
					added = true;
					break;
				}
			}
			if (!added)
				charactersSorted.Insert(character);
		}
		
		foreach (IEntity character : charactersSorted)
		{
			aiGroup.AddAIEntityToGroup(character);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.POSTFRAME);
	}
}
