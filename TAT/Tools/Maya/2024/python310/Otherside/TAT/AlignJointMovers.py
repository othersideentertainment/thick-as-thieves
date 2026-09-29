#######################################################################
# Owner: Horse
#
# Use this script with ARTv1.
# 1. Create New ARTv1 Rig, use 'basic' template, get to Skeleton Placement step
# 2. Import SKEL character, root+heirarchy named with *bone_* prefix
# 3. Run script
#######################################################################

import maya.cmds as cmds

coreJointNames = ['pelvis', 'spine_01', 'spine_02', 'spine_03', 'neck_01', 'head']
limbJointNames = ['clavicle', 'upperarm', 'lowerarm', 'hand', 'index_metacarpal', 'index_01', 'index_02', 'index_03',
                  'middle_metacarpal', 'middle_01', 'middle_02', 'middle_03', 'pinky_metacarpal', 'pinky_01',
                  'pinky_02', 'pinky_03',
                  'ring_metacarpal', 'ring_01', 'ring_02', 'ring_03', 'thumb_01', 'thumb_02', 'thumb_03',
                  'thigh', 'calf', 'foot', 'ball',
                  ]
def alignJointMovers(prefix, mover, side, array):
   for each in array:
      cmds.matchTransform(each+mover+side, prefix+each+side, pos=1, rot=1, scl=0)
      
alignJointMovers('bone_', '_mover', '', coreJointNames)
alignJointMovers('bone_', '_mover', '_l', limbJointNames)
alignJointMovers('bone_', '_mover', '_r', limbJointNames)
