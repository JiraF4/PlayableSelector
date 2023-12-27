class PS_SpectatorLabelIcon : SCR_ScriptedWidgetComponent
{	
	IEntity m_eEintity;
	
	ImageWidget m_wSpectatorLabelIcon;
	TextWidget m_wSpectatorLabelText;
	PanelWidget m_wSpectatorLabel;
	
	protected float m_fMaxIconDistance = 800.0;
	protected float m_fMinIconDistance = 10.0;
	protected float m_fMaxIconSize = 64.0;
	protected float m_fMinIconSize = 4.0;
	protected float m_fMaxIconOpacity = 1;
	protected float m_fMinIconOpacity = 0.8;
	
	protected float m_fMaxLabelDistance = 25.0;
	protected float m_fMinLabelDistance = 10.0;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wSpectatorLabelIcon = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIcon"));
		m_wSpectatorLabelText = TextWidget.Cast(w.FindAnyWidget("SpectatorLabelText"));
		m_wSpectatorLabel = PanelWidget.Cast(w.FindAnyWidget("SpectatorLabel"));
	}
	
	void SetEntity(IEntity entity)
	{
		m_eEintity = entity;
	}
	
	void Update()
	{
		vector iconWorldPosition = m_eEintity.GetOrigin();
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(iconWorldPosition, GetGame().GetWorld());
		
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		float distanceToIcon = vector.Distance(cameraPosition, iconWorldPosition);
		
		if (screenPosition[2] < 0)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		if (distanceToIcon > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		if (distanceToIcon > m_fMaxLabelDistance)
			m_wSpectatorLabel.SetOpacity(0.0);
		else
		{
			float opacityLabel = 1.0 - ((distanceToIcon - m_fMinLabelDistance) / (m_fMaxLabelDistance - m_fMinLabelDistance));
			if (opacityLabel > 1.0) opacityLabel = 1.0;
			m_wSpectatorLabel.SetOpacity(opacityLabel);
		}
		
		float distanceScale = 1.0 - ((distanceToIcon - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance));
		
		float currentScale = distanceScale;
		if (currentScale > 1.0) currentScale = 1.0;
		float scaledIconSize = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * currentScale;
		float scaledIconOpacity = m_fMinIconOpacity + (m_fMaxIconOpacity - m_fMinIconOpacity) * currentScale;
		
		float LabelSizeX, LabelSizeY;
		m_wSpectatorLabel.GetScreenSize(LabelSizeX, LabelSizeY);
		float LabelSizeXD = GetGame().GetWorkspace().DPIUnscale(LabelSizeX);
		float LabelSizeYD = GetGame().GetWorkspace().DPIUnscale(LabelSizeY);
		
		m_wRoot.SetOpacity(1.0);
		FrameSlot.SetSize(m_wSpectatorLabelIcon, scaledIconSize, scaledIconSize);
		FrameSlot.SetPos(m_wSpectatorLabelIcon, -scaledIconSize/2, -scaledIconSize/2);
		FrameSlot.SetPos(m_wSpectatorLabel, -LabelSizeXD/2, -scaledIconSize + scaledIconSize/4);
		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);
	}
}