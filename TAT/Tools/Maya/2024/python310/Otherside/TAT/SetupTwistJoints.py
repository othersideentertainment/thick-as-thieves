#######################################################################
# Add Twist Joints
#
# This script cleans up and creates twist joints in a basic skeleton.
#
# 1. Req: Skeleton with 'basic' template joint names (see ARTv1 //MayaTools/General/ART/SkeletonTemplates/basic.txt
# 2. Run script
# 3. Result: Add twist joints for thigh, 2 forearm, 2 upperarm, and neck
# TODO: Integrate into the Post Build script for ARTv1
#
# Owner: Horse
#######################################################################

import maya.cmds as cmds

upperarmTwistJoints = [u'upperarm_twist_01_l', u'upperarm_twist_02_l', u'upperarm_twist_01_r', u'upperarm_twist_02_r']
lowerarmTwistJoints = [u'lowerarm_twist_01_l', u'lowerarm_twist_02_l', u'lowerarm_twist_01_r', u'lowerarm_twist_02_r']
otherTwistJoints = [u'neck_twist']
thighTwistJoints = [u'thigh_twist_01_l', u'thigh_twist_01_r']
hindTwistJoints = [u'thigh_twist_01_hind_l', u'thigh_twist_01_hind_r']
helperTwistJoints = [u'lowerarm_helper_01', u'lowerarm_helper_02', u'lowerarm_helper_03', u'lowerarm_helper_04']
helperUpperTwistJoints = [u'upperarm_helper_01', u'upperarm_helper_02', u'upperarm_helper_03', u'upperarm_helper_04']

# toggle local axis display
def setLocalAxisDisplay(display=False):
   jointList = cmds.ls(sl=1, type='joint')
   for jnt in jointList:
      cmds.setAttr(jnt + '.displayLocalAxis', display)

def setRotateOrder(list,int,str): # 0 is xyz, 3 is xzy
    for each in list :
        cmds.setAttr(each+str+'.rotateOrder', int)

# create joints
def createJoint(name):
   cmds.joint(n=name)
   p = cmds.listRelatives(name, p=1)
   if p != None:  # parent all to world; default will create child joints
      cmds.parent(name, w=1)

# cleanup/create twist joints
def createTwistJoints(list,str):
   for i in list:
      if cmds.objExists(i+str):
         cmds.delete(i+str)
         createJoint(i+str)
         setLocalAxisDisplay(display=True)
      else:
         createJoint(i+str)
         setLocalAxisDisplay(display=True)

# create twist joints based on global list arrays
def createTwistJointsFromLists():
    createTwistJoints(upperarmTwistJoints,'')
    createTwistJoints(lowerarmTwistJoints,'')
    createTwistJoints(otherTwistJoints,'')
    if cmds.objExists('thigh_l'):
        createTwistJoints(thighTwistJoints,'')
    if cmds.objExists('thigh_hind_l') :
        createTwistJoints(hindTwistJoints,'')
    if cmds.objExists('pelvis_1p'):
        createTwistJoints(upperarmTwistJoints,'_1p')
        createTwistJoints(lowerarmTwistJoints,'_1p')

# parent constrain and delete constraint
def alignObject(src, tar):
    if cmds.objExists(src) :
        p = cmds.parentConstraint(src, tar)
        cmds.delete(p)

# position constrain and delete constraint
def averagePosition(src1, src2, tar):
   n = cmds.pointConstraint(src1, src2, tar)
   cmds.delete(n)

def alignAndParentJoint(targetParent,child):
    alignObject(targetParent,child)
    cmds.makeIdentity(child, apply=True, t=0, r=1, s=1)
    p = cmds.listRelatives(child, p=1)
    if p != targetParent:
         cmds.parent(child, targetParent)

def alignObjectsInList(list):
    for i in list :
        if cmds.objExists(i) :
            twistSplit = i.split('_')
            twistParent = (twistSplit[0] + '_' + twistSplit[-1])
            alignObject(twistParent, i)
            cmds.makeIdentity(i, apply=True, t=0, r=1, s=1)  # freeze transforms
            p = cmds.listRelatives(i, p=1)
            if p != twistParent:
                cmds.parent(i, twistParent)

