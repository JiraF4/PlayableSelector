class PS_LittleInventoryMatrix
{
	private int m_iMaxColumns;
	private int m_iMaxRows;
	
	private ref array<ref array<bool>> m_bMatrix;
	
	bool ReserveFirstFreePlace(out int xOut, out int yOut, int width, int height)
	{
		xOut = -1;
		yOut = -1;
		
		int y = 0;
		if (width > m_iMaxColumns) 
			return false;
		while (y < 200)
		{
			for (int x = 0; x < m_iMaxColumns; x++)
			{
				if (IsPlaceFree(x, y, width, height))
				{
					xOut = x;
					yOut = y;
					ReservePlace(x, y, width, height);
					return true;
				}
			}
			y++;
			if (y >= m_iMaxRows)
			{
				y -= 3;
				if (y < 0) y = 0;
				AddRow();
			}
		}
		return false;
	}
	
	void ReservePlace(int x, int y, int width, int height)
	{
		for (int xw = x; xw < x + width; xw++)
		{
			for (int yh = y; yh < y + height; yh++)
			{
				m_bMatrix[yh][xw] = true;
			}
		}
	}
	
	bool IsPlaceFree(int x, int y, int width, int height)
	{
		for (int xw = x; xw < x + width; xw++)
		{
			for (int yh = y; yh < y + height; yh++)
			{
				if (!m_bMatrix.IsIndexValid(yh))
					return false;
				if (!m_bMatrix[yh].IsIndexValid(xw))
					return false;
				if (m_bMatrix[yh][xw])
					return false;
			}
		}
		return true;
	}
	
	void AddRow()
	{
		m_iMaxRows++;
		m_bMatrix.Insert(new array<bool>());
		for (int i = 0;i < m_iMaxColumns;i++)
			m_bMatrix[m_iMaxRows-1].Insert(false);
	}
	
	void Reset()
	{
		for (int x = 0; x < m_iMaxRows; x++)
		{
			for (int y = 0; y < m_iMaxColumns; y++)
			{
				m_bMatrix[y][x] = false;
			}
		}
	}
	
	int GetColumnsCount()
	{
		return m_iMaxColumns;
	}
	
	int GetRowsCount()
	{
		return m_iMaxRows;
	}
	
	void PS_LittleInventoryMatrix(int maxColumns, int maxRows)
	{
		m_iMaxColumns = maxColumns;
		m_iMaxRows = maxRows;
		
		m_bMatrix = new array<ref array<bool>>();
		for (int y = 0;y < m_iMaxRows;y++)
		{
			array<bool> row = new array<bool>();
			for (int x = 0;x < m_iMaxColumns;x++)
			{
				row.Insert(false);
			}
			m_bMatrix.Insert(row);
		}
	}
}