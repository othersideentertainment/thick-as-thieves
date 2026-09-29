import maya.cmds as cmds
import os

OrigRefName = ''
NewRefName = ''

projectPath = cmds.workspace(q=1, rd=1)

AnimNodes = [u'index_finger_fk_ctrl_3_l', u'index_finger_fk_ctrl_1_l', u'index_finger_fk_ctrl_2_l', u'middle_finger_fk_ctrl_2_l', u'middle_finger_fk_ctrl_1_l', u'ring_finger_fk_ctrl_1_l', u'ring_finger_fk_ctrl_3_l', u'middle_finger_fk_ctrl_3_l', u'ring_finger_fk_ctrl_2_l', u'pinky_finger_fk_ctrl_1_l', u'pinky_finger_fk_ctrl_2_l', u'thumb_finger_fk_ctrl_3_l', u'pinky_finger_fk_ctrl_3_l', u'thumb_finger_fk_ctrl_1_l', u'thumb_finger_fk_ctrl_2_l', u'head_fk_anim', u'neck_01_fk_anim', u'root_anim', u'toe_wiggle_ctrl_r', u'toe_tip_ctrl_r', u'heel_ctrl_r', u'ik_foot_anim_r', u'index_finger_fk_ctrl_1_r', u'index_finger_fk_ctrl_2_r', u'index_finger_fk_ctrl_3_r', u'middle_finger_fk_ctrl_1_r', u'middle_finger_fk_ctrl_2_r', u'middle_finger_fk_ctrl_3_r', u'ring_finger_fk_ctrl_1_r', u'ring_finger_fk_ctrl_3_r', u'pinky_finger_fk_ctrl_1_r', u'ring_finger_fk_ctrl_2_r', u'pinky_finger_fk_ctrl_2_r', u'pinky_finger_fk_ctrl_3_r', u'thumb_finger_fk_ctrl_3_r', u'thumb_finger_fk_ctrl_2_r', u'thumb_finger_fk_ctrl_1_r', u'ik_foot_anim_l', u'toe_wiggle_ctrl_l', u'toe_tip_ctrl_l', u'heel_ctrl_l', u'chest_ik_anim', u'hip_anim', u'ik_elbow_r_anim', u'body_anim', u'spine_01_anim', u'spine_02_anim', u'spine_03_anim', u'mid_ik_anim', u'ik_wrist_r_anim', u'fk_clavicle_r_anim', u'clavicle_r_anim', u'r_global_ik_anim', u'l_global_ik_anim', u'ik_elbow_l_anim', u'offset_anim', u'master_anim', u'fk_clavicle_l_anim', u'clavicle_l_anim', u'ik_wrist_l_anim', u'fk_ball_r_anim', u'fk_foot_r_anim', u'fk_calf_r_anim', u'fk_thigh_r_anim', u'fk_ball_l_anim', u'fk_foot_l_anim', u'fk_calf_l_anim', u'fk_thigh_l_anim', u'fk_wrist_r_anim', u'fk_elbow_r_anim', u'fk_arm_r_anim', u'fk_wrist_l_anim', u'fk_elbow_l_anim', u'fk_arm_l_anim', u'Rig_Settings'] #

minTime = cmds.playbackOptions(minTime=1, q=1)
maxTime = cmds.playbackOptions(maxTime=1, q=1)

#load reference into current scene
def LoadRef(str):
    cmds.file(f=1, new=0)
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+str+'.mb', r=True, ns=str)

#update missing references
def UpdateExistingRef(str):
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+str+'.mb', lr=str+'RN', pmt=0)
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/ExportFiles/'+str+'_Export.mb', lr=str+':'+str+'_ExportRN', pmt=0)

#replace reference: old ref name, new ref name
def ReplaceRef(oldName,newName):
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+newName+'.mb', lr=oldName+'RN', pmt=0)
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/ExportFiles/'+newName+'_Export.mb', lr=oldName+':'+oldName+'_ExportRN', pmt=0)
    #cmds.file( projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+newName+'.mb', e=1, namespace=newName)
    UpdateExportNameSpace(newName)
    UpdateExportPath()

#remove reference
def RemoveRef(str):
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+str+'.mb', rr=1)

#copy keys from Orig Anim Nodes
def KeyRigSettings():
    obj = 'DefaultShort:Rig_Settings'
    attr = cmds.listAttr(obj, sn=1, k=1)
    cmds.select(cl=1)
    for each in attr:
        cmds.setKeyframe(obj, at=each, t=(0,0))

def CopyKeys(str):
    cmds.select(cl=1)
    for each in AnimNodes:
        cmds.select(str+each, add=1)

    cmds.copyKey(t=(minTime,maxTime))

#paste keys on to New Anim Nodes
def PasteKeys(str):
    cmds.select(cl=1)
    for each in AnimNodes:
        cmds.select(str+each, add=1)

    cmds.pasteKey()

#parent constrain Hip (translate only)
def ParConstTransLate (obj, src, tar):
    cmds.parentConstraint(src+obj, tar+obj, mo=0, sr=('x','y','z'), w=1)

#parentConstrain Limbs
def ParConstLimbs(src,tar):
    nodes = ['fk_elbow_l_anim','fk_wrist_l_anim', 'ik_foot_anim_l']
    for n in nodes :
        cmds.parentConstraint(src+n, tar+n, mo=1, w=1)

def BakeHipKeys(str):
    nodes = [str+'body_anim']
    cmds.bakeResults(nodes, sm=1, t=(minTime,maxTime))

def BakeLimbKeys(str):
    nodes = [str+'fk_arm_l_anim', str+'fk_elbow_l_anim', str+'fk_wrist_l_anim', str+'ik_foot_anim_l']
    cmds.bakeResults(nodes, sm=1, t=(minTime,maxTime))

def UpdateExportNameSpace(str):
    nodes = cmds.ls(type='gameFbxExporter')
    for x in nodes:
        cmds.setAttr(x+'.exportFilename', 'AN_' + str + '_', type='string')

def UpdateExportPath():
    nodes = cmds.ls(type='gameFbxExporter')
    filepath = cmds.file(q=1, sn=1)
    dirname = os.path.dirname(filepath)
    for x in nodes:
        cmds.setAttr(x+'.exportPath', dirname+'\Export', type='string')

def UpdateFile(OrigRefName, NewRefName):
    LoadRef(NewRefName)
    UpdateExistingRef(OrigRefName)
    BakeLimbKeys(OrigRefName+':')
    KeyRigSettings()
    CopyKeys(OrigRefName+':')
    PasteKeys(NewRefName+':')
    ParConstTransLate('body_anim', OrigRefName+':', NewRefName+':')
    BakeHipKeys(NewRefName+':')
    #RemoveRef(OrigRefName)
    UpdateExportNameSpace(NewRefName)
    UpdateExportPath()
