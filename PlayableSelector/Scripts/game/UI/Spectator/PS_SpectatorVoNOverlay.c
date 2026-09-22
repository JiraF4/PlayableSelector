/**
 * @brief Оверлей говорящих игроков в спектаторе — вертикальный список ников.
 * @subsystem UI
 * @context Client
 * @entity SpectatorMenu (контейнер SpectatorVoNOverlay в SpectatorMenu.layout)
 * @depends PlayableSelector
 * @listens SCR_VoNComponent::s_OnPSTalkingChanged (через PS_GetOnTalkingChanged())
 * @details Подписывается на движковый инвокер talking-состояния при
 *          HandlerAttached и отписывается при HandlerDeattached. При
 *          talking==true создаёт строку-виджет с ником говорящего; при
 *          talking==false удаляет её. Отображает только чужих — собственный
 *          ID фильтруется. Управляется видимостью через SetVisible()
 *          из PS_SpectatorMenu.Action_SwitchSpectatorUI.
 */
class PS_SpectatorVoNOverlay : ScriptedWidgetComponent
{
	/// Максимум одновременно видимых строк. При превышении вытесняется самая старая.
	protected static const int MAX_ROWS = 5;

	[Attribute("", UIWidgets.ResourceNamePicker, "Layout строки говорящего игрока", "layout")]
	protected ResourceName m_sRowPrefab;

	protected VerticalLayoutWidget m_wRowsLayout;

	/**
	 * @brief Словарь активных строк: playerId → корневой виджет строки.
	 * @details Порядок элементов соответствует порядку добавления в layout
	 *          (FIFO). При MAX_ROWS — первый элемент удаляется.
	 */
	protected ref map<int, Widget> m_mRows = new map<int, Widget>();

	/// ID локального игрока — его строка не показывается.
	protected int m_iLocalPlayerId = -1;

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Инициализация: найти layout, подписаться на инвокер talking-состояния.
	 * @integration PlayableSelector: SCR_VoNComponent.PS_GetOnTalkingChanged()
	 * @fallback Если VerticalLayout "VoNRows" не найден в иерархии — компонент молчит.
	 */
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;

		m_wRowsLayout = VerticalLayoutWidget.Cast(w.FindAnyWidget("VoNRows"));
		if (!m_wRowsLayout)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			m_iLocalPlayerId = pc.GetPlayerId();

		/**
		 * @event Подписка на s_OnPSTalkingChanged (int playerId, bool talking)
		 */
		SCR_VoNComponent.PS_GetOnTalkingChanged().Insert(OnTalkingChanged);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Очистка: отписаться и удалить все виджеты строк.
	 * @integration PlayableSelector: SCR_VoNComponent.PS_GetOnTalkingChanged()
	 */
	override void HandlerDeattached(Widget w)
	{
		SCR_VoNComponent.PS_GetOnTalkingChanged().Remove(OnTalkingChanged);
		ClearAllRows();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Колбэк изменения talking-состояния игрока.
	 * @param playerId  Идентификатор говорящего/замолчавшего игрока.
	 * @param talking   true — начал говорить, false — замолчал.
	 */
	protected void OnTalkingChanged(int playerId, bool talking)
	{
		// Не показывать собственный ник
		if (playerId == m_iLocalPlayerId)
			return;

		if (talking)
		{
			// Строка уже есть — ничего не делаем (таймер продолжается в движке)
			if (m_mRows.Contains(playerId))
				return;

			// Вытеснение при переполнении: удалить самую старую (первую) строку
			if (m_mRows.Count() >= MAX_ROWS)
			{
				int oldestId = m_mRows.GetKey(0);
				RemoveRow(oldestId);
			}

			CreateRow(playerId);
		}
		else
		{
			RemoveRow(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Создать строку с ником говорящего и добавить в layout.
	 * @param playerId  ID игрока, чья строка создаётся.
	 */
	protected void CreateRow(int playerId)
	{
		if (!m_wRowsLayout || m_sRowPrefab.IsEmpty())
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(m_sRowPrefab, m_wRowsLayout);
		if (!row)
			return;

		// Установить имя игрока в TextWidget "PlayerName"
		TextWidget nameWidget = TextWidget.Cast(row.FindAnyWidget("PlayerName"));
		if (nameWidget)
		{
			string playerName;
			PS_PlayableManager pm = PS_PlayableManager.GetInstance();
			if (pm)
				playerName = pm.GetPlayerName(playerId);
			if (playerName.IsEmpty())
				playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			nameWidget.SetText(playerName);
		}

		m_mRows.Set(playerId, row);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Удалить строку говорящего из layout и словаря.
	 * @param playerId  ID игрока, чья строка удаляется.
	 */
	protected void RemoveRow(int playerId)
	{
		Widget row;
		if (!m_mRows.Find(playerId, row))
			return;

		if (row)
			row.RemoveFromHierarchy();

		m_mRows.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Удалить все строки (вызывается при HandlerDeattached).
	 */
	protected void ClearAllRows()
	{
		foreach (int playerId, Widget row : m_mRows)
		{
			if (row)
				row.RemoveFromHierarchy();
		}
		m_mRows.Clear();
	}
}
