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
		SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
		characterCameraHandlerComponent.OnAlphatestChange(0);
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
		SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
		characterCameraHandlerComponent.OnAlphatestChange(255);

		int boneHead = m_CharacterEntity.GetAnimation().GetBoneIndex("Head");
		int boneEyeLeft = m_CharacterEntity.GetAnimation().GetBoneIndex("leftEye");
		vector mat[4];
		m_CharacterEntity.GetTransform(mat);
		vector matHead[4];
		m_CharacterEntity.GetAnimation().GetBoneMatrix(boneHead, matHead);
		vector matEyeLeft[4];
		m_CharacterEntity.GetAnimation().GetBoneMatrix(boneEyeLeft, matEyeLeft);
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
		Math3D.AnglesToMatrix(angles, matRes2);
		matRes2[3] = matResEye[3];
		
		if (!m_bMoveLink)
			SetTransform(matRes2);
		else
		{
			vector origin1 = newTransform[3];
			vector origin2 = matRes2[3];
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
