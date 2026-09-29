import maya.cmds as cmds


def create(*args):
    transformList = cmds.ls(fl=True, type="transform")
    for transform in transformList:
        if cmds.objExists(transform+".cameraPositionTol"):
            cmds.delete(transform)

    #Create Transforms
    root = cmds.createNode("transform", n="CameraPositionTool")    
    blondieReference = cmds.createNode("transform", n="BlondieReference", p=root)
    blondieHeightReference = cmds.createNode("transform", n="BlondieHeightReference", p=blondieReference)
    blondieHeadBone = cmds.createNode("transform", n="BlondieHeadBone", p=blondieReference)
    blondieCameraReference = cmds.createNode("transform", n="BlondieCameraReference", p=blondieHeadBone)    
    headBoneMarker = cmds.createNode("transform", n="HeadBoneMarker", p=root)
    positionMarker = cmds.createNode("transform", n="PositionMarker", p=headBoneMarker)
    orientationMarker = cmds.createNode("transform", n="OrientationMarker", p=positionMarker)    
    characterHeight = cmds.createNode("transform", n="CharacterHeight", p=root)    
    cameraMarker = cmds.createNode("transform", n="CameraMarker", p=root)    
    #Add Attrs
    cmds.addAttr(root, at="message", ln="cameraPositionTool")
    cmds.addAttr(characterHeight, ln="Ref", at="enum", en="----")
    cmds.setAttr(characterHeight+".Ref", e=True, k=True)
    cmds.addAttr(characterHeight, ln="defaultHeight", at="float")
    #Create Shapes
    locScale = .00025
    loc = cmds.createNode("locator", n="HeadBoneMarkerLocator", p=headBoneMarker)
    cmds.setAttr(loc+".localScale", locScale, locScale, locScale)
    loc = cmds.createNode("locator", n="CharacterHeightLocator", p=characterHeight)    
    cmds.setAttr(loc+".localScale", locScale, locScale, locScale)
    loc = cmds.createNode("locator", n="CameraMarkerLocator", p=cameraMarker)
    cmds.setAttr(loc+".localScale", locScale, locScale, locScale)
    #Set Marker Colors
    cmds.setAttr(headBoneMarker+".overrideEnabled", 1)
    cmds.setAttr(headBoneMarker+".overrideColor", 13)
    cmds.setAttr(characterHeight+".overrideEnabled", 1)
    cmds.setAttr(characterHeight+".overrideColor", 18)
    cmds.setAttr(cameraMarker+".overrideEnabled", 1)
    cmds.setAttr(cameraMarker+".overrideColor", 14)        
    #Set Positions
    cmds.setAttr(blondieHeightReference+".translateZ", 1.949)
    cmds.setAttr(blondieHeadBone+".translateY", .013)
    cmds.setAttr(blondieHeadBone+".translateZ", 1.731)
    cmds.setAttr(blondieCameraReference+".translateY", .1)
    cmds.setAttr(blondieCameraReference+".translateZ", .06)     
    cmds.setAttr(characterHeight+".translateZ", 1.949)
    cmds.setAttr(headBoneMarker+".translateY", .013)
    cmds.setAttr(headBoneMarker+".translateZ", 1.731)
    cmds.setAttr(orientationMarker+".rotateX", 100)    
    cmds.setAttr(cameraMarker+".translateY", .113)
    cmds.setAttr(cameraMarker+".translateZ", 1.791)
    cmds.setAttr(cameraMarker+".rotateX", 100)
    cmds.setAttr(characterHeight+".defaultHeight", 1.949)
    #Constraints
    cmds.pointConstraint(positionMarker, cameraMarker)
    cmds.orientConstraint(orientationMarker, cameraMarker)
    cmds.expression(s="$ratio = CharacterHeight.translateZ / BlondieHeightReference.translateZ;\n$x = BlondieCameraReference.translateX * $ratio;\n$y = BlondieCameraReference.translateY * $ratio;\n$z = BlondieCameraReference.translateZ * $ratio;\nPositionMarker.translateX = $x;\nPositionMarker.translateY = $y;\nPositionMarker.translateZ = $z;",  o="", ae=1, uc="all")