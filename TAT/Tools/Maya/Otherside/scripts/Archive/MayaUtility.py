import maya.cmds as cmds

def pointOrientConstraint(targetXform, objXform):
    pc = cmds.pointConstraint(targetXform, objXform)
    oc = cmds.orientConstraint(targetXform, objXform)
    return [pc[0], oc[0]]
    
    
def pointOrientConstraintSelected(*args):
    sel = cmds.ls(fl=True, os=True)
    constraints = None
    if len(sel) == 2:
        constraints = pointOrientConstraint(sel[0], sel[1])
    return constraints    


def matchTransforms(source, destination):
    m = cmds.xform(source, q=True, ws=True, matrix=True)
    cmds.xform(destination, ws=True, m=[
        m[0], m[1], m[2], m[3], 
        m[4], m[5], m[6], m[7], 
        m[8], m[9], m[10], m[11], 
        m[12], m[13], m[14], m[15]])


def matchTransformsSelected(*args):
    sel = cmds.ls(os=True)
    source = sel[-1]
    for i in range(0,len(sel)-1):
        destination = sel[i]
        matchTransforms(source, destination)


def convertRotationToOrientation(bones=[]):
    for bone in bones:
        rot = cmds.xform(bone, q=True, ws=True, ro=True)
        cmds.setAttr(bone+".jointOrient", 0,0,0)
        cmds.setAttr(bone+".rotate", 0,0,0)
        cmds.xform(bone, ws=True, ro=rot)
        rot = cmds.xform(bone, q=True, ws=True, ro=True)
        cmds.setAttr(bone+".rotate", 0,0,0)
        cmds.setAttr(bone+".jointOrient", rot[0],rot[1],rot[2])


def convertRotationToOrientationSelected(*args):
    bones = cmds.ls(sl=True, fl=True, type="joint")
    convertRotationToOrientation(bones)


def updateCameraClipPlanes(*args):
    cameraList = cmds.ls(type="camera")
    for cam in cameraList:
        cmds.setAttr(cam+".nearClipPlane", .1)
        cmds.setAttr(cam+".farClipPlane", 1000)
        if cam == "topShape":
            xform = cmds.listRelatives(cam, p=True)[0]
            cmds.setAttr(xform+".ty", 250)
        elif cam == "sideShape":
            xform = cmds.listRelatives(cam, p=True)[0]
            cmds.setAttr(xform+".tx", 250)
        elif cam == "frontShape":
            xform = cmds.listRelatives(cam, p=True)[0]
            cmds.setAttr(xform+".tz", 250)


def ResetTransformSelected(*args):
    sel = cmds.ls(sl=True, fl=True, type="transform")
    for s in sel:
        ResetTransform(s)


def ResetTransform(transform):
    cmds.setAttr(transform+".translate", 0,0,0)
    cmds.setAttr(transform+".rotate", 0,0,0)
    cmds.setAttr(transform+".scale", 1, 1, 1)
    if cmds.objExists(transform+".jointOrient"):
        cmds.setAttr(transform+".jointOrient", 0,0,0)