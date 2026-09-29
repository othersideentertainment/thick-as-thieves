####################################
# WIP - Horse
# This script requires joints from SKEL_Master.ma to work
# Copy & Paste into python script editor WIP menu
# Edit create<PredefinedFig> to change the character type
# TODO: UI and dropdown
####################################

import maya.cmds as cmds
import math

coreJointNames = ['pelvis', 'spine_01', 'spine_02', 'spine_03', 'neck_01', 'head']
armJointNames = ['clavicle', 'upperarm', 'lowerarm', 'hand']
legJointNames = ['thigh', 'calf', 'foot', 'ball']
hindJointNames = ['thigh', 'calf', 'heel', 'foot', 'ball']
fingerJointNames = ['index_metacarpal', 'index_01', 'index_02', 'index_03',
              'middle_metacarpal', 'middle_01', 'middle_02', 'middle_03',
              'pinky_metacarpal', 'pinky_01', 'pinky_02', 'pinky_03',
              'ring_metacarpal', 'ring_01', 'ring_02', 'ring_03',
              'thumb_01', 'thumb_02', 'thumb_03']
tailJointNames = ['tail_01', 'tail_02', 'tail_03', 'tail_04', 'tail_05', 'tail_06', 'tail_07', 'tail_08', 'tail_09', 'tail_10']
wingJointNames = ['clavicle', 'upperarm', 'lowerarm', 'hand', 'index_01', 'index_02', 'index_03', 'thumb_01', 'thumb_02', 'thumb_03']
ikJointNames = ['ik_foot_root', 'ik_foot_l', 'ik_foot_r', 'ik_hand_root', 'ik_hand_gun', 'ik_hand_l', 'ik_hand_r']
twistJointNames = ['lowerarm_twist_01', 'lowerarm_twist_02', 'upperarm_twist_01', 'upperarm_twist_02', 'thigh_twist_01']

coreSuffix = ['']
limbSuffix = ['_l', '_r']
hindSuffix = ['_hind_l','_hind_r']
wingSuffix = ['_wing_l', '_wing_r']

selected = cmds.ls(sl=1)
meshGroup = 'GrayBox_GRP'

def addGrayBoxes (arrayName, suffix, boxHeight, boxDepth) :
    for suff in suffix :
        for each in arrayName :
            #box = 'box_'+each+suff
            #if cmds.objExists(box) : #cleanup scene
                #cmds.delete(box)
            boneChild = cmds.listRelatives(each+suff)
            if boneChild != None : #get the length of the bone based on the children
                boxWidth = cmds.getAttr('%s.translateX' % boneChild[0])
                cube = cmds.polyCube(sx=2, sy=2, sz=2, w=abs(boxWidth), h=boxHeight, d=boxDepth, n=('box_'+each+suff))
                cmds.move(((boxWidth/2)*-1),0,0,'box_'+each+suff+'.scalePivot', 'box_'+each+suff+'.rotatePivot',a=1) #set pivot to X position
                cmds.matchTransform(cube, each+suff)
                cmds.parent('box_'+each+suff, meshGroup)
                #cmds.makeIdentity(apply=True, t=1, r=1, s=1) #freeze transforms
                cmds.delete(ch=1) #delete history
            else : #if there are no children, use own bone length
                boxWidth = cmds.getAttr(each+suff+'.translateX')
                cube = cmds.polyCube(sx=2, sy=2, sz=2, w=abs(boxWidth), h=boxHeight, d=boxDepth, n=('box_'+each+suff))
                cmds.move(((boxWidth/2)*-1),0,0,'box_'+each+suff+'.scalePivot', 'box_'+each+suff+'.rotatePivot',a=1) #set pivot to X position
                cmds.matchTransform(cube, each+suff)
                cmds.parent('box_'+each+suff, meshGroup)
                #cmds.makeIdentity(apply=True, t=1, r=1, s=1) #freeze transforms
                cmds.delete(ch=1) #delete history

def addGrayBoxesToSelected () :
    if selected != None :
        numSel = len(selected)
        for each in selected :
            if each == selected[(numSel-1)] :
                addGrayBoxes(selected, coreSuffix, 5, 5)
            else :
                addGrayBoxes(selected, coreSuffix, 5, 5)
                break #this is here to prevent it from creating a duplicate from selected
    else :
        print 'You must select joints to add gray boxes to.'

def createGroup () :
    if cmds.objExists(meshGroup) :
        cmds.delete(meshGroup)
        cmds.group(em=1, n=meshGroup)
    else :
        cmds.group(em=1, n=meshGroup)

def createBiped () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(legJointNames, limbSuffix, 12,12)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)

def createBipedWithTail () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(legJointNames, limbSuffix, 12,12)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(tailJointNames, coreSuffix, 5,5)

def createWingedBiped () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(legJointNames, limbSuffix, 12,12)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(wingJointNames, wingSuffix, 10,10)

def createWingedBipedWithTail () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(legJointNames, limbSuffix, 12,12)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(wingJointNames, wingSuffix, 3,10)
    addGrayBoxes(tailJointNames, coreSuffix, 5,5)

def createSatyr () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(hindJointNames, hindSuffix, 10,10)

def createSatyrWithTail () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(hindJointNames, hindSuffix, 10,10)
    addGrayBoxes(tailJointNames, coreSuffix, 5,5)

def createWingedSatyr () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(hindJointNames, hindSuffix, 10,10)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(wingJointNames, wingSuffix, 10,10)

def createWingedSatyrWithTail () :
    addGrayBoxes(coreJointNames, coreSuffix, 15,20)
    addGrayBoxes(armJointNames, limbSuffix, 10,10)
    addGrayBoxes(hindJointNames, hindSuffix, 10,10)
    addGrayBoxes(fingerJointNames, limbSuffix, 3,3)
    addGrayBoxes(wingJointNames, wingSuffix, 3,10)
    addGrayBoxes(tailJointNames, coreSuffix, 5,5)


createGroup()
#createSatyr()
addGrayBoxesToSelected()
