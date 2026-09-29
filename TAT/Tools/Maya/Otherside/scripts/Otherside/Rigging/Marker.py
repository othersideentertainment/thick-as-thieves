import maya.cmds as cmds
import maya.OpenMaya as OpenMaya
import Otherside.Rigging.RigUtility as RigUtility
import math


def create(**kwargs):
    name = kwargs.get("n", "marker")
    parent = kwargs.get("p", None)
    target = kwargs.get("t", None)
    marker = cmds.createNode("transform", n=name, p=parent)
    scaleList = [ [3,.1,.1], [.1,3,.1], [.1,.1,3] ]
    colorList = [4,14,6]
    for i in range(0, len(colorList)):
        scale = scaleList[i]
        color = colorList[i]
        pointList = [
            [0.5,   0.5, -0.5],
            [0.5,   0.5,  0.5],
            [-0.5,  0.5,  0.5],
            [-0.5,  0.5, -0.5],
            [0.5,   0.5, -0.5],
            [0.5,  -0.5, -0.5],
            [0.5,  -0.5,  0.5],
            [0.5,   0.5,  0.5],
            [0.5,  -0.5,  0.5],
            [-0.5, -0.5,  0.5],
            [-0.5,  0.5,  0.5],
            [-0.5, -0.5,  0.5],
            [-0.5, -0.5, -0.5],
            [-0.5,  0.5, -0.5],
            [-0.5, -0.5, -0.5],
            [0.5,  -0.5, -0.5]]
        for i in range(0,len(pointList)):
            p = pointList[i]
            p[0] = p[0] * scale[0] + (scale[0] * .5)
            p[1] = p[1] * scale[1] + (scale[1] * .5)
            p[2] = p[2] * scale[2] + (scale[2] * .5)
            pointList[i] = p
        temp = cmds.curve(d=1, p=pointList)
        shape = cmds.listRelatives(temp, s=True)[0]
        cmds.setAttr(shape+".overrideEnabled", 1)
        cmds.setAttr(shape+".overrideColor", color)
        cmds.parent(shape, marker, r=True, s=True)
        cmds.delete(temp)
    if target:
        attach(marker, target)
    return cmds.ls(marker, l=True)[-1]


def attach(marker, target):
    cmds.matchTransform(marker, target)
    RigUtility.parentByMatrix(marker, target)
    RigUtility.resetTransform(marker)


def orient(source, **kwargs):
    # Get Keyword Args
    target = kwargs.get("target", None)
    targetPoint = kwargs.get("targetPoint", None)
    aimAxis = kwargs.get("aimAxis", [1,0,0])
    upAxis = kwargs.get("upAxis", [0,1,0])
    worldUpAxis = kwargs.get("worldUpAxis", [0,1,0])
    # Convert Axis Lists to MVectors
    aimAxis = OpenMaya.MVector(aimAxis[0], aimAxis[1], aimAxis[2])
    secondaryAxis = OpenMaya.MVector(upAxis[0], upAxis[1], upAxis[2])
    secondaryAxisWorld = OpenMaya.MVector(worldUpAxis[0], worldUpAxis[1], worldUpAxis[2])
    # Get the Source Object's Position based off its RotatePivot
    sourceDag = __getDagPath(source)
    transformFn = OpenMaya.MFnTransform(sourceDag)
    sourcePivot = transformFn.rotatePivot(OpenMaya.MSpace.kWorld)
    # Get the Target Object's Position
    targetPivot = OpenMaya.MPoint()
    # | Using Target Object's Position based off its RotatePivot
    if target:
        targetDag = __getDagPath(target)
        transformFn = OpenMaya.MFnTransform(targetDag)
        targetPivot = transformFn.rotatePivot(OpenMaya.MSpace.kWorld)
    # | Using Provided Point (TargetPoint)
    elif targetPoint:
        targetPivot = OpenMaya.MPoint(targetPoint[0], targetPoint[1],  targetPoint[2] )
    #Calculate Axis Vectors
    aimVector = (targetPivot - sourcePivot)
    coordU = aimVector.normal()
    coordV = secondaryAxisWorld
    coordW = (coordU ^ coordV).normal()
    coordV = (coordW ^ coordU)
    # Build Quaternion for the Aim rotation
    quaternion = OpenMaya.MQuaternion()
    quaternionU = OpenMaya.MQuaternion(aimAxis, coordU)
    quaternion = quaternionU
    # Build Quaternion for the Up Rotation
    upRotated = secondaryAxis.rotateBy(quaternion)
    value = (upRotated * coordV)
    if  value < -1:
        value = -1
    if value > 1:
        value = 1
    angle = math.acos(value)
    angle = math.acos(value)
    quaternionV = OpenMaya.MQuaternion(angle, coordU)
    # | flip if needed
    if not coordV.isEquivalent(upRotated.rotateBy(quaternionV), 1.0e-5):
        angle = (2*math.pi) - angle
        quaternionV = OpenMaya.MQuaternion(angle, coordU)
    # Final Quaternion
    quaternion *= quaternionV
    # Apply the transform
    transformFn.setObject(sourceDag)
    transformFn.setRotation(quaternion, OpenMaya.MSpace.kWorld)


def __getDagPath(dagObj):
    dagPath = OpenMaya.MDagPath()
    sel = OpenMaya.MSelectionList()
    sel.add(dagObj)
    sel.getDagPath(0, dagPath)
    return dagPath



def setRestPosition(marker):
    if not cmds.objExists("{}.restPosition".format(marker)):
        cmds.addAttr(marker, ln="restPosition", at="float3")
        cmds.addAttr(marker, ln="restPositionX", at="float", p="restPosition")
        cmds.addAttr(marker, ln="restPositionY", at="float", p="restPosition")
        cmds.addAttr(marker, ln="restPositionZ", at="float", p="restPosition")
    pos = cmds.xform(marker, q=True, ws=True, t=True)
    cmds.setAttr("{}.restPosition".format(marker), pos[0], pos[1], pos[2])