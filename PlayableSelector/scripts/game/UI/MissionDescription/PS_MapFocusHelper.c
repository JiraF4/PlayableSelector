/**
 * @brief Resolves briefing links against the current client map and world.
 */
class PS_MapFocusHelper
{
	static const float DEFAULT_FOCUS_ZOOM = 2.5;
	static const float FOCUS_TRANSITION_TIME = 0.4;

	static void Focus(string targetSpec, float zoomOverride = 0.0)
	{
		if (!GetGame() || !GetGame().GetMenuManager() || !PS_BriefingMapMenu.Cast(GetGame().GetMenuManager().GetTopMenu()))
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		string target = targetSpec.Trim();
		float zoom = zoomOverride;
		int colon = target.LastIndexOf(":");
		if (colon >= 0)
		{
			string zoomText = target.Substring(colon + 1, target.Length() - colon - 1);
			if (!TryParseFloat(zoomText, zoom) || zoom <= 0)
			{
				Print("[PS_MapFocusHelper] Invalid zoom in link: " + targetSpec, LogLevel.WARNING);
				return;
			}
			target = target.Substring(0, colon);
		}

		if (zoom <= 0)
			zoom = DEFAULT_FOCUS_ZOOM;

		if (target.Length() == 0)
			return;

		if (target.Substring(0, 1) != "@")
		{
			ToEntity(target, zoom);
			return;
		}

		string coordinates = target.Substring(1, target.Length() - 1);
		int comma = coordinates.IndexOf(",");
		if (comma <= 0 || comma >= coordinates.Length() - 1 || coordinates.LastIndexOf(",") != comma)
		{
			Print("[PS_MapFocusHelper] Invalid coordinates in link: " + targetSpec, LogLevel.WARNING);
			return;
		}

		float worldX;
		float worldZ;
		if (!TryParseFloat(coordinates.Substring(0, comma), worldX) || !TryParseFloat(coordinates.Substring(comma + 1, coordinates.Length() - comma - 1), worldZ))
		{
			Print("[PS_MapFocusHelper] Invalid coordinates in link: " + targetSpec, LogLevel.WARNING);
			return;
		}

		ToCoords(worldX, worldZ, zoom);
	}

	static void ToEntity(string entityName, float zoom)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen() || !GetGame() || !GetGame().GetWorld())
			return;

		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
		{
			Print("[PS_MapFocusHelper] No world entity named '" + entityName + "'", LogLevel.WARNING);
			return;
		}

		vector origin = entity.GetOrigin();
		ToCoords(origin[0], origin[2], zoom);
	}

	static void ToCoords(float worldX, float worldZ, float zoom)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen() || !GetGame() || !GetGame().GetWorld())
			return;

		if (!IsWithinWorldBounds(worldX, worldZ))
		{
			Print("[PS_MapFocusHelper] Coordinates are outside world bounds", LogLevel.WARNING);
			return;
		}

		if (zoom <= 0)
			zoom = DEFAULT_FOCUS_ZOOM;
		zoom = Math.Clamp(zoom, mapEntity.GetMinZoom(), mapEntity.GetMaxZoom());
		mapEntity.ZoomPanSmooth(zoom, worldX, worldZ, FOCUS_TRANSITION_TIME);
	}

	protected static bool IsWithinWorldBounds(float worldX, float worldZ)
	{
		if (!GetGame() || !GetGame().GetWorld() || worldX != worldX || worldZ != worldZ)
			return false;

		vector mins;
		vector maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		return worldX >= mins[0] && worldX <= maxs[0] && worldZ >= mins[2] && worldZ <= maxs[2];
	}

	// ToFloat accepts a numeric prefix; briefing coordinates must consume the whole field.
	protected static bool TryParseFloat(string text, out float value)
	{
		string number = text.Trim();
		int parsed;
		value = number.ToFloat(parsed: parsed);
		return parsed == number.Length() && parsed > 0 && value == value;
	}
}
