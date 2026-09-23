/**
 * @brief Перехват данных неймтегов для опционального скрытия 3D-иконок отделения.
 * @subsystem UI
 * @context Client
 * @entity ChimeraCharacter
 * @depends PlayableSelector
 * @listens None
 * @details Перехват SCR_NameTagData.SetGroup: при включённой настройке в PS_GameModeCoop
 *          (m_bDisableSquadNametagIcons) деактивирует состояния GROUP_LEADER (жёлтая звезда)
 *          и GROUP_MEMBER (зелёный ромб), скрывая 3D-иконки над головами соотрядников.
 */
modded class SCR_NameTagData
{
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Обновление группы и деактивация иконок отделения при активной настройке гейммода.
	 * @integration PlayableSelector: PS_GameModeCoop.GetDisableSquadNametagIcons()
	 */
	override void SetGroup(SCR_AIGroup group)
	{
		super.SetGroup(group);

		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameModeCoop && gameModeCoop.GetDisableSquadNametagIcons())
		{
			if (m_eEntityStateFlags & ENameTagEntityState.GROUP_LEADER)
				DeactivateEntityState(ENameTagEntityState.GROUP_LEADER);

			if (m_eEntityStateFlags & ENameTagEntityState.GROUP_MEMBER)
				DeactivateEntityState(ENameTagEntityState.GROUP_MEMBER);
		}
	}
}
