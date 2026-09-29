#copy keys from DefaultShort
import maya.cmds as cmds

AnimNodes = [u'index_finger_fk_ctrl_3_l', u'index_finger_fk_ctrl_1_l', u'index_finger_fk_ctrl_2_l', u'middle_finger_fk_ctrl_2_l', u'middle_finger_fk_ctrl_1_l', u'ring_finger_fk_ctrl_1_l', u'ring_finger_fk_ctrl_3_l', u'middle_finger_fk_ctrl_3_l', u'ring_finger_fk_ctrl_2_l', u'pinky_finger_fk_ctrl_1_l', u'pinky_finger_fk_ctrl_2_l', u'thumb_finger_fk_ctrl_3_l', u'pinky_finger_fk_ctrl_3_l', u'thumb_finger_fk_ctrl_1_l', u'thumb_finger_fk_ctrl_2_l', u'head_fk_anim', u'neck_01_fk_anim', u'root_anim', u'toe_wiggle_ctrl_r', u'toe_tip_ctrl_r', u'heel_ctrl_r', u'ik_foot_anim_r', u'index_finger_fk_ctrl_1_r', u'index_finger_fk_ctrl_2_r', u'index_finger_fk_ctrl_3_r', u'middle_finger_fk_ctrl_1_r', u'middle_finger_fk_ctrl_2_r', u'middle_finger_fk_ctrl_3_r', u'ring_finger_fk_ctrl_1_r', u'ring_finger_fk_ctrl_3_r', u'pinky_finger_fk_ctrl_1_r', u'ring_finger_fk_ctrl_2_r', u'pinky_finger_fk_ctrl_2_r', u'pinky_finger_fk_ctrl_3_r', u'thumb_finger_fk_ctrl_3_r', u'thumb_finger_fk_ctrl_2_r', u'thumb_finger_fk_ctrl_1_r', u'ik_foot_anim_l', u'toe_wiggle_ctrl_l', u'toe_tip_ctrl_l', u'heel_ctrl_l', u'chest_ik_anim', u'hip_anim', u'ik_elbow_r_anim', u'body_anim', u'spine_01_anim', u'spine_02_anim', u'spine_03_anim', u'mid_ik_anim', u'ik_wrist_r_anim', u'fk_clavicle_r_anim', u'clavicle_r_anim', u'r_global_ik_anim', u'l_global_ik_anim', u'ik_elbow_l_anim', u'offset_anim', u'master_anim', u'fk_clavicle_l_anim', u'clavicle_l_anim', u'ik_wrist_l_anim', u'fk_ball_r_anim', u'fk_foot_r_anim', u'fk_calf_r_anim', u'fk_thigh_r_anim', u'fk_ball_l_anim', u'fk_foot_l_anim', u'fk_calf_l_anim', u'fk_thigh_l_anim', u'fk_wrist_r_anim', u'fk_elbow_r_anim', u'fk_arm_r_anim', u'fk_wrist_l_anim', u'fk_elbow_l_anim', u'fk_arm_l_anim', u'Rig_Settings'] #

minTime = cmds.playbackOptions(minTime=1, q=1)
maxTime = cmds.playbackOptions(maxTime=1, q=1)

def keyRigSettings(str):
    obj = str+'Rig_Settings'
    attr = cmds.listAttr(obj, sn=1, k=1)
    cmds.select(cl=1)
    for each in attr:
        cmds.setKeyframe(obj, at=each, t=(0,0))

def copyKeys(str):
    cmds.select(cl=1)
    for each in AnimNodes:
        cmds.select(str+each, add=1)

    cmds.copyKey(t=(minTime,maxTime))

keyRigSettings('DefaultShort:')
copyKeys('DefaultShort:')
