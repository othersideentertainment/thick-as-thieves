import maya.cmds as cmds

def controllerFix():
    #Controller Fix
    size = .001
    controlList = ["camera_anim", "weapon_r_anim", "weapon_l_anim"]
    for control in controlList:
        shapes = cmds.listRelatives(control, s=True)
        for shape in shapes:
            cmds.scale(size, size, size, shape+".cv[0:32]")

def visibleSkeleton():
    #Visible Skeleton
    root = "root"
    bones = cmds.listRelatives(root, ad=True)
    bones.append(root)
    for bone in bones:
        cmds.setAttr(bone+".v", 1)


def createFirstPersonCamera():
    #references to rig elements
    camera_anim = "camera_anim"
    head = "head"
    #Create Rig Heirarchy
    cameraRig = cmds.createNode("transform", n="FirstPersonCameraRig")
    cmds.addAttr(cameraRig, ln="firstPersonCameraRig", at="message")
    cameraSpace = cmds.createNode("transform", n="CameraSpace", p=cameraRig)
    cameraController = cmds.createNode("transform", n="CameraController", p=cameraSpace)
    offsetFOV = cmds.createNode("transform", n="OffsetFOV", p=cameraController)
    firstPersonCamera = cmds.createNode("transform", n="FirstPersonCamera", p=offsetFOV)
    cam = cmds.createNode("camera", p=firstPersonCamera)
    markerGroup = cmds.createNode("transform", n="Spaces", p=cameraRig)
    markerWorld = cmds.createNode("transform", n="World", p=markerGroup)
    markerHead = cmds.createNode("transform", n="Head", p=markerGroup)
    #Camera Settings
    cmds.setAttr(firstPersonCamera+".rotateY", 180)
    cmds.setAttr(firstPersonCamera+".overrideEnabled", 1)
    cmds.setAttr(firstPersonCamera+".overrideDisplayType", 2)
    cmds.setAttr(cam+".focalLength", 21.324)
    cmds.setAttr(cam+".nearClipPlane", .1)
    cmds.setAttr(cam+".farClipPlane", 1000)
    cmds.setAttr(cam+".verticalFilmAperture", .945)
    cmds.setAttr(cam+".horizontalFilmAperture", 1.679)
    #AlignTransforms
    cmds.matchTransform(markerGroup, camera_anim)
    cmds.matchTransform(cameraSpace, camera_anim)
    #Setup Space Switching
    pc = cmds.parentConstraint(markerHead, cameraSpace)[0]
    cmds.parentConstraint(markerWorld, cameraSpace)
    cmds.parentConstraint(cameraController, camera_anim)
    cmds.parentConstraint(head, markerHead, mo=True)
    cmds.addAttr(cameraController, ln="followParent", at="float", min=0, max=1, dv=1, k=1)
    wal = cmds.parentConstraint(pc, q=True, wal=True)
    #(Create the weight switch)
    weightSwitch = cmds.createNode("plusMinusAverage", n="weightSwitch")
    cmds.setAttr(weightSwitch+".operation", 2)
    cmds.setAttr(weightSwitch+".input1D[0]", 1)
    cmds.addAttr(weightSwitch, ln="input", at="float", min=0, max=1)
    cmds.addAttr(weightSwitch, ln="output", at="float", min=0, max=1)
    cmds.addAttr(weightSwitch, ln="outputInverse", at="float", min=0, max=1)
    cmds.connectAttr(weightSwitch+".input", weightSwitch+".input1D[1]", f=True)
    cmds.connectAttr(weightSwitch+".input", weightSwitch+".output", f=True)
    cmds.connectAttr(weightSwitch+".output1D", weightSwitch+".outputInverse", f=True)
    cmds.connectAttr(cameraController+".followParent", weightSwitch+".input", f=True)
    cmds.connectAttr(weightSwitch+".outputInverse", pc+"."+wal[1], f=True)
    cmds.connectAttr(weightSwitch+".output", pc+"."+wal[0], f=True)
    #Create Controller shape
    circle = cmds.circle()
    shapes = cmds.listRelatives(circle, s=True)
    for shape in shapes:
        cmds.setAttr(shape+".overrideEnabled", 1)
        cmds.setAttr(shape+".overrideColor", 17)
        cmds.parent(shape, cameraController, r=True,s=True)
    cmds.delete(circle)
    #Hide ArtV1 Camera Controller
    shapes = cmds.listRelatives(camera_anim, s=True)
    for shape in shapes:
        cmds.setAttr(shape+".overrideEnabled", 1)
        cmds.setAttr(shape+".overrideVisibility", 0)
    #Lock Unused Channels
    objList = [cameraRig, cameraSpace, offsetFOV, firstPersonCamera, markerGroup, markerWorld, markerHead]
    for obj in objList:
        cmds.setAttr(obj+".tx", l=True)
        cmds.setAttr(obj+".ty", l=True)
        cmds.setAttr(obj+".tz", l=True)
        cmds.setAttr(obj+".rx", l=True)
        cmds.setAttr(obj+".ry", l=True)
        cmds.setAttr(obj+".rz", l=True)
        cmds.setAttr(obj+".sx", l=True)
        cmds.setAttr(obj+".sy", l=True)
        cmds.setAttr(obj+".sz", l=True)
        cmds.setAttr(obj+".v", l=True)
    cmds.setAttr(cameraController+".sx", l=True)
    cmds.setAttr(cameraController+".sy", l=True)
    cmds.setAttr(cameraController+".sz", l=True)
    cmds.setAttr(cameraController+".v", l=True)

    #Lock Channels
    '''
    for channel in channels:
        attrPath = obj+"."+channel
        cmds.setAttr(attrPath, l=True)
    lockChannels(cameraRig,     ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(cameraSpace,   ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(cameraController,   ["sx","sy","sz","v"])
    lockChannels(offsetFOV,     ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(firstPersonCamera, ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(markerGroup,   ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(markerWorld,   ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    lockChannels(markerHead,    ["tx","ty","tz","rx","ry","rz","sx","sy","sz","v"])
    '''

def addCustomCurves():
    cmds.select('root')
    cmds.addAttr(ln='curve_ik_foot_r', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)
    cmds.addAttr(ln='curve_ik_foot_l', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)
    cmds.addAttr(ln='curve_ik_hand_r', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)
    cmds.addAttr(ln='curve_ik_hand_l', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)
    cmds.addAttr(ln='curve_cam_weight_position', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)
    cmds.addAttr(ln='curve_cam_weight_rotation', at='float', dv=0.0, k=1, h=0, min=0.0, max=1.0)

controllerFix()
createFirstPersonCamera()
visibleSkeleton()
addCustomCurves()
