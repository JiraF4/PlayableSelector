[EntityEditorProps(category: "GameScripted/Camera", description: "Manual camera", color: "0 255 255 255")]
class PS_ManualCameraSpectatorClass: SCR_ManualCameraClass
{
};

class PS_ManualCameraSpectator: SCR_ManualCamera
{
	protected IEntity m_eCharacterEntity;
	protected vector oldTransform[4];
	
	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);
		
		if (m_eCharacterEntity)
			CameraPositionUpdate();
	}
	
	IEntity GetCharacterEntity()
	{
		return m_eCharacterEntity;
	}
	
	void SetCharacterEntity(IEntity characterEntity)
	{
		ClearCharacterEntity();	
		m_eCharacterEntity = characterEntity;
		
		GetTransform(oldTransform);
	}
	
	void ClearCharacterEntity()
	{
		if (!m_eCharacterEntity)
			return;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_eCharacterEntity);
		SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
		characterCameraHandlerComponent.OnAlphatestChange(0);
	}
	
	void CameraPositionUpdate()
	{
		vector newTransform[4];
		GetTransform(newTransform);
		if (!SCR_Math3D.MatrixEqual(newTransform, oldTransform))
		{
			SetCharacterEntity(null);
			return;
		}
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_eCharacterEntity);
		SCR_CharacterCameraHandlerComponent characterCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));
		characterCameraHandlerComponent.OnAlphatestChange(255);
		
		int boneHead = m_eCharacterEntity.GetAnimation().GetBoneIndex("Head");
		int boneEyeLeft = m_eCharacterEntity.GetAnimation().GetBoneIndex("EyeLeft");
		vector mat[4];
		m_eCharacterEntity.GetTransform(mat);
		vector matHead[4];
		m_eCharacterEntity.GetAnimation().GetBoneMatrix(boneHead, matHead);
		vector matEyeLeft[4];
		m_eCharacterEntity.GetAnimation().GetBoneMatrix(boneEyeLeft, matEyeLeft);
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
		SetTransform(matRes2);
		
		GetTransform(oldTransform);
	}
	
	void ~PS_ManualCameraSpectator()
	{
		ClearCharacterEntity();
	}
};

