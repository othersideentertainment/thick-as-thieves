import maya.cmds as cmds
import Otherside.Rigging.RigUtility as RigUtility
from Otherside.Rigging.ControlRig.Arm import Arm
from Otherside.Rigging.ControlRig.Finger import Finger
from Otherside.Rigging.ControlRig.Hand import Hand
from Otherside.Rigging.ControlRig.Head import Head
from Otherside.Rigging.ControlRig.Leg import Leg
from Otherside.Rigging.ControlRig.Placement import Placement
from Otherside.Rigging.ControlRig.Prop import Prop
from Otherside.Rigging.ControlRig.Shoulder import Shoulder
from Otherside.Rigging.ControlRig.Torso import Torso


def getDefaultSpaces():
    #return [ ["body","pelvis"], ["chest","spine_03"]]
    return {"body":"pelvis","chest":"spine_03"}


def firstPerson(**kwargs):
    parent = kwargs.get("p", None)
    character_name = kwargs.get("n", "CONTROL_RIG")
    #- Control Rig Group
    characterRoot = cmds.createNode("transform", n=character_name, p=parent)
    cmds.setAttr(characterRoot+".hideOnPlayback", cb=True)
    #- placement
    placement = Placement("Placement")
    placement.boneRoot = "root"
    placement.rig(p=characterRoot)
    #- Torso
    instance = Torso("Torso")
    instance.spaces = getDefaultSpaces()
    instance.bonePelvis = "pelvis_1p"
    instance.boneSpine1 = "spine_01_1p"
    instance.boneSpine2 = "spine_02_1p"
    instance.boneSpine3 = "spine_03_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder L
    instance = Shoulder("Shoulder_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_l_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder R
    instance = Shoulder("Shoulder_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_r_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm L
    instance = Arm("Arm_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_l_1p"
    instance.boneLowerArm = "lowerarm_l_1p"
    instance.boneHand = "hand_l_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm R
    instance = Arm("Arm_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_r_1p"
    instance.boneLowerArm = "lowerarm_r_1p"
    instance.boneHand = "hand_r_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Hand L
    ''' Hand'''
    instance = Hand("Hand_L", side=1)
    instance.boneHand = "hand_l_1p"
    instance.spaces = getDefaultSpaces()
    instance.boneMetacarpalIndex = "index_metacarpal_l_1p"
    instance.boneMetacarpalMiddle = "middle_metacarpal_l_1p"
    instance.boneMetacarpalRing = "ring_metacarpal_l_1p"
    instance.boneMetacarpalPinky = "pinky_metacarpal_l_1p"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_l_1p"
    instance.thumb.boneFingerB = "thumb_02_l_1p"
    instance.thumb.boneFingerC = "thumb_03_l_1p"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_l_1p"
    instance.index.boneFingerB = "index_02_l_1p"
    instance.index.boneFingerC = "index_03_l_1p"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_l_1p"
    instance.middle.boneFingerB = "middle_02_l_1p"
    instance.middle.boneFingerC = "middle_03_l_1p"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_l_1p"
    instance.ring.boneFingerB = "ring_02_l_1p"
    instance.ring.boneFingerC = "ring_03_l_1p"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_l_1p"
    instance.pinky.boneFingerB = "pinky_02_l_1p"
    instance.pinky.boneFingerC = "pinky_03_l_1p"
    ''' Rig '''
    instance.rig(p=characterRoot)
    #- Hand R
    ''' Hand'''
    instance = Hand("Hand_R", side=2)
    instance.spaces = getDefaultSpaces()
    instance.boneHand = "hand_r_1p"
    instance.boneMetacarpalIndex = "index_metacarpal_r_1p"
    instance.boneMetacarpalMiddle = "middle_metacarpal_r_1p"
    instance.boneMetacarpalRing = "ring_metacarpal_r_1p"
    instance.boneMetacarpalPinky = "pinky_metacarpal_r_1p"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_r_1p"
    instance.thumb.boneFingerB = "thumb_02_r_1p"
    instance.thumb.boneFingerC = "thumb_03_r_1p"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_r_1p"
    instance.index.boneFingerB = "index_02_r_1p"
    instance.index.boneFingerC = "index_03_r_1p"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_r_1p"
    instance.middle.boneFingerB = "middle_02_r_1p"
    instance.middle.boneFingerC = "middle_03_r_1p"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_r_1p"
    instance.ring.boneFingerB = "ring_02_r_1p"
    instance.ring.boneFingerC = "ring_03_r_1p"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_r_1p"
    instance.pinky.boneFingerB = "pinky_02_r_1p"
    instance.pinky.boneFingerC = "pinky_03_r_1p"
    ''' Rig'''
    instance.rig(p=characterRoot)
    #- Weapon L (Prop)
    instance = Prop("Weapon_L")
    instance.side = 1
    instance.spaces = [ ["body","pelvis_1p"], ["chest","spine_03_1p"], ["hand","hand_l_1p"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_l_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon R (Prop)
    instance = Prop("Weapon_R")
    instance.side = 2
    instance.spaces = [ ["body","pelvis_1p"], ["chest","spine_03_1p"], ["hand","hand_r_1p"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_r_1p"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)


def biped(**kwargs):
    parent = kwargs.get("p", None)
    character_name = kwargs.get("n", "CONTROL_RIG")
    #- Control Rig Group
    characterRoot = cmds.createNode("transform", n=character_name, p=parent)
    cmds.setAttr(characterRoot+".hideOnPlayback", cb=True)
    #- placement
    placement = Placement("Placement")
    placement.boneRoot = "root"
    placement.rig(p=characterRoot)
    #- Head
    instance = Head("Head")
    instance.spaces = getDefaultSpaces()
    instance.boneNeck = "neck_01"
    instance.boneHead = "head"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Torso
    instance = Torso("Torso")
    instance.spaces = getDefaultSpaces()
    instance.bonePelvis = "pelvis"
    instance.boneSpine1 = "spine_01"
    instance.boneSpine2 = "spine_02"
    instance.boneSpine3 = "spine_03"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder L
    instance = Shoulder("Shoulder_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_l"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder R
    instance = Shoulder("Shoulder_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_r"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm L
    instance = Arm("Arm_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_l"
    instance.boneLowerArm = "lowerarm_l"
    instance.boneHand = "hand_l"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm R
    instance = Arm("Arm_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_r"
    instance.boneLowerArm = "lowerarm_r"
    instance.boneHand = "hand_r"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Hand L
    ''' Hand'''
    instance = Hand("Hand_L", side=1)
    instance.boneHand = "hand_l"
    instance.spaces = getDefaultSpaces()
    instance.boneMetacarpalIndex = "index_metacarpal_l"
    instance.boneMetacarpalMiddle = "middle_metacarpal_l"
    instance.boneMetacarpalRing = "ring_metacarpal_l"
    instance.boneMetacarpalPinky = "pinky_metacarpal_l"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_l"
    instance.thumb.boneFingerB = "thumb_02_l"
    instance.thumb.boneFingerC = "thumb_03_l"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_l"
    instance.index.boneFingerB = "index_02_l"
    instance.index.boneFingerC = "index_03_l"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_l"
    instance.middle.boneFingerB = "middle_02_l"
    instance.middle.boneFingerC = "middle_03_l"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_l"
    instance.ring.boneFingerB = "ring_02_l"
    instance.ring.boneFingerC = "ring_03_l"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_l"
    instance.pinky.boneFingerB = "pinky_02_l"
    instance.pinky.boneFingerC = "pinky_03_l"
    ''' Rig '''
    instance.rig(p=characterRoot)
    #- Hand R
    ''' Hand'''
    instance = Hand("Hand_R", side=2)
    instance.spaces = getDefaultSpaces()
    instance.boneHand = "hand_r"
    instance.boneMetacarpalIndex = "index_metacarpal_r"
    instance.boneMetacarpalMiddle = "middle_metacarpal_r"
    instance.boneMetacarpalRing = "ring_metacarpal_r"
    instance.boneMetacarpalPinky = "pinky_metacarpal_r"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_r"
    instance.thumb.boneFingerB = "thumb_02_r"
    instance.thumb.boneFingerC = "thumb_03_r"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_r"
    instance.index.boneFingerB = "index_02_r"
    instance.index.boneFingerC = "index_03_r"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_r"
    instance.middle.boneFingerB = "middle_02_r"
    instance.middle.boneFingerC = "middle_03_r"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_r"
    instance.ring.boneFingerB = "ring_02_r"
    instance.ring.boneFingerC = "ring_03_r"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_r"
    instance.pinky.boneFingerB = "pinky_02_r"
    instance.pinky.boneFingerC = "pinky_03_r"
    ''' Rig'''
    instance.rig(p=characterRoot)
    #- Leg L
    instance = Leg("Leg_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperLeg = "thigh_l"
    instance.boneLowerLeg = "calf_l"
    instance.boneFoot = "foot_l"
    instance.boneToe = "ball_l"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Leg R
    instance = Leg("Leg_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperLeg = "thigh_r"
    instance.boneLowerLeg = "calf_r"
    instance.boneFoot = "foot_r"
    instance.boneToe = "ball_r"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon L (Prop)
    instance = Prop("Weapon_L")
    instance.side = 1
    instance.spaces = [ ["body","pelvis"], ["chest","spine_03"], ["hand","hand_l"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_l"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon R (Prop)
    instance = Prop("Weapon_R")
    instance.side = 2
    instance.spaces = [ ["body","pelvis"], ["chest","spine_03"], ["hand","hand_r"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_r"
    instance.rig(p=characterRoot)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)



def player(**kwargs):
    parent = kwargs.get("p", None)
    character_name = kwargs.get("n", "CONTROL_RIG")
    #- Control Rig Group
    characterRoot = cmds.createNode("transform", n=character_name, p=parent)
    cmds.setAttr(characterRoot+".hideOnPlayback", cb=True)
    #-------------------------------------- 3RD PERSON --------------------------------------
    thirdPerson = cmds.createNode("transform", n="thirdPerson", p=characterRoot)
    #- placement
    placement = Placement("Placement")
    placement.boneRoot = "root"
    placement.rig(p=thirdPerson)
    #- Head
    instance = Head("Head")
    instance.spaces = getDefaultSpaces()
    instance.boneNeck = "neck_01"
    instance.boneHead = "head"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Torso
    instance = Torso("Torso")
    instance.spaces = getDefaultSpaces()
    instance.bonePelvis = "pelvis"
    instance.boneSpine1 = "spine_01"
    instance.boneSpine2 = "spine_02"
    instance.boneSpine3 = "spine_03"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder L
    instance = Shoulder("Shoulder_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_l"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder R
    instance = Shoulder("Shoulder_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_r"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm L
    instance = Arm("Arm_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_l"
    instance.boneLowerArm = "lowerarm_l"
    instance.boneHand = "hand_l"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm R
    instance = Arm("Arm_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_r"
    instance.boneLowerArm = "lowerarm_r"
    instance.boneHand = "hand_r"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Hand L
    ''' Hand'''
    instance = Hand("Hand_L", side=1)
    instance.boneHand = "hand_l"
    instance.spaces = getDefaultSpaces()
    instance.boneMetacarpalIndex = "index_metacarpal_l"
    instance.boneMetacarpalMiddle = "middle_metacarpal_l"
    instance.boneMetacarpalRing = "ring_metacarpal_l"
    instance.boneMetacarpalPinky = "pinky_metacarpal_l"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_l"
    instance.thumb.boneFingerB = "thumb_02_l"
    instance.thumb.boneFingerC = "thumb_03_l"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_l"
    instance.index.boneFingerB = "index_02_l"
    instance.index.boneFingerC = "index_03_l"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_l"
    instance.middle.boneFingerB = "middle_02_l"
    instance.middle.boneFingerC = "middle_03_l"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_l"
    instance.ring.boneFingerB = "ring_02_l"
    instance.ring.boneFingerC = "ring_03_l"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_l"
    instance.pinky.boneFingerB = "pinky_02_l"
    instance.pinky.boneFingerC = "pinky_03_l"
    ''' Rig '''
    instance.rig(p=thirdPerson)
    #- Hand R
    ''' Hand'''
    instance = Hand("Hand_R", side=2)
    instance.spaces = getDefaultSpaces()
    instance.boneHand = "hand_r"
    instance.boneMetacarpalIndex = "index_metacarpal_r"
    instance.boneMetacarpalMiddle = "middle_metacarpal_r"
    instance.boneMetacarpalRing = "ring_metacarpal_r"
    instance.boneMetacarpalPinky = "pinky_metacarpal_r"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_r"
    instance.thumb.boneFingerB = "thumb_02_r"
    instance.thumb.boneFingerC = "thumb_03_r"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_r"
    instance.index.boneFingerB = "index_02_r"
    instance.index.boneFingerC = "index_03_r"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_r"
    instance.middle.boneFingerB = "middle_02_r"
    instance.middle.boneFingerC = "middle_03_r"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_r"
    instance.ring.boneFingerB = "ring_02_r"
    instance.ring.boneFingerC = "ring_03_r"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_r"
    instance.pinky.boneFingerB = "pinky_02_r"
    instance.pinky.boneFingerC = "pinky_03_r"
    ''' Rig'''
    instance.rig(p=thirdPerson)
    #- Leg L
    instance = Leg("Leg_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperLeg = "thigh_l"
    instance.boneLowerLeg = "calf_l"
    instance.boneFoot = "foot_l"
    instance.boneToe = "ball_l"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Leg R
    instance = Leg("Leg_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperLeg = "thigh_r"
    instance.boneLowerLeg = "calf_r"
    instance.boneFoot = "foot_r"
    instance.boneToe = "ball_r"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon L (Prop)
    instance = Prop("Weapon_L")
    instance.side = 1
    instance.spaces = [ ["body","pelvis"], ["chest","spine_03"], ["hand","hand_l"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_l"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon R (Prop)
    instance = Prop("Weapon_R")
    instance.side = 2
    instance.spaces = [ ["body","pelvis"], ["chest","spine_03"], ["hand","hand_r"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_r"
    instance.rig(p=thirdPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #-------------------------------------- 1ST PERSON --------------------------------------
    firstPerson = cmds.createNode("transform", n="firstPerson", p=characterRoot)
    #- Torso
    instance = Torso("Torso")
    instance.spaces = getDefaultSpaces()
    instance.bonePelvis = "pelvis_1p"
    instance.boneSpine1 = "spine_01_1p"
    instance.boneSpine2 = "spine_02_1p"
    instance.boneSpine3 = "spine_03_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder L
    instance = Shoulder("Shoulder_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_l_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Shoulder R
    instance = Shoulder("Shoulder_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneShoulder = "clavicle_r_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm L
    instance = Arm("Arm_L")
    instance.side = 1
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_l_1p"
    instance.boneLowerArm = "lowerarm_l_1p"
    instance.boneHand = "hand_l_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Arm R
    instance = Arm("Arm_R")
    instance.side = 2
    instance.spaces = getDefaultSpaces()
    instance.boneUpperArm = "upperarm_r_1p"
    instance.boneLowerArm = "lowerarm_r_1p"
    instance.boneHand = "hand_r_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Hand L
    ''' Hand'''
    instance = Hand("Hand_L", side=1)
    instance.boneHand = "hand_l_1p"
    instance.spaces = getDefaultSpaces()
    instance.boneMetacarpalIndex = "index_metacarpal_l_1p"
    instance.boneMetacarpalMiddle = "middle_metacarpal_l_1p"
    instance.boneMetacarpalRing = "ring_metacarpal_l_1p"
    instance.boneMetacarpalPinky = "pinky_metacarpal_l_1p"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_l_1p"
    instance.thumb.boneFingerB = "thumb_02_l_1p"
    instance.thumb.boneFingerC = "thumb_03_l_1p"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_l_1p"
    instance.index.boneFingerB = "index_02_l_1p"
    instance.index.boneFingerC = "index_03_l_1p"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_l_1p"
    instance.middle.boneFingerB = "middle_02_l_1p"
    instance.middle.boneFingerC = "middle_03_l_1p"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_l_1p"
    instance.ring.boneFingerB = "ring_02_l_1p"
    instance.ring.boneFingerC = "ring_03_l_1p"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_l_1p"
    instance.pinky.boneFingerB = "pinky_02_l_1p"
    instance.pinky.boneFingerC = "pinky_03_l_1p"
    ''' Rig '''
    instance.rig(p=firstPerson)
    #- Hand R
    ''' Hand'''
    instance = Hand("Hand_R", side=2)
    instance.spaces = getDefaultSpaces()
    instance.boneHand = "hand_r_1p"
    instance.boneMetacarpalIndex = "index_metacarpal_r_1p"
    instance.boneMetacarpalMiddle = "middle_metacarpal_r_1p"
    instance.boneMetacarpalRing = "ring_metacarpal_r_1p"
    instance.boneMetacarpalPinky = "pinky_metacarpal_r_1p"
    ''' Finger Thumb'''
    instance.thumb.boneFingerA = "thumb_01_r_1p"
    instance.thumb.boneFingerB = "thumb_02_r_1p"
    instance.thumb.boneFingerC = "thumb_03_r_1p"
    ''' Finger Index'''
    instance.index.boneFingerA = "index_01_r_1p"
    instance.index.boneFingerB = "index_02_r_1p"
    instance.index.boneFingerC = "index_03_r_1p"
    ''' Finger Middle'''
    instance.middle.boneFingerA = "middle_01_r_1p"
    instance.middle.boneFingerB = "middle_02_r_1p"
    instance.middle.boneFingerC = "middle_03_r_1p"
    ''' Finger Rig'''
    instance.ring.boneFingerA = "ring_01_r_1p"
    instance.ring.boneFingerB = "ring_02_r_1p"
    instance.ring.boneFingerC = "ring_03_r_1p"
    ''' Finger Pinky'''
    instance.pinky.boneFingerA = "pinky_01_r_1p"
    instance.pinky.boneFingerB = "pinky_02_r_1p"
    instance.pinky.boneFingerC = "pinky_03_r_1p"
    ''' Rig'''
    instance.rig(p=firstPerson)
    #- Weapon L (Prop)
    instance = Prop("Weapon_L")
    instance.side = 1
    instance.spaces = [ ["body","pelvis_1p"], ["chest","spine_03_1p"], ["hand","hand_l_1p"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_l_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Weapon R (Prop)
    instance = Prop("Weapon_R")
    instance.side = 2
    instance.spaces = [ ["body","pelvis_1p"], ["chest","spine_03_1p"], ["hand","hand_r_1p"] ]
    instance.defaultSpace = "hand"
    instance.boneProp = "weapon_r_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #- Camera (Prop)
    instance = Prop("Camera")
    instance.side = 2
    instance.spaces = [ ["chest","spine_03_1p"], ["body","pelvis_1p"]]
    instance.defaultSpace = "chest"
    instance.boneProp = "camera_1p"
    instance.rig(p=firstPerson)
    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement)
    #Rig Visual Switch
    cmds.addAttr(characterRoot, ln="thirdPerson", at="bool", k=False)
    cmds.addAttr(characterRoot, ln="firstPerson", at="bool", k=False)
    cmds.setAttr(characterRoot+".thirdPerson", e=True, cb=True)
    cmds.setAttr(characterRoot+".firstPerson", e=True, cb=True)
    cmds.setAttr(characterRoot+".firstPerson", False)
    cmds.setAttr(characterRoot+".thirdPerson", True)
    cmds.connectAttr(characterRoot+".firstPerson", firstPerson+".visibility")
    cmds.connectAttr(characterRoot+".thirdPerson", thirdPerson+".visibility")

