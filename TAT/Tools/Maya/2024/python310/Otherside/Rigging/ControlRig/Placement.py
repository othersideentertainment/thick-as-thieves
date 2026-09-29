from collections import OrderedDict
import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UI.Widgets as Widgets
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase

RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Placement"
CLASS_NAME = "Placement"
CONTROLLER_SIZE = [15, 15, 15]


class Placement(ControlRigBase):
    def __init__(self, name="Placement", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = 0
        self.characterized = False
        self.rigged = False
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        self.boneRoot = None
        self.markerRoot = None
        self.controllerPlacement = None
        self.controllerRoot = None


    @staticmethod
    def load(node):
        instance = Placement()
        instance.node = node
        instance.spaces = {}
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneRoot = RigNode.getPlug(node, "boneRoot")
        instance.markerRoot = RigNode.getPlug(node, "markerRoot")
        instance.controllerPlacement = RigNode.getPlug(node, "controllerPlacement")
        instance.controllerRoot = RigNode.getPlug(node, "controllerRoot")
        return instance


    def setBoneList(self, boneList):
        self.boneRoot = boneList[0]


    def getBoneList(self):
        return [self.boneRoot]


    def getBoneNames(self):
        return ["boneRoot"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.controllerPlacement,
            self.controllerRoot
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerPlacement",
            "controllerRoot"
            ]


    def getMarkerList(self):
        return [self.markerRoot]


    def getMarkerNames(self):
        return ["markerRoot"]


    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneRoot", "message")
        nodeBuilder.addAttr("markerRoot", "message")
        nodeBuilder.addAttr("controllerPlacement", "message")
        nodeBuilder.addAttr("controllerRoot", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, **kwargs):
        #kwargs
        parentGroup = kwargs.get("p", None)
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.controlGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        # Marker Gorup
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        # System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        # Create Markers
        self.markerRoot = Marker.create(n="markerRoot", t=self.boneRoot, p=self.markerGroup)
        Marker.orient(self.markerRoot, targetPoint=[0,0,10], aimAxis=[0,0,1], upAxis=[0,1,0], worldAxis=[0,1,0])
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneRoot", self.boneRoot)
        RigNode.setPlug(self.node, "markerRoot", self.markerRoot)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        #Controller - Placement
        self.controllerPlacement = Controller.create(
            name = "placement_controller",
            parent = self.controlGroup,
            shape = Controller.Shape.TRIANGLE,
            color = Controller.Color.GREEN,
            size = [50,50,50],
            normal = [0,1,0]
            )
        #Controller - Root
        self.controllerRoot = Controller.create(
            name = "root_controller",
            parent = self.controlGroup,
            shape = Controller.Shape.TRIANGLE,
            color = Controller.Color.RED,
            size = [5,5,5],
            normal = [0,1,0]
            )
        #
        cmds.addAttr(self.systemGroup, ln="follow", at="float", min=0, max=1, dv=1, k=True)
        cmds.addAttr(self.controlGroup, ln="follow", proxy=self.systemGroup+".follow")
        multMatrix= cmds.createNode("multMatrix")
        cmds.connectAttr(self.controllerPlacement+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        cmds.connectAttr(self.controlGroup+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        blendMatrix = cmds.createNode("blendMatrix")
        cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".follow", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.controllerRoot+".offsetParentMatrix")
        cmds.parentConstraint(self.controllerRoot, self.boneRoot)
        #Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode Plugs
        cmds.setAttr(self.node+".rigged", True)
        RigNode.setPlug(self.node, "controllerPlacement", self.controllerPlacement)
        RigNode.setPlug(self.node, "controllerRoot", self.controllerRoot)
        self.rigged = True


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        Widgets.WAttributeToggle.Create(self.controlGroup+".follow", label="Root Follow", labelWidth=200, fieldWidth=150, changeCommand=self.setRootFollow, keyCommand=self.keyRootFollow)
        cmds.setParent('..')


    def setRootFollow(self, value, *args):
        worldMatrix = cmds.xform(self.controllerRoot, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".follow", value)
        cmds.xform(self.controllerRoot, ws=True, m=worldMatrix)


    def keyRootFollow(self, *args):
        cmds.setKeyframe(self.controlGroup+".follow")
        cmds.setKeyframe(self.controllerRoot)


    def reset(self, *args):
        cmds.setAttr(self.controlGroup+".follow", 1)
        #reset control list
        controlList = [self.controllerPlacement, self.controllerRoot]
        for control in controlList:
            cmds.setAttr(control+".translateX", 0)
            cmds.setAttr(control+".translateY", 0)
            cmds.setAttr(control+".translateZ", 0)
            cmds.setAttr(control+".rotateX", 0)
            cmds.setAttr(control+".rotateY", 0)
            cmds.setAttr(control+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setKeyframe(self.controllerPlacement)
        cmds.setKeyframe(self.controllerRoot)
        cmds.setKeyframe(self.controlGroup+".follow")


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        mode = kwargs.get("mode", "root")
        startFrame = kwargs.get("startFrame", cmds.playbackOptions(q=True, min=True))
        # print(mode + " from placement.sync")
        if node:
            instance = Placement.load(node)
            rot = 1
            if mode == "root":                
                xyz = cmds.xform(instance.markerRoot, q=True, ws=True, ro=True)
                print("Checking root orientation: ")
                #TODO: option for which rot axis to check
                if abs(xyz[0] + xyz[1] + xyz[2]) > 0:
                    print("root rotations detected! rotations will not be matched!")
                    rot = 0
                cmds.matchTransform(self.controllerPlacement, instance.markerRoot, pos=1, rot=rot, scl=0)
            elif mode == "origin":
               cmds.xform(self.controllerPlacement, ws=1, t=[0,0,0], ro=[0,0,0])
            elif mode == "rootFirstFrame":
                currentTime = cmds.currentTime(q=1)
                cmds.currentTime(startFrame)
                t = cmds.xform(self.controllerPlacement, q=True, ws=True, t=True)
                r = cmds.xform(self.controllerPlacement, q=True, ws=True, ro=True)
                cmds.currentTime(currentTime)
                cmds.xform(self.controllerPlacement, ws=1, t=t, ro=r)
                
            cmds.matchTransform(self.controllerRoot, instance.markerRoot, pos=1, rot=1, scl=0)
