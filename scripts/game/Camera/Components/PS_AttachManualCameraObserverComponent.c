void PS_ScriptInvokerOnAttachChangeMethod(bool attached, IEntity entity);
typedef func PS_ScriptInvokerOnAttachChangeMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnAttachChangeMethod> PS_ScriptInvokerOnAttachChange;

[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
class PS_AttachManualCameraObserverComponent : SCR_BaseManualCameraComponent
{
	[Attribute(defvalue: "0")]
	protected bool m_bRotateWithTarget;

	[Attribute(string.Format("%1", EAttachManualCameraType.INPUT), UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(EAttachManualCameraType))]
	private EAttachManualCameraType m_eAttachType;

	[Attribute(params: "layout")]
	private ResourceName m_sLayout;

	protected IEntity m_Target;
	protected SCR_AttachEntity m_AttachHelper;
	protected bool m_bAttachChanged;
	private Widget m_wWidget;
	protected ref PS_ScriptInvokerOnAttachChange m_OnAttachChange = new PS_ScriptInvokerOnAttachChange();

	// Thx for privates BI
	static PS_AttachManualCameraObserverComponent s_Instance;

	IEntity GetTarget()
	{
		return m_Target;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! Attach camera to an entity.
	//! \param[in] parent Target entity
	bool AttachTo(IEntity target)
	{
		if (!target)
			return false;

		IEntity parent = target.GetParent();
		if (parent)
		{
			while (parent.GetParent())
			{
				parent = parent.GetParent();
			}
		}
		if (parent && parent.IsInherited(ChimeraCharacter))
			target = parent;

		//--- No target, already attached target or inactive target - ignore
		if (target == m_Target || !SCR_Enum.HasFlag(target.GetFlags(), EntityFlags.TRACEABLE))
			return false;

		SCR_ManualCamera camera = GetCameraEntity();
		if (!camera)
			return false;

		Physics targetPhysics = target.GetPhysics();
		//--- A target is static if is not kinematic and not dynamic (eg: tree)
		if (targetPhysics && !targetPhysics.IsKinematic() && !targetPhysics.IsDynamic())
			return false;

		//--- Already attached - detach first
		if (m_AttachHelper)
			Detach();

		m_Target = target;

		//--- Create helper entity and attach camera to it, so it moves in its local coordinate space
		//--- Don't attach on target itself, because we only want to move, not rotate camera with it
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = m_Target.CoordToParent(vector.Zero);
		m_AttachHelper = SCR_AttachEntity.Cast(GetGame().SpawnEntity(SCR_AttachEntity, camera.GetWorld(), spawnParams));
		m_AttachHelper.AttachTo(m_Target);
		m_AttachHelper.RotateWithTarget(m_bRotateWithTarget);
		m_AttachHelper.EOnPostFrame(m_AttachHelper, 0);
		camera.AttachTo(m_AttachHelper);

		m_bAttachChanged = true;

		m_OnAttachChange.Invoke(true, m_Target);

		//Print("Attached to " + target.ToString(), LogLevel.DEBUG);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Detach camera from its parent entity.
	void Detach()
	{
		if (!m_AttachHelper)
			return;

		SCR_ManualCamera camera = GetCameraEntity();
		if (!camera)
			return;

		m_OnAttachChange.Invoke(false, m_Target);

		m_Target = null;

		//--- Detach and delete the attach helper
		camera.Detach();
		delete m_AttachHelper;

		m_bAttachChanged = true;

		//Print("Detach", LogLevel.DEBUG);
	}

	//------------------------------------------------------------------------------------------------
	//! Get event called when camera is attached or detached.
	//! \return Script invoker
	PS_ScriptInvokerOnAttachChange GetOnAttachChange()
	{
		return m_OnAttachChange;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraSave(SCR_ManualCameraComponentSave data)
	{
		if (m_Target)
		{
			data.m_aValues = {m_eAttachType};
			data.m_Target = m_Target;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraLoad(SCR_ManualCameraComponentSave data)
	{
		if (data.m_Target && m_eAttachType == data.m_aValues[0] && m_eAttachType != EAttachManualCameraType.PLAYER)
			AttachTo(data.m_Target);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		//--- Target deleted, detach
		if (m_AttachHelper && !m_Target)
			Detach();

		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (spectatorMenu)
		{
			if (!spectatorMenu.IsUIVisible())
			{
				if (m_wWidget)
					m_wWidget.SetVisible(false);

				return;
			}
		}

		if (!param.isManualInputEnabled)
		{
			if (m_wWidget)
				m_wWidget.SetVisible(false);

			return;
		}

		//--- Change attachment on input
		switch (m_eAttachType)
		{
			case EAttachManualCameraType.INPUT:
			{
				if (param.isManualInputEnabled && GetInputManager().GetActionTriggered("ManualCameraAttach"))
				{
					param.GetCursorWorldPos(); //--- Update entity under cursor
					if (!AttachTo(param.target))
						Detach();
				}
				break;
			}

			case EAttachManualCameraType.PLAYER:
			{
				if (!m_AttachHelper)
					AttachTo(SCR_PlayerController.GetLocalMainEntity());

				break;
			}

			case EAttachManualCameraType.ENTITY:
			{
				if (!m_AttachHelper)
					AttachTo(SCR_PlayerController.GetLocalMainEntity());

				break;
			}
		}

		//--- Convert camera coordinates to local space
		if (m_bAttachChanged)
		{
			m_bAttachChanged = false;

			SCR_ManualCamera camera = GetCameraEntity();
			if (camera)
			{
				camera.GetLocalTransform(param.transform);
				camera.GetLocalTransform(param.transformOriginal);
				param.isDirty = true;
			}

			if (m_wWidget)
				m_wWidget.SetVisible(m_Target != null);
		}

		//--- Update GUI
		if (m_wWidget && m_Target)
		{
			m_wWidget.SetVisible(true);
			vector screenPos = m_wWidget.GetWorkspace().ProjWorldToScreen(m_AttachHelper.GetOrigin(), param.world);
			if (screenPos[2] > 0)
				FrameSlot.SetPos(m_wWidget, screenPos[0], screenPos[1]);
		}
	}

	//------------------------------------------------------------------------------------------------
	override bool EOnCameraInit()
	{
		s_Instance = this;

		m_wWidget = GetCameraEntity().CreateCameraWidget(m_sLayout, false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraExit()
	{
		if (m_wWidget)
			m_wWidget.RemoveFromHierarchy();

		delete m_AttachHelper;
	}
}