# position and orient twist joints into skeleton
def alignTwistJoints():
   alignObjectsInList(upperarmTwistJoints)
   alignObjectsInList(lowerarmTwistJoints)
   if cmds.objExists('thigh_l') :
        alignObjectsInList(thighTwistJoints)

   # special case for core twist joints that don't meet above filter req
   alignObject('neck_01', 'neck_twist')
   cmds.makeIdentity('neck_twist', apply=True, t=0, r=1, s=1)  # freeze transforms
   cmds.parent('neck_twist', 'neck_01')

   if cmds.objExists('pelvis_1p'):
       alignAndParentJoint('upperarm_l_1p','upperarm_twist_01_l_1p')
       alignAndParentJoint('upperarm_l_1p','upperarm_twist_02_l_1p')
       alignAndParentJoint('lowerarm_l_1p','lowerarm_twist_01_l_1p')
       alignAndParentJoint('lowerarm_l_1p','lowerarm_twist_02_l_1p')
       alignAndParentJoint('upperarm_r_1p','upperarm_twist_01_r_1p')
       alignAndParentJoint('upperarm_r_1p','upperarm_twist_02_r_1p')
       alignAndParentJoint('lowerarm_r_1p','lowerarm_twist_01_r_1p')
       alignAndParentJoint('lowerarm_r_1p','lowerarm_twist_02_r_1p')

   if cmds.objExists('thigh_hind_l') :
       alignAndParentJoint('thigh_hind_l', 'thigh_twist_01_hind_l')
       alignAndParentJoint('thigh_hind_r', 'thigh_twist_01_hind_r')

# move twist 02 joints 50% along parent joint
# twist 01 joints stay on the parent, twist 02 joints placed in middle of parent and child
def positionTwistJoints():
   averagePosition('head', 'neck_01', 'neck_twist')
   averagePosition('upperarm_r', 'lowerarm_r', 'upperarm_twist_02_r')
   averagePosition('upperarm_l', 'lowerarm_l', 'upperarm_twist_02_l')
   averagePosition('lowerarm_r', 'hand_r', 'lowerarm_twist_02_r')
   averagePosition('lowerarm_l', 'hand_l', 'lowerarm_twist_02_l')
   if cmds.objExists('pelvis_1p'):
       averagePosition('upperarm_r_1p', 'lowerarm_r_1p', 'upperarm_twist_02_r_1p')
       averagePosition('upperarm_l_1p', 'lowerarm_l_1p', 'upperarm_twist_02_l_1p')
       averagePosition('lowerarm_r_1p', 'hand_r_1p', 'lowerarm_twist_02_r_1p')
       averagePosition('lowerarm_l_1p', 'hand_l_1p', 'lowerarm_twist_02_l_1p')

# add orient constraints
def orientConstUpperArmTwistJoints(side):
   # upperarm twists
   setRotateOrder(upperarmTwistJoints, 2, '')
   orntUpper01 = cmds.orientConstraint('upperarm' + side, 'upperarm_twist_01' + side, mo=False, w=0.5)
   cmds.orientConstraint('clavicle' + side, 'upperarm_twist_01' + side, mo=False)
   cmds.setAttr(orntUpper01[0] + '.interpType', 2)  # set to 0: No Flip, 1:shortest, 2: Average
   orntUpper02 = cmds.orientConstraint('upperarm' + side, 'upperarm_twist_02' + side, mo=False)
   cmds.orientConstraint('clavicle' + side, 'upperarm_twist_02' + side, mo=False, w=0.5)
   cmds.setAttr(orntUpper02[0] + '.interpType', 2)  # set to 0: No Flip, 1:shortest, 2: Average


def orientConstHelperArmJoints(side,list):
    orntLower01 = cmds.orientConstraint(list[3]+side, list[1]+side, mo=True, skip=['y', 'z'], w=0.5)
    cmds.orientConstraint(list[0]+side, list[1]+side, mo=True, skip=['y', 'z'])
    cmds.setAttr(orntLower01[0] + '.interpType', 2)  # set to 0: No Flip, 1: Average
    orntLower02 = cmds.orientConstraint(list[3]+side, list[2]+side, mo=True, skip=['y', 'z'])
    cmds.orientConstraint(list[0]+side, list[2]+side, mo=True, skip=['y', 'z'], w=0.5)
    cmds.setAttr(orntLower02[0] + '.interpType', 2)  # set to 0: No Flip, 1: Average

def orientConstThighTwistJoints(side):
   # thigh twists
   if cmds.objExists('thigh'+side):
       orntThigh = cmds.orientConstraint('pelvis', 'thigh_twist_01' + side, mo=False)
       cmds.orientConstraint('thigh' + side, 'thigh_twist_01' + side, mo=False)
       cmds.setAttr(orntThigh[0] + '.interpType', 0)  # set to 0: No Flip, 1: Average
       if cmds.objExists('thigh_hind'+side) : # if biped thigh exists AND hind thigh exists, constrain hind thigh to biped thigh
           orntHindThigh = cmds.orientConstraint('pelvis', 'thigh_twist_01_hind' + side, mo=False)
           cmds.orientConstraint('thigh' + side, 'thigh_twist_01' + side, mo=False)
           cmds.setAttr(orntHindThigh[0] + '.interpType', 0)  # set to 0: No Flip, 1: Average

