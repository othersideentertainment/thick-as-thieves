import maya.cmds as cmds
import Select

#toggle local axis display
def DisplayLocalAxis (display=False) :
    selected = Select.Type('joint')
    #jointList = cmds.ls(sl=1, type='joint')
    for jnt in selected :
        cmds.setAttr(jnt + '.displayLocalAxis', display)
