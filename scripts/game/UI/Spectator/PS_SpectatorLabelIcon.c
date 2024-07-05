class PS_SpectatorLabelIcon : SCR_ScriptedWidgetComponent
{	
	IEntity m_eEntity;
	
	ImageWidget m_wSpectatorLabelIcon;
	ButtonWidget m_wLabelButton;
	OverlayWidget m_wSpectatorLabelBackground;
	TextWidget m_wSpectatorLabelText;
	PanelWidget m_wSpectatorLabel;
	
	protected float m_fMaxIconDistance = 800.0;
	protected float m_fMinIconDistance = 10.0;
	[Attribute("64.0")]
	protected float m_fMaxIconSize;
	[Attribute("4.0")]
	protected float m_fMinIconSize;
	protected float m_fMaxIconOpacity = 1;
	protected float m_fMinIconOpacity = 0.8;
	
	protected float m_fMaxLabelDistance = 25.0;
	protected float m_fMinLabelDistance = 10.0;
	
	protected TNodeId w_iBoneIndex;
	
	protected float m_fDistanceToIcon;
	
	protected bool m_bForceShowName;
	
	vector m_vWorldPosition;
	vector GetWorldPosition()
		return m_vWorldPosition;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wSpectatorLabelIcon = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIcon"));
		m_wLabelButton = ButtonWidget.Cast(w.FindAnyWidget("LabelButton"));
		m_wSpectatorLabelBackground = OverlayWidget.Cast(w.FindAnyWidget("SpectatorLabelBackground"));
		m_wSpectatorLabelText = TextWidget.Cast(w.FindAnyWidget("SpectatorLabelText"));
		m_wSpectatorLabel = PanelWidget.Cast(w.FindAnyWidget("SpectatorLabel"));
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bForceShowName = true;
		return true;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bForceShowName = false;
		return true;
	}
	
	void SetEntity(IEntity entity, string boneName)
	{
		m_eEntity = entity;
		if (boneName != "")
		{
			w_iBoneIndex = entity.GetAnimation().GetBoneIndex(boneName);
		}
	}
	
	void Update()
	{
		m_vWorldPosition = m_eEntity.GetOrigin();
		if (w_iBoneIndex > 0)
		{
			vector mat[4];
			m_eEntity.GetTransform(mat);
			vector boneMat[4];
			m_eEntity.GetAnimation().GetBoneMatrix(w_iBoneIndex, boneMat);
			vector resMat[4];
			Math3D.MatrixMultiply4(mat, boneMat, resMat);
			m_vWorldPosition = resMat[3];
		}
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());
		
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		m_fDistanceToIcon = vector.Distance(cameraPosition, m_vWorldPosition);
		
		if (screenPosition[2] < 0)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		if (m_fDistanceToIcon > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		if (m_fDistanceToIcon > m_fMaxLabelDistance)
			m_wSpectatorLabel.SetOpacity(0.0);
		else
		{
			float opacityLabel = 1.0 - ((m_fDistanceToIcon - m_fMinLabelDistance) / (m_fMaxLabelDistance - m_fMinLabelDistance));
			if (opacityLabel > 1.0) opacityLabel = 1.0;
			m_wSpectatorLabel.SetOpacity(opacityLabel);
		}
		
		float distanceScale = 1.0 - ((m_fDistanceToIcon - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance));
		
		float currentScale = distanceScale;
		if (currentScale > 1.0) currentScale = 1.0;
		float scaledIconSize = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * currentScale;
		float scaledIconOpacity = m_fMinIconOpacity + (m_fMaxIconOpacity - m_fMinIconOpacity) * currentScale;
		
		float LabelSizeX, LabelSizeY;
		m_wSpectatorLabel.GetScreenSize(LabelSizeX, LabelSizeY);
		float LabelSizeXD = GetGame().GetWorkspace().DPIUnscale(LabelSizeX);
		float LabelSizeYD = GetGame().GetWorkspace().DPIUnscale(LabelSizeY);
		
		m_wRoot.SetOpacity(1.0);
		UpdateLabel();
		
		FrameSlot.SetSize(m_wSpectatorLabelIcon, scaledIconSize, scaledIconSize);
		FrameSlot.SetPos(m_wSpectatorLabelIcon, -scaledIconSize/2, -scaledIconSize/2);
		if (m_wLabelButton)
		{
			FrameSlot.SetSize(m_wLabelButton, scaledIconSize, scaledIconSize);
			FrameSlot.SetPos(m_wLabelButton, -scaledIconSize/2, -scaledIconSize/2);
		}
		FrameSlot.SetPos(m_wSpectatorLabel, -LabelSizeXD/2, -scaledIconSize);
		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);
		
		m_wRoot.SetZOrder(screenPosition[2] * -10000);
		
		if (m_bForceShowName)
		{
			m_wSpectatorLabel.SetOpacity(1.0);
			m_wRoot.SetZOrder(100);
		}
	}
	
	void UpdateLabel()
	{
		
	}
}