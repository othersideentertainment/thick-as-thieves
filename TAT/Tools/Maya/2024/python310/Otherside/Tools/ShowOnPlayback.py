import maya.cmds as cmds

sel = cmds.ls(sl=True)
for s in sel:
    cmds.setAttr(s+".hideOnPlayback", 0)