def orientConstHindThighTwistJoints(side):
    if cmds.objExists('thigh_hind'+side) :
        orntHindThigh = cmds.orientConstraint('pelvis', 'thigh_twist_01_hind' + side, mo=False)
        cmds.orientConstraint('thigh_hind' + side, 'thigh_twist_01_hind' + side, mo=False)
        cmds.setAttr(orntHindThigh[0] + '.interpType', 0)  # set to 0: No Flip, 1: Average

def createRigGroup(name):
   if cmds.objExists(name) :
       cmds.setAttr(name+".visibility", 0)
   else :
       cmds.group(em=1, n=name)
       cmds.setAttr(name+".visibility", 0)

def setupHelperTwistJoints(side):
    #create the helper joints
    createTwistJoints(helperTwistJoints,side)
    #parent helper joints in a hidden group
    rigGroup = 'rig_GRP'
    createRigGroup(rigGroup)
    cmds.parent(helperTwistJoints[0]+side, rigGroup)
    #align the first joint to lower arm and clear history
    alignObject('lowerarm'+side, helperTwistJoints[0]+side)
    cmds.makeIdentity(helperTwistJoints[0]+side, apply=True, t=0, r=1, s=1)
    #align the remaining joints and parent to root helper joint
    alignAndParentJoint(helperTwistJoints[0]+side, helperTwistJoints[1]+side)
    alignAndParentJoint(helperTwistJoints[0]+side, helperTwistJoints[2]+side)
    alignAndParentJoint(helperTwistJoints[0]+side, helperTwistJoints[3]+side)
    #set the rotate order to XZY for all helper joints
    setRotateOrder(helperTwistJoints, 3, side)
    #reposition the last helper joint to the hand and delete constraint
    p = cmds.pointConstraint('hand'+side, helperTwistJoints[3]+side)
    cmds.delete(p)
    #reposition helper joints based in between hand, lowerarm and twists
    averagePosition('lowerarm'+side, 'lowerarm_twist_02'+side, helperTwistJoints[1]+side)
    averagePosition('hand'+side, 'lowerarm_twist_02'+side, helperTwistJoints[2]+side)
    #setup the new helper joint orient constraints
    orientConstHelperArmJoints(side, helperTwistJoints)
    #rotate hand joints for caching twists
    addLowerarmKeyframes('hand'+side, True)
    #setup final parent constraints for new helper joints
    cmds.parentConstraint('lowerarm'+side, helperTwistJoints[0]+side, mo=True)
    cmds.parentConstraint('hand'+side, helperTwistJoints[3]+side, mo=True)
    cmds.orientConstraint(helperTwistJoints[1]+side, 'lowerarm_twist_01'+side, mo=True, cc=(0,360))
    cmds.orientConstraint(helperTwistJoints[2]+side, 'lowerarm_twist_02'+side, mo=True, cc=(0,360))
    addLowerarmKeyframes('hand'+side, False)
    

#rotate the specified joint 360 degrees in X, use True / False to add or clear keyframes, respectively 
#example case: rotate the hand joints to cache rotation for twist joints
def addLowerarmKeyframes(target, bool):
    cmds.cutKey(target, t=(0,360))
    if bool == True :
        cmds.currentTime(0)
        cmds.setKeyframe(target, v=0, at='rotateX', t=(0,360))
        cmds.currentTime(180)
        cmds.setKeyframe(target, v=180, at='rotateX')
        cmds.currentTime(181)
        cmds.setKeyframe(target, v=-180, at='rotateX')
        cmds.currentTime(0)

def orientConstJoints():
    orientConstUpperArmTwistJoints('_r')
    orientConstUpperArmTwistJoints('_l')
    orientConstThighTwistJoints('_r')
    orientConstThighTwistJoints('_l')
    cmds.expression(s="neck_twist.rotateX = head.rotateX * 0.5")
    if cmds.objExists('thigh_hind_l'):
        orientConstHindThighTwistJoints('_l')
        orientConstHindThighTwistJoints('_r')
    if cmds.objExists('pelvis_1p'):
        orientConstUpperArmTwistJoints('_r_1p')
        orientConstUpperArmTwistJoints('_l_1p')

def setupTwistJoints():
   createTwistJointsFromLists()
   alignTwistJoints()
   positionTwistJoints()
   orientConstJoints()
   setupHelperTwistJoints('_l')
   setupHelperTwistJoints('_r')
   if cmds.objExists('pelvis_1p'):
       setupHelperTwistJoints('_l_1p')
       setupHelperTwistJoints('_r_1p')

setupTwistJoints()
