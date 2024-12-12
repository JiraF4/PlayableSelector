BaseSource {
 AnimSetTemplate "{AFED9D355AB37777}anims/workspaces/player/player_main_PS_VON.ast"
 AnimSetInstances {
  "{B086D0CD3B9E3AB0}anims/workspaces/player/player_PS_VON.asi"
 }
 AnimGraph "{BD1FFB356D76A384}anims/workspaces/player/player_main_PS_VON.agr"
 PreviewModels {
  AnimSrcWorkspacePreviewModel "{563EF338E13AB791}" {
   Model "{790CB9C809DE64B8}Assets/Characters/Animation/AnimTestChar_US_01.xob"
  }
  AnimSrcWorkspacePreviewModel "{563EF338E13AB5EA}" {
   Model "{7297BDBDE223627F}Assets/Characters/Animation/AnimTestChar_USSR_01.xob"
  }
 }
 ChildPreviewModels {
  AnimSrcWorkspaceChildPreviewModel "{5A44BACB00F067DA}" {
   Enabled 0
   Model "{BA5449C606206A19}Assets/Weapons/Rifles/AK74/AK74_body.xob"
   Bone "RightHandProp"
   Parent "{563EF338E13AB791}"
   ChildBone "w_root"
  }
  AnimSrcWorkspaceChildPreviewModel "{5A76ED170BD46D3E}" {
   Model "{6F3FF53AF2AB04B3}Assets/Weapons/Handguns/PM/PM_body.xob"
   Bone "RightHandProp"
   Parent "{563EF338E13AB791}"
  }
 }
 AudioTesting AnimSrcWorkspaceAudioTesting "{58749017CEBF9BA9}" {
 }
 EventTable "{CCF5B98AA120E569}anims/workspaces/player/player_EventTable_PS_VON.ae"
 SyncTable "{DC61978ADAAEB032}anims/workspaces/player/Player_SyncTable_PS_VON.asy"
 AttachmentTesting AnimSrcWorkspaceAttachmentTesting "{6305039DC7AECF48}" {
 }
 IkTesting AnimSrcWorkspaceIkTesting "{52928355A81E5FAE}" {
  IkTargets {
   AnimSrcWorkspaceIkTargetTest "{52ACCA55AEBEECBE}" {
    Enabled 0
    BindingName "RFootIKTarget"
    Position 0.2 0.23 0
    Rotation 0 3.24 0.1
   }
   AnimSrcWorkspaceIkTargetTest "{52B6C1642C518E0E}" {
    Enabled 0
    BindingName "LFootIKTarget"
    Position -0.2 0.23 0
    Rotation 4 3.24 0.1
   }
  }
  IkPlanes {
   AnimSrcWorkspaceIkTargetPlaneTest "{55DEF0D15A324261}" {
    Enabled 0
    BindingName "FloorRight"
    Position 0 1 0
    Normal 0 1 0
   }
  }
 }
}