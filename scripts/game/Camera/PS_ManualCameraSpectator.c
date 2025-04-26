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
