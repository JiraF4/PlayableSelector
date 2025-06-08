/*
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_SpectatorDrawProjClass : ScriptComponentClass
{
}

class PS_SpectatorDrawProj : ScriptComponent
{
	protected static const float MAX_ALIVE = 1.75;
	protected static bool m_isEnabled;
	
	protected ref array<ref Shape> m_lines = {};
	protected float m_alive = 0;
	protected float m_startSpeed;
	protected ShellMoveComponent shell;
	protected vector m_lastPos;
	
	override protected void OnPostInit(IEntity owner)
	{
		if(!m_isEnabled)
			return;
		
		GetGame().GetCallqueue().CallLater(DelayedInit, 0, false, owner);
	}
	
	protected void DelayedInit(IEntity owner)
	{
		shell = ShellMoveComponent.Cast(owner.FindComponent(ShellMoveComponent));
		m_startSpeed = shell.GetVelocity().LengthSq() * 0.9;
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
	}

	override protected void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		m_alive += timeSlice;
		if(m_alive > MAX_ALIVE || !m_isEnabled)
		{
			ClearEventMask(owner, EntityEvent.FIXEDFRAME);
			m_lines.Clear();
			return;
		}
		
		DrawLine(owner);
	}
	
	protected void DrawLine(IEntity owner)
	{
		vector pos = owner.GetOrigin();
		m_lines.Insert(Shape.Create(ShapeType.LINE, GetColor(), ShapeFlags.DEFAULT, m_lastPos, pos));
		m_lastPos = pos;
	}

	protected int GetColor()
	{
		if(m_alive < 0.1)
			return Color.RED;
		
		float speed = shell.GetVelocity().LengthSq();
		float t = Math.Clamp(speed / m_startSpeed, 0.0, 1.0);
		
		Color result = Color.Green.LerpNew(Color.Red, t);
		result.Saturate();
		
		return result.PackToInt();
	}
	
	void SetPos(vector position)
	{
		if(!m_isEnabled)
			return;
		
		m_lastPos = position;
	}
	
	static void SetEnabled(bool isEnabled)
	{
		m_isEnabled = isEnabled;
	}

	static void Toggle()
	{
		m_isEnabled = !m_isEnabled;
	}	
}
*/