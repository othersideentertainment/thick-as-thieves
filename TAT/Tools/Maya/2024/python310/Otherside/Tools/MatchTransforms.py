import maya.cmds

def matchPosRot (selected) :
    cmds.matchTransform(selected[0],selected[1],pos=1,rot=1,scl=0)

matchPosRot (cmds.ls(sl=1))