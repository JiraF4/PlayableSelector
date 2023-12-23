AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "Distance"
   tl -131.901 317.197
   value 40
   valueMax 100
  }
  IOPInputValueClass {
   id 29
   name "Spatial Reverb INT [Amp]"
   tl -536.356 374.377
   children {
    61 62
   }
   value 0.2
  }
  IOPItemInputClass {
   id 35
   name "Interior"
   comment "Emitter Inside"
   tl -542.07 1090.578
   ctl 0 -21
   children {
    72
   }
   value 1
  }
  IOPItemInputClass {
   id 40
   name "GInterior"
   comment "Listener Inside"
   tl -293.77 1409.956
   ctl 0 -21
   children {
    60 61 63
   }
   value 1
   global 1
  }
  IOPInputValueClass {
   id 46
   name "Ambient Reverb INT [Amp]"
   tl -536.356 448.371
   children {
    67
   }
   value 0.2
  }
  IOPInputValueClass {
   id 53
   name "1"
   tl -536.356 949.207
   children {
    54
   }
   value 1
  }
  IOPInputValueClass {
   id 101
   name "1"
   tl -536.356 1267.5
   children {
    60
   }
   value 1
  }
  IOPItemInputClass {
   id 106
   name "DistanceObserver"
   tl -157.181 438.091
   children {
    99 104
   }
  }
 }
 Ops {
  IOPItemOpSubClass {
   id 54
   name "Sub 54"
   comment "Emitor Outside"
   tl -295.992 952.858
   ctl 0 -21
   children {
    63 64
   }
   inputs {
    ConnectionClass connection {
     id 72
     port 1
    }
    ConnectionClass connection {
     id 53
     port 0
    }
   }
  }
  IOPItemOpSubClass {
   id 60
   name "Sub 54"
   comment "Listener Outside"
   tl -293.77 1266.111
   ctl 0 -21
   children {
    62 64
   }
   inputs {
    ConnectionClass connection {
     id 101
     port 0
    }
    ConnectionClass connection {
     id 40
     port 1
    }
   }
  }
  IOPItemOpMulClass {
   id 61
   name "Mul 61"
   comment "Emitter Inside && Listener Inside"
   tl 122.46 976.984
   ctl 0 -21
   children {
    69
   }
   inputs {
    ConnectionClass connection {
     id 104
     port 0
    }
    ConnectionClass connection {
     id 72
     port 0
    }
    ConnectionClass connection {
     id 29
     port 0
    }
    ConnectionClass connection {
     id 40
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 62
   name "Mul 62"
   comment "Emitter Inside && Listener Outside"
   tl 118.333 1109.028
   ctl 0 -21
   children {
    69
   }
   inputs {
    ConnectionClass connection {
     id 60
     port 0
    }
    ConnectionClass connection {
     id 29
     port 0
    }
    ConnectionClass connection {
     id 72
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 63
   name "Mul 63"
   comment "Emitor Outside && Listener Inside"
   tl 113.889 1245.694
   ctl 0 -21
   children {
    39 67
   }
   inputs {
    ConnectionClass connection {
     id 40
     port 0
    }
    ConnectionClass connection {
     id 54
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 64
   name "Mul 64"
   comment "Emitor Outside && Listener Outside"
   tl 112.722 1508.306
   ctl 0 -21
   children {
    68
   }
   inputs {
    ConnectionClass connection {
     id 60
     port 0
    }
    ConnectionClass connection {
     id 54
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 67
   name "Mul 67"
   comment "Ambient Reveb"
   tl 422.976 1247.837
   ctl 0 -21
   children {
    69
   }
   inputs {
    ConnectionClass connection {
     id 46
     port 0
    }
    ConnectionClass connection {
     id 63
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 68
   name "Mul"
   comment "Spatial Reverb"
   tl 430.083 1592.333
   ctl 0 -21
   children {
    105
   }
   inputs {
    ConnectionClass connection {
     id 99
     port 0
    }
    ConnectionClass connection {
     id 64
     port 0
    }
   }
  }
  IOPItemOpSumClass {
   id 69
   name "Sum 69"
   tl 767.833 1096.167
   children {
    34
   }
   inputs {
    ConnectionClass connection {
     id 61
     port 0
    }
    ConnectionClass connection {
     id 62
     port 0
    }
    ConnectionClass connection {
     id 67
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 72
   name "Mul 72"
   comment "Emitter Inside"
   tl -294.742 1088.555
   ctl 0 -21
   children {
    54 61 62
   }
   inputs {
    ConnectionClass connection {
     id 35
     port 0
    }
   }
  }
  IOPItemOpInterpolateClass {
   id 99
   name "Interpolate EXT"
   comment "Reverb EXT Distance Adjustment"
   tl 120.833 644.06
   ctl 0 -21
   children {
    68
   }
   inputs {
    ConnectionClass connection {
     id 106
     port 0
    }
   }
   "X min" 3
   "X max" 300
  }
  IOPItemOpInterpolateClass {
   id 104
   name "Interpolate INT"
   comment "Reverb INT Distance Adjustment"
   tl 120.528 452.278
   ctl 0 -21
   children {
    61
   }
   inputs {
    ConnectionClass connection {
     id 106
     port 0
    }
   }
   EnableCustomCurve 1
   CustomCurve {
    CurvePoint "1" {
     Y 1
    }
    CurvePoint "2" {
     X 40
     Y 4.5
    }
    CurvePoint "3" {
     X 50
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 34
   name "Reverb_INT_V"
   tl 978.135 1108.277
   input 69
  }
  IOPItemOutputClass {
   id 39
   name "Reverb_INT_M"
   tl 978.135 1365.067
   input 63
  }
  IOPItemOutputClass {
   id 105
   name "Reverb_EXT_V"
   tl 983 1584.333
   input 68
  }
 }
 groups {
  GroupClass {
   id 47
   name "Setup"
   children {
    11 5 17 3 29 46 90 91
   }
   parent -1
   Color 0.6 0.6 0.6 0.251
  }
  GroupClass {
   id 103
   name "Emitor/Listene combination"
   children {
    64 62 61 63
   }
   parent -1
   Color 0.6 0.6 0.6 0.251
  }
  GroupClass {
   id 102
   name "Emitor/Listeners position"
   children {
    60 40 72 54
   }
   parent -1
   Color 0.6 0.6 0.6 0.251
  }
  GroupClass {
   id 105
   name "Distance Behaviour"
   children {
    99 104
   }
   parent -1
   Color 0.6 0.6 0.6 0.251
  }
 }
 compiled IOPCompiledClass {
  version 2
 }
}