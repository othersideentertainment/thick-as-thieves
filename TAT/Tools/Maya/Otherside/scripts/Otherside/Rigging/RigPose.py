import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.ControlRig.Character as Character
from Otherside.Rigging.ControlRig.Arm import Arm
from Otherside.Rigging.ControlRig.Hand import Hand
from Otherside.Rigging.ControlRig.Leg import Leg
from Otherside.Rigging.ControlRig.HindLeg import HindLeg



def checkBoneList(boneList):
    if boneList == None:
        return False
    valid=True
    for bone in boneList:
        if bone == None or cmds.objExists(bone) == False:
            valid=False
            break
    return valid


def poseArm(boneList):
    if checkBoneList(boneList) == False:
        return
    offsetList = [5,-5]
    direction = 1
    #Check Side
    pos = cmds.xform(boneList[0], q=True, ws=True, t=True)
    if pos[0] < 0:
        direction = -1
    # Apply Pose
    for i in range(1,len(boneList)):
        boneA = boneList[i-1]
        boneB = boneList[i]
        offset = offsetList[i-1] * direction
        # Get Current Vector
        vectorA = OpenMaya.MVector(cmds.xform(boneA, q=True, ws=True, t=True))
        vectorB = OpenMaya.MVector(cmds.xform(boneB, q=True, ws=True, t=True))
        vectorAB = vectorB - vectorA
        # Calculate Target Vector
        length = vectorAB.length()
        vectorB2 = vectorA + OpenMaya.MVector(direction * length, 0,0)
        vectorAB2 = vectorB2 - vectorA
        # Apply Rotation to make arm straight
        angle = cmds.angleBetween(er=True, v1=vectorAB, v2=vectorAB2)
        cmds.xform(boneA, r=True, ws=True, ro=angle)
        # Apply offset
        cmds.xform(boneA, r=True, ws=True, ro=[0,offset,0])



def poseLeg(boneList):
    if checkBoneList(boneList) == False:
        return
    for i in range(1, len(boneList)):
        boneA = boneList[i-1]
        boneB = boneList[i]
        vectorA = OpenMaya.MVector(cmds.xform(boneA, q=True, ws=True, t=True))
        vectorB = OpenMaya.MVector(cmds.xform(boneB, q=True, ws=True, t=True))
        vectorAB = vectorB - vectorA
        vectorAB2 = OpenMaya.MVector(0, vectorAB.y, vectorAB.z)
        angle = cmds.angleBetween(er=True, v1=vectorAB, v2=vectorAB2)
        cmds.xform(boneA, r=True, ws=True, ro=angle)


def poseHindLeg(boneList):
    if checkBoneList(boneList) == False:
        return
    for i in range(1,len(boneList)):
        boneA = boneList[i-1]
        boneB = boneList[i]
        vectorA = OpenMaya.MVector(cmds.xform(boneA, q=True, ws=True, t=True))
        vectorB = OpenMaya.MVector(cmds.xform(boneB, q=True, ws=True, t=True))
        vectorAB = vectorB - vectorA
        vectorAB2 = OpenMaya.MVector(0, vectorAB.y, vectorAB.z)
        angle = cmds.angleBetween(er=True, v1=vectorAB, v2=vectorAB2)
        cmds.xform(boneA, r=True, ws=True, ro=angle)


def poseHand(boneList):
    if checkBoneList(boneList) == False:
        return
    #Align Vertically with Middle
    vectorA = OpenMaya.MVector(cmds.xform(boneList[0], q=True, ws=True, t=True))
    vectorB = OpenMaya.MVector(cmds.xform(boneList[1], q=True, ws=True, t=True))
    vectorAB = vectorB - vectorA
    vectorAB2 = OpenMaya.MVector(vectorAB.x, 0, vectorAB.z)
    angle = cmds.angleBetween(er=True, v1=vectorAB, v2=vectorAB2)
    cmds.xform(boneList[0], r=True, ws=True, ro=angle)
    #Align Horizontally with Middle
    vectorA = OpenMaya.MVector(cmds.xform(boneList[0], q=True, ws=True, t=True))
    vectorB = OpenMaya.MVector(cmds.xform(boneList[1], q=True, ws=True, t=True))
    vectorAB = vectorA - vectorB
    vectorAB2 = OpenMaya.MVector(vectorAB.x, 0, 0)
    angle = cmds.angleBetween(er=True, v1=vectorAB, v2=vectorAB2)
    cmds.xform(boneList[0], r=True, ws=True, ro=angle)


