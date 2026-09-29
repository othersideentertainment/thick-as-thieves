import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UtilityNodes as UtilityNodes
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.HindLeg"
CLASS_NAME = "HindLeg"
CONTROLLER_SIZE = [5, 5, 5]
IK_FOOT_SIZE = [20,15,25]
IK_KNEE_SIZE = [5,5,5]
IK_HOCK_SIZE = [10,10,10]
IK_HEEL_SIZE = [10,10,10]
IK_TOES_SIZE = [3,3,3]
IK_PIVOT_SIZE = [2,2,2]

class HindLeg(ControlRigBase):
    def __init__(self, instanceName="HindLeg", **kwargs):
        self.node = None
        self.instanceName = instanceName
        self.spaces = {}
        self.side = kwargs.get("side", 0)
        #
        self.characterized = False
        self.rigged = False
        #
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        #
        self.boneUpperLeg = None
        self.boneLowerLeg = None
        self.boneHock = None
        self.boneFoot = None
        self.boneToes = None
        #
        self.markerUpperLeg = None
        self.markerLowerLeg = None
        self.markerHock = None
        self.markerFoot = None
        self.markerToes = None
        #
        self.fkSpace = None
        self.fkUpperLeg = None
        self.fkLowerLeg = None
        self.fkHock = None
        self.fkFoot = None
        self.fkToes = None
        #
        self.ikSpace = None
        self.ikKnee = None
        self.ikHock = None
        self.ikFoot = None
        self.ikToes = None
        #
        self.ikHeelLift = None
        self.ikHeelPivot = None
        self.ikBallPivot = None
        self.ikToePivot = None
        #
        self.ikFootMarker = None


    @staticmethod
    def load(node):
        instance = HindLeg()
        #
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.spaces = {}
        instance.side = cmds.getAttr(node+".side")
        #
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        #
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        #Bone
        instance.boneUpperLeg = RigNode.getPlug(node, "boneUpperLeg")
        instance.boneLowerLeg = RigNode.getPlug(node, "boneLowerLeg")
        instance.boneHock = RigNode.getPlug(node, "boneHock")
        instance.boneFoot = RigNode.getPlug(node, "boneFoot")
        instance.boneToes = RigNode.getPlug(node, "boneToes")
        #Marker
        instance.markerUpperLeg = RigNode.getPlug(node, "markerUpperLeg")
        instance.markerLowerLeg = RigNode.getPlug(node, "markerLowerLeg")
        instance.markerHock = RigNode.getPlug(node, "markerHock")
        instance.markerFoot = RigNode.getPlug(node, "markerFoot")
        instance.markerToes = RigNode.getPlug(node, "markerToes")
        #FK
        instance.fkSpace = RigNode.getPlug(node, "fkSpace")
        instance.fkUpperLeg = RigNode.getPlug(node, "fkUpperLeg")
        instance.fkLowerLeg = RigNode.getPlug(node, "fkLowerLeg")
        instance.fkHock = RigNode.getPlug(node, "fkHock")
        instance.fkFoot = RigNode.getPlug(node, "fkFoot")
        instance.fkToes = RigNode.getPlug(node, "fkToes")
        #IK
        instance.ikSpace = RigNode.getPlug(node, "ikSpace")
        instance.ikFoot = RigNode.getPlug(node, "ikFoot")
        instance.ikKnee = RigNode.getPlug(node, "ikKnee")
        instance.ikHock = RigNode.getPlug(node, "ikHock")
        instance.ikToes = RigNode.getPlug(node, "ikToes")
        instance.ikHeelLift = RigNode.getPlug(node, "ikHeelLift")
        instance.ikHeelPivot = RigNode.getPlug(node, "ikHeelPivot")
        instance.ikBallPivot = RigNode.getPlug(node, "ikBallPivot")
        instance.ikToePivot = RigNode.getPlug(node, "ikToePivot")
        instance.ikFootMarker = RigNode.getPlug(node, "ikFootMarker")
        return instance


    def setBoneList(self, boneList):
        self.boneUpperLeg = boneList[0]
        self.boneLowerLeg = boneList[1]
        self.boneHock = boneList[2]
        self.boneFoot = boneList[3]
        self.boneToes = boneList[4]


    def getBoneList(self):
        return [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]


    def getBoneNames(self):
        return ["boneUpperLeg", "boneLowerLeg", "boneHock", "boneFoot", "boneToes"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkUpperLeg,
            self.fkLowerLeg,
            self.fkHock,
            self.fkFoot,
            self.fkToes,
            self.ikFoot,
            self.ikKnee,
            self.ikHock,
            self.ikToes,
            self.ikHeelLift,
            self.ikHeelPivot,
            self.ikBallPivot,
            self.ikToePivot
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkUpperLeg",
            "fkLowerLeg",
            "fkHock",
            "fkFoot",
            "fkToes",
            "ikFoot",
            "ikKnee",
            "ikHock",
            "ikToes",
            "ikHeelLift",
            "ikHeelPivot",
            "ikBallPivot",
            "ikToePivot"
            ]



    def getMarkerList(self):
        return [self.markerUpperLeg, self.markerLowerLeg, self.markerHock, self.markerFoot, self.markerToes]


    def getMarkerNames(self):
        return ["markerUpperLeg", "markerLowerLeg", "markerHock", "markerFoot", "markerToes"]


    def createNode(self):
        #Create Empty Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneUpperLeg", "message")
        nodeBuilder.addAttr("boneLowerLeg", "message")
        nodeBuilder.addAttr("boneHock", "message")
        nodeBuilder.addAttr("boneFoot", "message")
        nodeBuilder.addAttr("boneToes", "message")
        nodeBuilder.addAttr("markerUpperLeg", "message")
        nodeBuilder.addAttr("markerLowerLeg", "message")
        nodeBuilder.addAttr("markerHock", "message")
        nodeBuilder.addAttr("markerFoot", "message")
        nodeBuilder.addAttr("markerToes", "message")
        nodeBuilder.addAttr("fkSpace","message")
        nodeBuilder.addAttr("fkUpperLeg","message")
        nodeBuilder.addAttr("fkLowerLeg","message")
        nodeBuilder.addAttr("fkHock","message")
        nodeBuilder.addAttr("fkFoot","message")
        nodeBuilder.addAttr("fkToes","message")
        nodeBuilder.addAttr("ikSpace","message")
        nodeBuilder.addAttr("ikFoot","message")
        nodeBuilder.addAttr("ikKnee","message")
        nodeBuilder.addAttr("ikHock","message")
        nodeBuilder.addAttr("ikToes","message")
        nodeBuilder.addAttr("ikHeelLift","message")
        nodeBuilder.addAttr("ikHeelPivot","message")
        nodeBuilder.addAttr("ikBallPivot","message")
        nodeBuilder.addAttr("ikToePivot","message")
        nodeBuilder.addAttr("ikFootMarker","message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)


    def characterize(self, **kwargs):
        parentGroup=kwargs.get("p", None)
        '''Setup Control Rig Group'''
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # Setup System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        cmds.addAttr(self.systemGroup, ln="ikMode", at="float", min=0, max=1, dv=1, k=1)
        #
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        #Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerUpperLeg = Marker.create(n="markerUpperLeg", t=self.boneUpperLeg, p=self.markerGroup)
        self.markerLowerLeg = Marker.create(n="markerLowerLeg", t=self.boneLowerLeg, p=self.markerGroup)
        self.markerHock = Marker.create(n="markerHock", t=self.boneHock, p=self.markerGroup)
        self.markerFoot = Marker.create(n="markerFoot", t=self.boneFoot, p=self.markerGroup)
        self.markerToes = Marker.create(n="markerToes", t=self.boneToes, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [-1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        if self.side == 2:
            aimAxis = [ aimAxis[0] * -1, aimAxis[1] * -1, aimAxis[2] * -1]
        endPoint = cmds.xform(self.markerToes, q=True, ws=True, rp=True)
        endPoint = [endPoint[0], endPoint[1], endPoint[2]+1]
        Marker.orient(self.markerUpperLeg, target=self.markerLowerLeg, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerLowerLeg, target=self.markerHock, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerHock, target=self.markerFoot, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerFoot, target=self.markerToes, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerToes, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneUpperLeg", self.boneUpperLeg)
        RigNode.setPlug(self.node, "boneLowerLeg", self.boneLowerLeg)
        RigNode.setPlug(self.node, "boneHock", self.boneHock)
        RigNode.setPlug(self.node, "boneFoot", self.boneFoot)
        RigNode.setPlug(self.node, "boneToes", self.boneToes)
        RigNode.setPlug(self.node, "markerUpperLeg", self.markerUpperLeg)
        RigNode.setPlug(self.node, "markerLowerLeg", self.markerLowerLeg)
        RigNode.setPlug(self.node, "markerHock", self.markerHock)
        RigNode.setPlug(self.node, "markerFoot", self.markerFoot)
        RigNode.setPlug(self.node, "markerToes", self.markerToes)


    def rig(self, **kwargs):
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
        #-FK Channels
        cmds.addAttr(self.systemGroup, ln="fkLimbScale", k=True, at="float", dv=1, min=.01)
        #-IK Channels
        #cmds.addAttr(self.systemGroup, ln="ikSpace", at="enum", en=":world:hips", k=True)
        cmds.addAttr(self.systemGroup, ln="ikPoleVectorFollow", at="float", k=True, min=0, max=1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikStretch", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikLimbScale", at="float", k=True, min=0.1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikSquash", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikSquashScale", at="float", k=True, min=0.01, dv=1)
        #Setup Controls
        fkBones = self.setupFK()
        ikBones = self.setupIK()
        #Setup IK/FK Switching
        weightSwitch = RigUtility.blendJointChain(boneList, fkBones, ikBones)
        cmds.connectAttr(self.systemGroup+".ikMode", weightSwitch+".input", f=True)
        cmds.connectAttr(weightSwitch+".outputInverse", self.fkSpace+".v", f=True)
        cmds.connectAttr(weightSwitch+".output", self.ikSpace+".v", f=True)
        #Stretchy - Scale Constraint
        stretchSetList = [
            {"FK":fkBones[0], "IK":ikBones[0], "TARGET":boneList[0]},
            {"FK":fkBones[1], "IK":ikBones[1], "TARGET":boneList[1]},
            {"FK":fkBones[2], "IK":ikBones[2], "TARGET":boneList[2]},
            {"FK":fkBones[3], "IK":ikBones[3], "TARGET":boneList[3]},
            {"FK":fkBones[4], "IK":ikBones[4], "TARGET":boneList[4]}]
        for stretchSet in stretchSetList:
            blendNodeX = cmds.createNode("blendTwoAttr", n=self.instanceName+"_BlendScaleX")
            blendNodeYZ = cmds.createNode("blendTwoAttr", n=self.instanceName+"_BlendScaleYZ")
            cmds.connectAttr(self.systemGroup+".ikMode", blendNodeX+".attributesBlender")
            cmds.connectAttr(self.systemGroup+".ikMode", blendNodeYZ+".attributesBlender")
            cmds.connectAttr(stretchSet["FK"]+".scaleX", blendNodeX+".input[0]")
            cmds.connectAttr(stretchSet["IK"]+".scaleX", blendNodeX+".input[1]")
            cmds.connectAttr(stretchSet["FK"]+".scaleY", blendNodeYZ+".input[0]")
            cmds.connectAttr(stretchSet["IK"]+".scaleY", blendNodeYZ+".input[1]")
            cmds.connectAttr(blendNodeX+".output", stretchSet["TARGET"]+".scaleX")
            cmds.connectAttr(blendNodeYZ+".output", stretchSet["TARGET"]+".scaleY")
            cmds.connectAttr(blendNodeYZ+".output", stretchSet["TARGET"]+".scaleZ")
        # Lock ControlGroup and SystemGround Channels
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        #Lock Unused FK Channels
        RigUtility.modifyTransformChannels(self.fkSpace)
        FKControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes]
        for ctrl in FKControls:
            RigUtility.modifyTransformChannels(ctrl, r=False)
        #Lock Unused IK Channels
        RigUtility.modifyTransformChannels(self.ikSpace)
        RigUtility.modifyTransformChannels(self.ikFoot, t=False, r=False)
        RigUtility.modifyTransformChannels(self.ikKnee, t=False)
        RigUtility.modifyTransformChannels(self.ikHock, r=False)
        RigUtility.modifyTransformChannels(self.ikHeelLift, r=False)
        RigUtility.modifyTransformChannels(self.ikHeelPivot, r=False)
        RigUtility.modifyTransformChannels(self.ikBallPivot, r=False)
        RigUtility.modifyTransformChannels(self.ikToePivot, r=False)
        cmds.setAttr(self.ikHock+".rotateX", l=True, k=False, cb=False)
        #ProxyAttrs
        # | ControlGroup - FK
        cmds.addAttr(self.controlGroup, ln="ikMode", proxy=self.systemGroup+".ikMode")
        cmds.addAttr(self.controlGroup, ln="FK", k=True)
        cmds.setAttr(self.controlGroup+".FK", l=True)
        cmds.addAttr(self.controlGroup, ln="fkSpace", proxy=self.systemGroup+".fkSpace")
        cmds.addAttr(self.controlGroup, ln="fkLimbScale", proxy=self.systemGroup+".fkLimbScale")
        # | ControlGroup - IK
        cmds.addAttr(self.controlGroup, ln="IK", k=True)
        cmds.setAttr(self.controlGroup+".IK", l=True)
        cmds.addAttr(self.controlGroup, ln="ikSpace", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.controlGroup, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.controlGroup, ln="ikLimbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.controlGroup, ln="ikStretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.controlGroup, ln="ikSquash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.controlGroup, ln="ikSquashScale", proxy=self.systemGroup+".ikSquashScale")
        cmds.addAttr(self.controlGroup, ln="showFootControls", proxy=self.systemGroup+".showIKFootControls")
        cmds.addAttr(self.controlGroup, ln="hockFwdBack", proxy=self.ikHock+".rotateY")
        cmds.addAttr(self.controlGroup, ln="hockLeftRight", proxy=self.ikHock+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="heelLiftUpDown", proxy=self.ikHeelLift+".rotateY")
        cmds.addAttr(self.controlGroup, ln="heelLiftLeftRight", proxy=self.ikHeelLift+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="heelLiftTwist", proxy=self.ikHeelLift+".rotateX")
        cmds.addAttr(self.controlGroup, ln="heelPivotUpDown", proxy=self.ikHeelPivot+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="heelPivotLeftRight", proxy=self.ikHeelPivot+".rotateY")
        cmds.addAttr(self.controlGroup, ln="heelPivotTwist", proxy=self.ikHeelPivot+".rotateX")
        cmds.addAttr(self.controlGroup, ln="ballPivotUpDown", proxy=self.ikBallPivot+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="ballPivotLeftRight", proxy=self.ikBallPivot+".rotateY")
        cmds.addAttr(self.controlGroup, ln="ballPivotTwist", proxy=self.ikBallPivot+".rotateX")
        cmds.addAttr(self.controlGroup, ln="toePivotUpDown", proxy=self.ikToePivot+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="toePivotLeftRight", proxy=self.ikToePivot+".rotateY")
        cmds.addAttr(self.controlGroup, ln="toePivotTwist", proxy=self.ikToePivot+".rotateX")
        # | Foot
        cmds.addAttr(self.ikFoot, ln="IK", k=True)
        cmds.setAttr(self.ikFoot+".IK", l=True)
        cmds.addAttr(self.ikFoot, ln="space", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.ikFoot, ln="poleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.ikFoot, ln="limbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.ikFoot, ln="stretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.ikFoot, ln="squash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.ikFoot, ln="squashScale", proxy=self.systemGroup+".ikSquashScale")
        # | Foot-Pivots
        cmds.addAttr(self.ikFoot, ln="Pivots", k=True)
        cmds.setAttr(self.ikFoot+".Pivots", l=True)
        #
        cmds.addAttr(self.ikFoot, ln="showFootControls", proxy=self.systemGroup+".showIKFootControls")
        #
        cmds.addAttr(self.ikFoot, ln="hockFwdBack", proxy=self.ikHock+".rotateY")
        cmds.addAttr(self.ikFoot, ln="hockLeftRight", proxy=self.ikHock+".rotateZ")
        #
        cmds.addAttr(self.ikFoot, ln="heelLiftUpDown", proxy=self.ikHeelLift+".rotateY")
        cmds.addAttr(self.ikFoot, ln="heelLiftLeftRight", proxy=self.ikHeelLift+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="heelLiftTwist", proxy=self.ikHeelLift+".rotateX")
        #
        cmds.addAttr(self.ikFoot, ln="heelPivotUpDown", proxy=self.ikHeelPivot+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="heelPivotLeftRight", proxy=self.ikHeelPivot+".rotateY")
        cmds.addAttr(self.ikFoot, ln="heelPivotTwist", proxy=self.ikHeelPivot+".rotateX")
        #
        cmds.addAttr(self.ikFoot, ln="ballPivotUpDown", proxy=self.ikBallPivot+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="ballPivotLeftRight", proxy=self.ikBallPivot+".rotateY")
        cmds.addAttr(self.ikFoot, ln="ballPivotTwist", proxy=self.ikBallPivot+".rotateX")
        #
        cmds.addAttr(self.ikFoot, ln="toePivotUpDown", proxy=self.ikToePivot+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="toePivotLeftRight", proxy=self.ikToePivot+".rotateY")
        cmds.addAttr(self.ikFoot, ln="toePivotTwist", proxy=self.ikToePivot+".rotateX")
        #Hide SystemGroup
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        #Attach to RigNode
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkSpace", self.fkSpace)
        RigNode.setPlug(self.node, "fkUpperLeg", self.fkUpperLeg)
        RigNode.setPlug(self.node, "fkLowerLeg", self.fkLowerLeg)
        RigNode.setPlug(self.node, "fkHock", self.fkHock)
        RigNode.setPlug(self.node, "fkFoot", self.fkFoot)
        RigNode.setPlug(self.node, "fkToes", self.fkToes)
        RigNode.setPlug(self.node, "ikSpace", self.ikSpace)
        RigNode.setPlug(self.node, "ikFoot", self.ikFoot)
        RigNode.setPlug(self.node, "ikKnee", self.ikKnee)
        RigNode.setPlug(self.node, "ikHock", self.ikHock)
        RigNode.setPlug(self.node, "ikHeelLift", self.ikHeelLift)
        RigNode.setPlug(self.node, "ikHeelPivot", self.ikHeelPivot)
        RigNode.setPlug(self.node, "ikBallPivot", self.ikBallPivot)
        RigNode.setPlug(self.node, "ikToePivot", self.ikToePivot)
        RigNode.setPlug(self.node, "ikToes", self.ikToes)
        RigNode.setPlug(self.node, "ikFootMarker", self.ikFootMarker)
        # Set Rigged Status
        self.rigged = True


    #FK SETUP
    def setupFK(self):
        bonePelvis = RigUtility.firstParentOf(self.boneUpperLeg)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
        fkBoneSpace = RigUtility.createLocalSpace("FKBones", boneList[0], self.systemGroup)
        fkBones = RigUtility.cloneJointChain(boneList, "FKBone_", fkBoneSpace)
        #Create Local Space - FK
        self.fkSpace = RigUtility.createLocalSpace("FK", self.boneUpperLeg, self.controlGroup)
        spaceSwitch = SpaceSwitch2.create(self.fkSpace, bonePelvis)
        spaceSwitch.addSpace("hips", bonePelvis)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "fkSpace")
        #|- Create Controls
        self.fkUpperLeg = Controller.create(
            name = Controller.buildName("upperleg", self.side, "FK"),
            parent = self.fkSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        self.fkLowerLeg = Controller.create(
            name = Controller.buildName("lowerleg", self.side, "FK"),
            parent = self.fkUpperLeg,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        self.fkHock = Controller.create(
            name = Controller.buildName("hock", self.side, "FK"),
            parent = self.fkLowerLeg,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        self.fkFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "FK"),
            parent = self.fkHock,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        self.fkToes = Controller.create(
            name = Controller.buildName("toes", self.side, "FK"),
            parent = self.fkFoot,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        #|- update position/rotation
        cmds.matchTransform(self.fkUpperLeg, self.markerUpperLeg)
        cmds.matchTransform(self.fkLowerLeg, self.markerLowerLeg)
        cmds.matchTransform(self.fkHock, self.markerHock)
        cmds.matchTransform(self.fkFoot, self.markerFoot)
        cmds.matchTransform(self.fkToes, self.markerToes)
        #|- bake transforms to offset parent matrix
        RigUtility.bakeOffsetParentMatrix(self.fkUpperLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkLowerLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkHock)
        RigUtility.bakeOffsetParentMatrix(self.fkFoot)
        RigUtility.bakeOffsetParentMatrix(self.fkToes)
        #
        cmds.parentConstraint(self.fkUpperLeg, fkBones[0], mo=True)
        cmds.parentConstraint(self.fkLowerLeg, fkBones[1], mo=True)
        cmds.parentConstraint(self.fkHock, fkBones[2], mo=True)
        cmds.parentConstraint(self.fkFoot, fkBones[3], mo=True)
        cmds.parentConstraint(self.fkToes, fkBones[4], mo=True)
        # ----------------- Stretch (FK) --------------------
        RigUtility.setupFKStretch(self.fkLowerLeg, self.systemGroup+".fkLimbScale")
        RigUtility.setupFKStretch(self.fkHock, self.systemGroup+".fkLimbScale")
        #return the bones
        return fkBones


    def setupIK(self):
        cmds.addAttr(self.systemGroup, at="bool", ln="showIKFootControls", k=True)
        cmds.setAttr(self.systemGroup+".showIKFootControls", k=False, cb=True)
        IK_PIVOT_OFFSET = (IK_FOOT_SIZE[0] + IK_PIVOT_SIZE[0]) * 0.5 + 1
        direction = [1,0,0]
        if self.side == 2:
            direction = [-1,0,0]
        mv_direction = OpenMaya.MVector(direction)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
        ikBones = RigUtility.cloneJointChain(boneList, "IKBone_", self.systemGroup)
        #Create Local Space - IK
        self.ikSpace = RigUtility.createLocalSpace("IK", self.boneUpperLeg, self.controlGroup)
        #|- Space Switch
        bonePelvis = RigUtility.firstParentOf(self.boneUpperLeg)
        spaceSwitch = SpaceSwitch2.create(self.ikSpace, None)
        spaceSwitch.addSpace("hips", bonePelvis)
        spaceSwitch.addSpace("world", self.rootSpace, True)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "ikSpace")
        # ------ Controllers ------ #
        # --- IK Foot --- #
        self.ikFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "IK"),
            shape = Controller.Shape.PLANE,
            color = Controller.Color.YELLOW,
            size = IK_FOOT_SIZE,
            normal = [0, 1, 0],
            parent = self.ikSpace
            )
        cmds.matchTransform(self.ikFoot, self.markerFoot, position=1, rotation=0, scale=0)
        foot_offset = IK_FOOT_SIZE[2] * .25
        Controller.moveCurve(self.ikFoot, [0,0,foot_offset])
        RigUtility.bakeOffsetParentMatrix(self.ikFoot)
        # --- IK Foot Marker--- #
        self.ikFootMarker = cmds.createNode("transform", n="IKFootMarker", p=self.systemGroup)
        cmds.matchTransform(self.ikFootMarker, self.ikFoot, position=1, rotation=1, scale=0)
        RigUtility.bakeOffsetParentMatrix(self.ikFootMarker)
        cmds.parentConstraint(self.boneFoot, self.ikFootMarker, mo=True)
        # - IK Knee (Pole Vector) Controller
        self.ikKnee = Controller.create(
            name = Controller.buildName("knee", self.side, "IK"),
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.YELLOW,
            size = IK_KNEE_SIZE,
            normal = [0,1,0],
            parent = self.ikSpace
            )
        pos = RigUtility.calculatePoleVector(self.markerUpperLeg, self.markerLowerLeg, self.markerHock)
        cmds.xform(self.ikKnee, ws=True, t=pos)
        RigUtility.bakeOffsetParentMatrix(self.ikKnee)
        # --- IK Toe Pivot --- #
        self.ikToePivot = Controller.create(
            name = Controller.buildName("toepivot", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [0,1,0],
            size = IK_PIVOT_SIZE)
        cmds.matchTransform(self.ikToePivot, self.markerToes)
        pointA = OpenMaya.MPoint(cmds.xform(self.markerFoot, q=True, ws=True, t=True))
        pointB = OpenMaya.MPoint(cmds.xform(self.markerToes, q=True, ws=True, t=True))
        foot_length = pointA.distanceTo(pointB)
        pos = cmds.xform(self.ikToePivot, q=True, ws=True, t=True)
        cmds.xform(self.ikToePivot, ws=True, t=[pos[0], 0, pos[2]])
        cmds.move(foot_length, self.ikToePivot, r=True, z=True, ws=True)
        Controller.moveCurve(self.ikToePivot, mv_direction * IK_PIVOT_OFFSET)
        RigUtility.bakeOffsetParentMatrix(self.ikToePivot)
        RigUtility.parentByMatrix(self.ikToePivot, self.ikFoot, mo=True)
        # --- IK Ball Pivot --- #
        self.ikBallPivot = Controller.create(
            name = Controller.buildName("ballpivot", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [0,1,0],
            size = IK_PIVOT_SIZE)
        cmds.matchTransform(self.ikBallPivot, self.markerToes)
        pos = cmds.xform(self.ikBallPivot, q=True, ws=True, t=True)
        cmds.xform(self.ikBallPivot, ws=True, t=[pos[0], 0, pos[2]])
        Controller.moveCurve(self.ikBallPivot, mv_direction * IK_PIVOT_OFFSET)
        RigUtility.bakeOffsetParentMatrix(self.ikBallPivot)
        RigUtility.parentByMatrix(self.ikBallPivot, self.ikToePivot, mo=True)
        # --- IK Heel Pivot --- #
        self.ikHeelPivot = Controller.create(
            name = Controller.buildName("heelpivot", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [0,1,0],
            size = IK_PIVOT_SIZE)
        cmds.matchTransform(self.ikHeelPivot, self.markerFoot)
        pos = cmds.xform(self.ikHeelPivot, q=True, ws=True, t=True)
        cmds.xform(self.ikHeelPivot, ws=True, t=[pos[0], 0, pos[2]])
        Controller.moveCurve(self.ikHeelPivot, mv_direction * IK_PIVOT_OFFSET)
        RigUtility.bakeOffsetParentMatrix(self.ikHeelPivot)
        RigUtility.parentByMatrix(self.ikHeelPivot, self.ikBallPivot, mo=True)
        # --- IK Heel Lift --- #
        self.ikHeelLift = Controller.create(
            name = Controller.buildName("heellift", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [0,1,0],
            size = IK_PIVOT_SIZE)
        # |- position heel controller
        cmds.matchTransform(self.ikHeelLift, self.markerToes)
        planeNormal = RigUtility.calculatePlaneNormalTransform(self.markerHock, self.markerFoot, self.markerToes)
        RigUtility.aim(self.ikHeelLift, target=self.markerFoot, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=planeNormal)
        pointA = OpenMaya.MVector(cmds.xform(self.markerFoot, q=True, ws=True, t=True))
        pointB = OpenMaya.MVector(cmds.xform(self.markerToes, q=True, ws=True, t=True))
        foot_length = (pointB-pointA).length()
        translatePoint = pointA + OpenMaya.MVector(0,0,-1 * foot_length)
        cmds.xform(self.ikHeelLift, ws=True, t=translatePoint)
        cmds.xform(self.ikHeelLift, ws=True, piv=pointB)
        # |- make ikHeelLift a child of ikFoot by matrix
        RigUtility.bakeOffsetParentMatrix(self.ikHeelLift)
        RigUtility.parentByMatrix(self.ikHeelLift, self.ikBallPivot, mo=True)
        # --- IKHock --- #
        self.ikHock = Controller.create(
            name = Controller.buildName("hock", self.side, "IK"),
            parent=self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [1,0,0],
            size = IK_HOCK_SIZE)
        # |- position hock controller
        #---mid point position
        midpoint = RigUtility.getAveragePosition([self.markerHock, self.markerFoot])
        cmds.xform(self.ikHock, ws=True, t=midpoint)
        #---orientation via aim
        planeNormal = RigUtility.calculatePlaneNormalTransform(self.markerLowerLeg, self.markerHock, self.markerFoot)
        RigUtility.aim(self.ikHock, target=self.markerHock, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=planeNormal)
        #---pivot placed at root
        rootPoint = cmds.xform(self.boneFoot, q=True, ws=True, t=True)
        cmds.xform(self.ikHock, ws=True, piv=rootPoint)
        # |- make ikHeelLift a child of ikFoot by matrix
        RigUtility.bakeOffsetParentMatrix(self.ikHock)
        RigUtility.parentByMatrix(self.ikHock, self.ikHeelLift, mo=True)
        # --- IK Toes --- #
        self.ikToes = Controller.create(
            name = Controller.buildName("toes", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = IK_TOES_SIZE,
            normal = [1,0,0])
        cmds.matchTransform(self.ikToes, self.markerToes, position=1, rotation=1, scale=0)
        Controller.moveCurve(self.ikToes, [0,5,0])
        RigUtility.bakeOffsetParentMatrix(self.ikToes)
        RigUtility.parentByMatrix(self.ikToes, self.ikHeelPivot, mo=True)
        # IKHandles
        ik_leg = cmds.ikHandle(solver="ikRPsolver" , sj=ikBones[0], ee=ikBones[2])[0]
        ik_heel = cmds.ikHandle(solver="ikSCsolver" , sj=ikBones[2], ee=ikBones[3])[0]
        ik_toes = cmds.ikHandle(solver="ikSCsolver" , sj=ikBones[3], ee=ikBones[4])[0]
        ik_leg = cmds.parent(ik_leg, self.systemGroup)[-1]
        ik_heel = cmds.parent(ik_heel, self.systemGroup)[-1]
        ik_toes = cmds.parent(ik_toes, self.systemGroup)[-1]
        # Constraints
        # - Hip
        boneParent = RigUtility.firstParentOf(self.boneUpperLeg)
        if boneParent != None:
            cmds.parentConstraint(boneParent, ikBones[0], mo=True)
        # - Foot
        cmds.orientConstraint(self.ikFoot, ikBones[3], mo=True)
        # - Knee
        cmds.poleVectorConstraint(self.ikKnee, ik_leg)
        # - Hock
        cmds.parentConstraint(self.ikHock, ik_leg, mo=True)
        # - Heel
        cmds.parentConstraint(self.ikHeelLift, ik_heel, mo=True)
        # - Toes
        cmds.parentConstraint(self.ikHeelPivot, ik_toes, mo=True)
        cmds.orientConstraint(self.ikToes, ikBones[4], mo=True)
        # - PoleVector Following
        opm = cmds.getAttr(self.ikKnee+".offsetParentMatrix")
        opmNode = RigUtility.convertMatrixToNode(opm)
        blendMatrix = cmds.createNode("blendMatrix")
        pvMarker = cmds.createNode("transform", n="ik_knee_Marker", p=self.systemGroup)
        cmds.matchTransform(pvMarker, self.ikKnee)
        cmds.parentConstraint(self.ikFoot, pvMarker, mo=True)
        multMatrix = cmds.createNode("multMatrix")
        cmds.connectAttr(pvMarker+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikSpace+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        cmds.connectAttr(opmNode+".matrixSum", blendMatrix+".inputMatrix")
        cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".ikPoleVectorFollow", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.ikKnee+".offsetParentMatrix")
        #-------------------- IK Stretch --------------------------
        #|-calculate default length of chain
        lengthA = RigUtility.calculateDistanceBetweenTransforms(ikBones[0], ikBones[1])
        lengthB = RigUtility.calculateDistanceBetweenTransforms(ikBones[1], ikBones[2])
        defaultLength = lengthA + lengthB
        #|-create distance node to get the 'current' length of chain
        leg_ik_marker = cmds.createNode("transform", n="leg_ik_marker", p=self.systemGroup)
        cmds.matchTransform(leg_ik_marker, ikBones[2], pos=True, rotation=0, scale=0)
        RigUtility.bakeOffsetParentMatrix(leg_ik_marker)
        RigUtility.parentByMatrix(leg_ik_marker, self.ikHock, mo=True)
        distanceNode = UtilityNodes.createDistanceNode(
            n=self.instanceName+"_IK_CurrentDistance",
            transformA=ikBones[0],
            transformB=leg_ik_marker,
            p=self.systemGroup)
        #|-wire up the stretchNode
        stretchNode = UtilityNodes.createStretchNode(n=self.instanceName+"_IK_StretchNode")
        cmds.setAttr(stretchNode+".defaultLength", defaultLength)
        cmds.connectAttr(distanceNode+".distance", stretchNode+".currentLength")
        cmds.connectAttr(self.systemGroup+".ikStretch", stretchNode+".weight")
        #|-(Clamp the stretch affect to not 'shrink' the arm)
        stretchMaxNode = cmds.createNode("floatMath", n=self.instanceName+"_IK_StretchFactorMaxNode")
        cmds.setAttr(stretchMaxNode+".operation", 5)
        cmds.connectAttr(self.systemGroup+".ikLimbScale", stretchMaxNode+".floatA")
        cmds.connectAttr(stretchNode+".stretchFactor", stretchMaxNode+".floatB")
        #Squash
        squashNode = UtilityNodes.createSquashNode(n=self.instanceName+"_IK_SquashNode")
        cmds.connectAttr(stretchNode+".stretchFactor", squashNode+".stretchFactor")
        cmds.connectAttr(self.systemGroup+".ikSquash", squashNode+".weight")
        #Volume
        squashScaleNode = cmds.createNode("floatMath", n=self.instanceName+"_IK_SquashVolumeNode")
        cmds.setAttr(squashScaleNode+".operation", 2)
        cmds.connectAttr(squashNode+".squashFactor", squashScaleNode+".floatA")
        cmds.connectAttr(self.systemGroup+".ikSquashScale", squashScaleNode+".floatB")
        #Connect Squash/Stretch to Bone Scale
        #-X Axis
        cmds.connectAttr(stretchMaxNode+".outFloat", ikBones[0]+".scaleX")
        cmds.connectAttr(stretchMaxNode+".outFloat", ikBones[1]+".scaleX")
        #-YZ Axis
        for bone in ikBones:
            cmds.connectAttr(squashScaleNode+".outFloat", bone+".scaleY")
            cmds.connectAttr(squashScaleNode+".outFloat", bone+".scaleZ")
        #Visibility for SubControls
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikHeelLift+".v", f=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikHeelPivot+".v", f=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikBallPivot+".v", f=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikToePivot+".v", f=True)
        return ikBones


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        '''Globals'''
        cmds.text(l="  Global", width=350, align="left", bgc=(.4,.4,.4))
        #IK FK Switch Widget
        Widgets.WAttributeToggle.Create(
            self.controlGroup+".ikMode",
            label="IK Mode",
            labelWidth=200,
            fieldWidth=150,
            buttonLabel0="FK",
            buttonLabel1="IK",
            changeCommand=self.setIKMode,
            keyCommand=self.keyIKMode)
        '''FK Group'''
        cmds.text(l="  FK", width=350, align="left", bgc=(.4,.4,.4))
        fkSpaceAttr = "{}.fkSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(
            fkSpaceAttr,
            label="FK Space",
            labelWidth=200,
            fieldWidth=150,
            changeCommand=self.setFKSpace,
            keyCommand=self.keyFKSpace)
        #FK Attributes
        Widgets.WAttributeField.Create(self.controlGroup+'.fkLimbScale', label="FK Limb Scale")
        cmds.separator( height=15)
        '''IK GROUP'''
        cmds.text(l="  IK", width=350, align="left", bgc=(.4,.4,.4))
        #IK Space Switch
        ikSpaceAttr = "{}.ikSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(
            ikSpaceAttr,
            label="IK Space",
            labelWidth=200,
            fieldWidth=150,
            changeCommand=self.setIKSpace,
             keyCommand=self.keyIKSpace)
        #IK Pole Vector Follow
        Widgets.WAttributeToggle.Create(
            self.controlGroup+".ikPoleVectorFollow",
            label="IK Knee Follow", labelWidth=200,
            fieldWidth=150,
            changeCommand=self.setIKPoleVectorFollow,
            keyCommand=self.keyIKPoleVectorFollow)
        Widgets.WAttributeToggle.Create(
            self.controlGroup+".ikStretch",
            label="IK Stretch",
            labelWidth=200,
            fieldWidth=150)
        Widgets.WAttributeToggle.Create(
            self.controlGroup+".ikSquash",
            label="IK Squash",
            labelWidth=200,
            fieldWidth=150)
        #IK Leg Attributes
        ikAttrList = [
            ["IK Limb Scale", "ikLimbScale"],
            ["IK Squash Scale", "ikSquashScale"]]
        for attr in ikAttrList:
            label = attr[0]
            attributePath = "{0}.{1}".format(self.controlGroup, attr[1])
            Widgets.WAttributeField.Create(attributePath, label=label)
        cmds.setParent('..')


    def reset(self, *args):
        #reset control list
        controlList = [self.ikFoot, self.ikKnee, self.ikHock, self.ikHeelLift, self.ikToes,
        self.ikHeelPivot, self.ikBallPivot, self.ikToePivot,
        self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.ikHeelLift, self.fkFoot, self.fkToes ]
        attributeList = ["translateX", "translateY", "translateZ", "rotateX", "rotateY", "rotateZ"]
        for control in controlList:
            for attr in attributeList:
                attrPath = "{}.{}".format(control, attr)
                if cmds.getAttr(attrPath, l=True) == False:
                    cmds.setAttr(attrPath, 0)


    def sync(self, *args, **kwargs):
        orgIKValue = cmds.getAttr(self.systemGroup+".ikMode")
        #
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        # If it's an external node - temporarily switch us to FK
        if node != self.node:
            cmds.setAttr(self.controlGroup+".ikMode", 0)
        # --- Markers --- #
        #Note: Using RigNode.GetPlug because FK controls can be used to sync to EXTERNAL bone heirarchies.
        # The ik contorls only reference the internal bones so it is read directly from the current instance
        markerList = [
            RigNode.getPlug(node, "markerUpperLeg"),
            RigNode.getPlug(node, "markerLowerLeg"),
            RigNode.getPlug(node, "markerHock"),
            RigNode.getPlug(node, "markerFoot"),
            RigNode.getPlug(node, "markerToes")]
        # Marker Points
        pointList = []
        for i in range(0, len(markerList)):
            pointList.append(cmds.xform(markerList[i], q=True, ws=True, t=True))
        # --- FK --- #
        fkControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes]
        for i in range(0,len(fkControls)):
            cmds.matchTransform(fkControls[i], markerList[i], pos=0, rot=1, scl=0)
        cmds.setAttr(self.controlGroup+".ikMode", 0)
        # --- IK --- #
        #Reset Secondary IK Pivots
        controlList = [self.ikHeelPivot, self.ikBallPivot, self.ikToePivot]
        attributeList = ["rotateX", "rotateY", "rotateZ"]
        for control in controlList:
            for attr in attributeList:
                attrPath = "{}.{}".format(control, attr)
                if cmds.getAttr(attrPath, l=True) == False:
                    cmds.setAttr(attrPath, 0)
        # --- IK Foot
        cmds.matchTransform(self.ikFoot, self.ikFootMarker)
        # --- IK Heel
        planeNormal = RigUtility.calculatePlaneNormal(pointList[2], pointList[3], pointList[4])
        RigUtility.aim(self.ikHeelLift, target=self.markerFoot, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=planeNormal)
        # --- IK Hock
        lowerPlaneNormal = RigUtility.calculatePlaneNormal(pointList[1], pointList[2], pointList[3])
        RigUtility.aim(self.ikHock, target=self.markerHock, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=lowerPlaneNormal)
        # --- IK Knee
        pvTarget = RigUtility.calculatePoleVectorPoints(pointList[0], pointList[1], pointList[2])
        cmds.xform(self.ikKnee, ws=True, t=pvTarget)
        # --- IK Toes
        cmds.matchTransform(self.ikToes, markerList[4], pos=0, rot=1, scl=0)
        #reset to org value
        cmds.setAttr(self.systemGroup+".ikMode", orgIKValue)


    def setIKMode(self, value, *args):
        self.sync()
        cmds.setAttr(self.controlGroup+".ikMode", value)


    def keyIKMode(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikMode")
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikKnee)
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.fkLowerLeg)
        cmds.setKeyframe(self.fkHock)
        cmds.setKeyframe(self.fkFoot)


    def setIKPoleVectorFollow(self, value, *args):
        pv_worldMatrix = cmds.xform(self.ikKnee, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikPoleVectorFollow", value)
        cmds.xform(self.ikKnee, ws=True, m=pv_worldMatrix)


    def keyIKPoleVectorFollow(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikPoleVectorFollow")
        cmds.setKeyframe(self.ikKnee)


    def keyAll(self, *args):
        attrList = ["fkSpace", "fkLimbScale",
        "ikMode", "ikSpace", "ikPoleVectorFollow", "ikLimbScale", "ikStretch", "ikSquash"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)
        controlList = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes,
        self.ikFoot, self.ikKnee, self.ikHock, self.ikHeelLift, self.ikToes]
        for control in controlList:
            cmds.setKeyframe(control)

    ###
    def setFKSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkUpperLeg, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpace", index)
        cmds.xform(self.fkUpperLeg, ws=True, m=worldMatrix)


    def setIKSpace(self, index, *args):
        foot_worldMatrix = cmds.xform(self.ikFoot, q=True, ws=True, m=True)
        pv_worldMatrix = cmds.xform(self.ikKnee, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikSpace", index)
        cmds.xform(self.ikFoot, ws=True, m=foot_worldMatrix)
        cmds.xform(self.ikKnee, ws=True, m=pv_worldMatrix)


    def keyIKSpace(self, *args):
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikKnee)
        cmds.setKeyframe(self.controlGroup+".ikSpace")


    def keyFKSpace(self, *args):
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.controlGroup+".fkSpace")

    ###