enum PS_ESpectatorCameraMode
{
	FIRST_PERSON,
	GOPRO
}

[EntityEditorProps(category: "GameScripted/Camera", description: "Manual camera", color: "0 255 255 255")]
class PS_ManualCameraSpectatorClass : SCR_ManualCameraClass
{
}

class PS_ManualCameraSpectator : SCR_ManualCamera
{
	protected bool m_bMoveLink;
	protected IEntity m_CharacterEntity;
	protected vector oldTransform[4];
	protected float m_fDistance;

	protected PS_ESpectatorCameraMode m_eCameraMode = PS_ESpectatorCameraMode.FIRST_PERSON;

	PS_ESpectatorCameraMode GetCameraMode()
	{
		return m_eCameraMode;
	}

	void SetCameraMode(PS_ESpectatorCameraMode mode)
	{
		if (m_eCameraMode == mode)
			return;

		m_eCameraMode = mode;
		if (m_eCameraMode == PS_ESpectatorCameraMode.GOPRO && m_CharacterEntity)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_CharacterEntity);
			if (character)
			{
				SCR_CharacterCameraHandlerComponent handler = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
				if (handler)
					handler.OnAlphatestChange(0);
			}
		}
	}

	void ToggleCameraMode()
	{
		if (m_eCameraMode == PS_ESpectatorCameraMode.FIRST_PERSON)
			SetCameraMode(PS_ESpectatorCameraMode.GOPRO);
		else
			SetCameraMode(PS_ESpectatorCameraMode.FIRST_PERSON);
	}

	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);

		if (m_CharacterEntity)
			CameraPositionUpdate();
	}

	override protected bool IsDisabledByMenu()
	{
		if (m_bMoveLink)
		{
			if (m_fDistance > 0.6)
				return true;
		}
		return super.IsDisabledByMenu();
	}

	IEntity GetCharacterEntity()
	{
		return m_CharacterEntity;
	}
	
	void SetCharacterEntity(IEntity characterEntity)
	{
		ClearCharacterEntity();
		m_CharacterEntity = characterEntity;
		m_bMoveLink = false;
		
		GetTransform(oldTransform);
	}

	void SetCharacterEntityMove(IEntity characterEntity)
	{
		ClearCharacterEntity();
		m_CharacterEntity = characterEntity;
		
		m_bMoveLink = false;
		CameraPositionUpdate();
		m_bMoveLink = true;
	}

	void ClearCharacterEntity()
	{
		if (!m_CharacterEntity)
			return;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_CharacterEntity);
		if (character)
		{
			SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
			if (characterCameraHandlerComponent)
				characterCameraHandlerComponent.OnAlphatestChange(0);
		}

		m_CharacterEntity = null;
	}

	// Spectator jump-to-coordinates: place the free camera at a raw world position with no entity to
	// follow. Used when the clicked playable is outside this client's replication pool (default NDS
	// culling) so there is no local entity for SetCharacterEntity to track - the server sends only the
	// coordinates. SCR_ManualCamera re-reads the entity transform every frame (ProcessComponents ->
	// GetLocalTransform), so SetOrigin sticks and manual input continues from the new spot; SetOrigin
	// also keeps the camera's current orientation.
	void MoveToPosition(vector pos)
	{
		SetCharacterEntity(null);
		SetOrigin(pos);
	}

	// Place the camera 5 meters behind and 2 meters above a target position, oriented to look at it.
	// Used on death (camera behind corpse) and when spectating an un-replicated player.
	static const float SPECTATE_BEHIND_DISTANCE = 5.0;
	static const float SPECTATE_ABOVE_OFFSET = 2.0;
	void SetCameraBehindPosition(vector targetPos, vector targetForward)
	{
		SetCharacterEntity(null);

		vector backDir = -targetForward;
		backDir[1] = 0;
		// Ragdoll guard: if the corpse tumbled and its forward is near-vertical, the horizontal
		// component collapses to zero — fall back to a default "behind" direction.
		if (backDir.Length() < 0.01)
			backDir = vector.Forward;
		backDir = backDir.Normalized();

		vector cameraPos = targetPos + backDir * SPECTATE_BEHIND_DISTANCE + vector.Up * SPECTATE_ABOVE_OFFSET;

		// Terrain protection: clamp to surface Y so the camera never ends up underground
		float surfaceY = GetGame().GetWorld().GetSurfaceY(cameraPos[0], cameraPos[2]);
		if (cameraPos[1] < surfaceY + 1.0)
			cameraPos[1] = surfaceY + 1.0;

		// Build a transform that looks from cameraPos toward the target
		vector dir = targetPos - cameraPos;
		vector angles = dir.VectorToAngles();
		angles[2] = 0; // no roll
		vector mat[4];
		Math3D.AnglesToMatrix(angles, mat);
		mat[3] = cameraPos;

		SetTransform(mat);
	}

	protected void MountGoPro(ChimeraCharacter character, out vector outMat[4])
	{
		Animation anim = character.GetAnimation();
		if (!anim)
		{
			character.GetWorldTransform(outMat);
			return;
		}

		int boneHead = anim.GetBoneIndex("Head");
		if (boneHead < 0)
		{
			character.GetWorldTransform(outMat);
			return;
		}

		SCR_CharacterCameraHandlerComponent handler = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
		if (handler)
			handler.OnAlphatestChange(0);

		vector charTransform[4];
		character.GetWorldTransform(charTransform);

		vector headBoneTM[4];
		anim.GetBoneMatrix(boneHead, headBoneTM);

		vector headWorld[4];
		Math3D.MatrixMultiply4(charTransform, headBoneTM, headWorld);

		// Head bone in Enfusion model space is rotated 180 deg around yaw; undo so +X is right and +Z is forward.
		vector rot180[3] = { "-1 0 0", "0 1 0", "0 0 -1" };
		vector headForwardTM[4];
		Math3D.MatrixMultiply3(headWorld, rot180, headForwardTM);
		headForwardTM[3] = headWorld[3];

		vector localMount[4];
		Math3D.MatrixIdentity4(localMount);
		localMount[3] = "0.06 0.10 0.12"; // X: right, Y: up, Z: forward

		vector mountWorld[4];
		Math3D.MatrixMultiply4(headForwardTM, localMount, mountWorld);

		vector angles = Math3D.MatrixToAngles(headForwardTM);
		angles[2] = 0.0;
		Math3D.AnglesToMatrix(angles, outMat);

		TraceSphere trace = new TraceSphere();
		trace.Radius = 0.025;
		trace.Start = character.EyePosition();
		trace.End = mountWorld[3];
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.LayerMask = TRACE_LAYER_CAMERA;

		array<IEntity> excluded = { character };
		trace.ExcludeArray = excluded;

		float hit = character.GetWorld().TraceMove(trace, null);
		if (hit < 1.0)
			outMat[3] = vector.Lerp(trace.Start, trace.End, hit);
		else
			outMat[3] = mountWorld[3];
	}

	void CameraPositionUpdate()
	{
		vector newTransform[4];
		GetTransform(newTransform);
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!SCR_Math3D.MatrixEqual(newTransform, oldTransform) && !gameModeCoop.GetFriendliesSpectatorOnly() && !m_bMoveLink)
		{
			SetCharacterEntity(null);
			return;
		}

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_CharacterEntity);
		if (!character)
			return;

		vector targetMat[4];

		if (m_eCameraMode == PS_ESpectatorCameraMode.GOPRO)
		{
			MountGoPro(character, targetMat);
		}
		else
		{
			SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
			if (characterCameraHandlerComponent)
				characterCameraHandlerComponent.OnAlphatestChange(255);

			Animation anim = m_CharacterEntity.GetAnimation();
			if (!anim)
				return;

			int boneHead = anim.GetBoneIndex("Head");
			int boneEyeLeft = anim.GetBoneIndex("leftEye");
			vector mat[4];
			m_CharacterEntity.GetTransform(mat);
			vector matHead[4];
			anim.GetBoneMatrix(boneHead, matHead);
			vector matEyeLeft[4];
			anim.GetBoneMatrix(boneEyeLeft, matEyeLeft);
			vector matRes[4];
			Math3D.MatrixMultiply4(mat, matHead, matRes);
			vector matResEye[4];
			Math3D.MatrixMultiply4(mat, matEyeLeft, matResEye);
			vector matScale[3] = { "1 0 0", "0 1 0", "0 0 -1" };
			vector matRes2[4];
			Math3D.MatrixMultiply3(matRes, matScale, matRes2);
			vector angles = Math3D.MatrixToAngles(matRes2);
			angles[2] = 0.0;
			angles[1] = angles[1] + 10.0;
			angles[0] = angles[0] - 5.0;
			Math3D.AnglesToMatrix(angles, targetMat);
			targetMat[3] = matResEye[3];
		}
		
		if (!m_bMoveLink)
			SetTransform(targetMat);
		else
		{
			vector origin1 = newTransform[3];
			vector origin2 = targetMat[3];
			vector originDiff = origin2 - origin1;
			m_fDistance = originDiff.Length();
			if (m_fDistance > 0.5)
			{
				m_fDistance -= 0.5;
				Print(m_fDistance);
				vector moveVector = vector.Lerp(origin1, origin1 + originDiff.Normalized() * m_fDistance, GetGame().GetWorld().GetTimeSlice() * 5);
				newTransform[3] = moveVector;
				SetTransform(newTransform);
			}
		}

		GetTransform(oldTransform);
	}

	void ~PS_ManualCameraSpectator()
	{
		ClearCharacterEntity();
	}
}