def assumeRigPose_v2(characterNode, *args, **kwargs):
    # load and update the character instance before processing
    chr_instance = Character.load(characterNode)
    chr_instance.updateModuleList()

    # internal helper functions
    def get_mod_node(character, modName):
        if character == None:
            return None
        instance = character.getModuleByName(modName)
        if instance == None:
            return None
        return instance.node

    def get_arm_bones(node):
        if node == None:
            return None
        instance = Arm.load(node)
        if instance == None:
            return None
        return [instance.boneUpperArm,instance.boneLowerArm,instance.boneHand]

    def get_hand_bones(node):
        if node == None:
            return None
        instance = Hand.load(node)
        if instance == None:
            return None
        # we need to do some additional checks here to get a bone from a subrig
        boneMiddle = None
        if instance.middle != None:
            boneMiddle = instance.middle.boneFingerA
        return [instance.boneHand, boneMiddle]

    def get_leg_bones(node):
        if node == None:
            return None
        instance = Leg.load(node)
        if instance == None:
            return None
        return [instance.boneUpperLeg, instance.boneLowerLeg, instance.boneFoot, instance.boneToe]

    def get_hind_leg_bones(node):
        if node == None:
            return None
        instance = HindLeg.load(node)
        if instance == None:
            return None
        return [instance.boneUpperLeg, instance.boneLowerLeg, instance.boneHock, instance.boneFoot, instance.boneToe]

    # Build Bone Map
    bonemap = {}
    # |- Arm L
    node = get_mod_node(chr_instance, "arm_L")
    bonemap["arm_L"] = get_arm_bones(node)
    # |- Arm R
    node = get_mod_node(chr_instance, "arm_R")
    bonemap["arm_R"] = get_arm_bones(node)
    # |- Hand L
    node = get_mod_node(chr_instance, "hand_L")
    bonemap["hand_L"] = get_hand_bones(node)
    # |- Hand R
    node = get_mod_node(chr_instance, "hand_R")
    bonemap["hand_R"] = get_hand_bones(node)
    # |- Leg L
    node = get_mod_node(chr_instance, "leg_L")
    bonemap["leg_L"] = get_leg_bones(node)
    # |- Leg R
    node = get_mod_node(chr_instance, "leg_R")
    bonemap["leg_R"] = get_leg_bones(node)
    # |- Hind Leg L
    node = get_mod_node(chr_instance, "hind_leg_L")
    bonemap["hind_leg_L"] = get_hind_leg_bones(node)
    # |- Hind Leg R
    node = get_mod_node(chr_instance, "hind_leg_R")
    bonemap["hind_leg_R"] = get_hind_leg_bones(node)
    # Apply the Pose
    poseArm(bonemap["arm_L"])
    poseArm(bonemap["arm_R"])
    poseHand(bonemap["hand_L"])
    poseHand(bonemap["hand_R"])
    poseLeg(bonemap["leg_L"])
    poseLeg(bonemap["leg_R"])
    poseHindLeg(bonemap["hind_leg_L"])
    poseHindLeg(bonemap["hind_leg_R"])


def assumeRigPose(*args, **kwargs):
    root = kwargs.get("root", None)
    #Default bone names
    arm_l_bones = ["upperarm_l", "lowerarm_l", "hand_l"]
    arm_r_bones = ["upperarm_r", "lowerarm_r", "hand_r"]
    arm_l_1p_bones = ["upperarm_l_1p", "lowerarm_l_1p", "hand_l_1p"]
    arm_r_1p_bones = ["upperarm_r_1p", "lowerarm_r_1p", "hand_r_1p"]
    leg_l_bones = ["thigh_l", "calf_l",  "foot_l", "ball_l"]
    leg_r_bones= ["thigh_r", "calf_r", "foot_r", "ball_r"]
    hind_leg_l_bones = ["thigh_hind_l", "calf_hind_l", "heel_hind_l", "foot_hind_l", "ball_hind_l"]
    hind_leg_r_bones= ["thigh_hind_r", "calf_hind_r", "heel_hind_r", "foot_hind_r", "ball_hind_r"]
    hand_l_bones = ["hand_l", "middle_01_l"]
    hand_r_bones = ["hand_r", "middle_01_r"]
    hand_l_1p_bones = ["hand_l_1p", "middle_01_l_1p"]
    hand_r_1p_bones = ["hand_r_1p", "middle_01_r_1p"]
    #Update the list to items beneath the root
    if root != None:
        arm_l_bones = _updateBoneListUnderRoot(root, arm_l_bones)
        arm_r_bones = _updateBoneListUnderRoot(root, arm_r_bones)
        arm_l_1p_bones = _updateBoneListUnderRoot(root, arm_l_1p_bones)
        arm_r_1p_bones = _updateBoneListUnderRoot(root, arm_r_1p_bones)
        hand_l_bones = _updateBoneListUnderRoot(root, hand_l_bones)
        hand_r_bones = _updateBoneListUnderRoot(root, hand_r_bones)
        hand_l_1p_bones = _updateBoneListUnderRoot(root, hand_l_1p_bones)
        hand_r_1p_bones = _updateBoneListUnderRoot(root, hand_r_1p_bones)
        leg_l_bones = _updateBoneListUnderRoot(root, leg_l_bones)
        leg_r_bones = _updateBoneListUnderRoot(root, leg_r_bones)
        hind_leg_l_bones = _updateBoneListUnderRoot(root, hind_leg_l_bones)
        hind_leg_r_bones = _updateBoneListUnderRoot(root, hind_leg_r_bones)
    #Apply the Pose
    poseArm(arm_l_bones)
    poseArm(arm_r_bones)
    poseArm(arm_l_1p_bones)
    poseArm(arm_r_1p_bones)
    poseHand(hand_l_bones)
    poseHand(hand_r_bones)
    poseHand(hand_l_1p_bones)
    poseHand(hand_r_1p_bones)
    poseLeg(leg_l_bones)
    poseLeg(leg_r_bones)
    poseHindLeg(hind_leg_l_bones)
    poseHindLeg(hind_leg_r_bones)


def _updateBoneListUnderRoot(root, boneList):
    for i in range(0, len(boneList)):
        boneList[i] = RigUtility.findObjectInHierarchy(boneList[i], root)
    return boneList


def setBoneRotationsToZero(*args, **kwargs):
    #root = kwargs.get("root", "root")
    #root = cmds.ls(root, recursive=True)[0]
    #bones = cmds.listRelatives(root, allDescendents=True, path=True, type='joint')
    bones = cmds.ls(recursive=True, type='joint')
    for bone in bones:
        try:
            cmds.setAttr(bone+".rotate", 0,0,0)
        except:
            print('Skipping bone "{}"'.format(bone))