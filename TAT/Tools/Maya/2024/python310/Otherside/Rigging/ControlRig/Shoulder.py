from collections import OrderedDict
import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
from Otherside.Rigging.SpaceSwitch import SpaceSwitch
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase
import json


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Shoulder"
CLASS_NAME = "Shoulder"
CONTROLLER_SIZE = [5, 5, 5]


class Shoulder(ControlRigBase):

    def __init__(self, name="Shoulder", **kwargs):
        self.node = None
        self.instanceName = name
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
        self.boneShoulder = None
        #
        self.markerShoulder = None
        #
        self.fkShoulder = None


    @staticmethod
    def load(node):
        instance = Shoulder()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.spaces = {}
        instance.side = cmds.getAttr(node+".side")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneShoulder = RigNode.getPlug(node, "boneShoulder")
        instance.markerShoulder = RigNode.getPlug(node, "markerShoulder")
        instance.fkShoulder = RigNode.getPlug(node, "fkShoulder")
        #SpaceMap
        spaceString = cmds.getAttr(f"{node}.spaces")
        if not spaceString:
            spaceString = "{}"
        #dump it first to deal with old single quotes formatting
        spaceString = json.dumps(spaceString)
        instance.spaces = json.loads(spaceString, object_pairs_hook=OrderedDict)
        #
        return instance


    def setBoneList(self, boneList):
        self.boneShoulder = boneList[0]


    def getBoneList(self):
        return [self.boneShoulder]


    def getBoneNames(self):
        return ["boneShoulder"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkShoulder
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkShoulder"
            ]


    def getMarkerList(self):
        return [self.markerShoulder]


    def getMarkerNames(self):
        return ["markerShoulder"]

    #
    # Setup Functions
    #

    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("boneShoulder", "message")
        nodeBuilder.addAttr("markerShoulder", "message")
        nodeBuilder.addAttr("fkShoulder", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)


    def characterize(self, **kwargs):
        #get args
        parentGroup = kwargs.get("p", None)
        '''Setup Control Rig Group'''
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # Setup System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        #RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        #Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerShoulder = Marker.create(n="markerShoulder", t=self.boneShoulder, p=self.markerGroup)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneShoulder", self.boneShoulder)
        RigNode.setPlug(self.node, "markerShoulder", self.markerShoulder)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        #'''Shoulder'''
        self.fkShoulder = Controller.create(
            name = Controller.buildName("shoulder", self.side, "FK"),
            parent = self.controlGroup,
            shape = Controller.Shape.BOX,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        cmds.matchTransform(self.fkShoulder, self.markerShoulder)
        RigUtility.bakeOffsetParentMatrix(self.fkShoulder)
        cmds.parentConstraint(self.fkShoulder, self.boneShoulder, mo=True)
        #Space Switch
        chestBone = RigUtility.firstParentOf(self.boneShoulder)
        spaceSwitch = SpaceSwitch2.create(self.fkShoulder, chestBone)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "space")
        spaceSwitch.addProxy(self.controlGroup, "space")
        #Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.fkShoulder, r=False)
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        #Connect to Node
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkShoulder", self.fkShoulder)
        # Set Rigged Status
        self.rigged = True


    #
    # Animation Control Functions
    #
    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        spaceAttr = "{}.space".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(spaceAttr, label="Shoulder Space", labelWidth=200, fieldWidth=150, changeCommand=self.setShoulderSpace, keyCommand=self.keyShoulderSpace)
        cmds.setParent('..')


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        marker = None
        if not node:
            node = self.node
        markerShoulder = RigNode.getPlug(node, "markerShoulder")
        cmds.matchTransform(self.fkShoulder, markerShoulder, pos=0, rot=1, scl=0)


    def setShoulderSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkShoulder, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".space", index)
        cmds.xform(self.fkShoulder, ws=True, m=worldMatrix)


    def keyShoulderSpace(self, *args):
        cmds.setKeyframe(self.controlGroup+".space")
        cmds.setKeyframe(self.fkShoulder)


    def reset(self, *args):
        cmds.setAttr(self.controlGroup+".space", 0)
        cmds.setAttr(self.fkShoulder+".translateX", 0)
        cmds.setAttr(self.fkShoulder+".translateY", 0)
        cmds.setAttr(self.fkShoulder+".translateZ", 0)
        cmds.setAttr(self.fkShoulder+".rotateX", 0)
        cmds.setAttr(self.fkShoulder+".rotateY", 0)
        cmds.setAttr(self.fkShoulder+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setKeyframe(self.fkShoulder)
        cmds.setKeyframe(self.controlGroup+".space")