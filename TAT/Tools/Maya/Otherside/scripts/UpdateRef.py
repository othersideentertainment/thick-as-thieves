###########################################################
# Owner: Horse
# Description - A few functions to be used on your shelf.
# Requirements: Set Maya project to Animation/_maya_/ folder.
#
# 1. The last 3 lines are commented out. Remove the '#' for the function you want to run.
# 2. Edit the 'text' in the () for the character you would like. It will automatically bring in the _Export version as well.
# 3. I recommend creating 3 different shelf icons, each one removing the # comment for the function you want to run.
###########################################################

import maya.cmds as cmds

projectPath = cmds.workspace(q=1, rd=1)

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

#remove reference
def RemoveRef(str):
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+str+'.mb', rr=1)


#LoadRef('Tr_MN')
#UpdateExistingRef('DefaultShort')
#RemoveRef('DefaultShort')
