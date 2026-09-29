import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.SpaceSwitch import SpaceSwitch
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Prop"
CLASS_NAME = "Prop"
CONTROLLER_SIZE = [5, 5, 5]


class Prop(ControlRigBase):

    def __init__(self, name="Prop", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = 0
        self.defaultSpace = ""
        #
        self.characterized = False
        self.rigged = False
        #
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        #
        self.boneProp = None
        #
        self.markerProp = None
        #
        self.controllerProp = None


    @staticmethod
    def load(node):
        instance = Prop()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")

        instance.defaultSpace = cmds.getAttr(node+".defaultSpace")
        if not instance.defaultSpace:
            instance.defaultSpace = ""

        spacesString = cmds.getAttr(node+".spaces")
        if not spacesString:
            spacesString = "{}"
        instance.spaces = eval(spacesString)
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneProp = RigNode.getPlug(node, "boneProp")
        instance.markerProp = RigNode.getPlug(node, "markerProp")
        instance.controllerProp = RigNode.getPlug(node, "controllerProp")
        return instance


    def setBoneList(self, boneList):
        self.boneProp = boneList[0]


    def getBoneList(self):
        return [self.boneProp]


    def getBoneNames(self):
        return ["boneProp"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.controllerProp
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerProp"
            ]


    def getMarkerList(self):
        return [self.markerProp]


    def getMarkerNames(self):
        return ["markerProp"]

    #
    # Setup Functions
    #
    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("defaultSpace", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneProp", "message")
        nodeBuilder.addAttr("markerProp", "message")
        nodeBuilder.addAttr("controllerProp", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, **kwargs):
        parentGroup = kwargs.get("p", None)
        ''' Get Required Variables '''
        '''Setup Control Rig Group'''
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # Setup System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        cmds.addAttr(self.systemGroup, ln="ikMode", at="float", min=0, max=1, dv=0, k=1)
        #RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        #Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerProp = Marker.create(n="markerProp", t=self.boneProp, p=self.markerGroup)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneProp", self.boneProp)
        RigNode.setPlug(self.node, "markerProp", self.markerProp)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        RigUtility.modifyTransformChannels(self.controlGroup)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        # Controller
        self.controllerProp = Controller.create(
            name = Controller.buildName(self.instanceName, None, "CTRL"),
            parent = self.controlGroup,
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.WHITE,
            size = CONTROLLER_SIZE)
        cmds.matchTransform(self.controllerProp, self.boneProp)
        RigUtility.bakeOffsetParentMatrix(self.controllerProp)
        # Space Switching
        spaceSwitch = SpaceSwitch2.create(self.controllerProp, None)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "space")
        spaceSwitch.addProxy(self.controlGroup, "space")
        spaceSwitch.addProxy(self.controllerProp, "space")
        cmds.parentConstraint(self.controllerProp, self.boneProp, mo=True)
        # Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        cmds.setAttr(self.node+".defaultSpace", self.defaultSpace, type="string")
        RigNode.setPlug(self.node, "controllerProp", self.controllerProp)
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
        Widgets.WSpaceSwitch2.Create(spaceAttr, label="Prop Space", labelWidth=200, fieldWidth=150, changeCommand=self.setSpace, keyCommand=self.keySpace)
        cmds.setParent('..')


    def setSpace(self, index, *args):
        worldMatrix = cmds.xform(self.controllerProp, q=True, ws=True, m=True)
        cmds.setAttr("{}.space".format(self.controlGroup), index)
        cmds.xform(self.controllerProp, ws=True, m=worldMatrix)


    def keySpace(self, *args):
        cmds.setKeyframe("{}.space".format(self.controlGroup))
        cmds.setKeyframe(self.controllerProp)


    def reset(self, *args):
        cmds.setAttr("{}.space".format(self.controlGroup), 0)
        cmds.setAttr(self.controllerProp+".translateX", 0)
        cmds.setAttr(self.controllerProp+".translateY", 0)
        cmds.setAttr(self.controllerProp+".translateZ", 0)
        cmds.setAttr(self.controllerProp+".rotateX", 0)
        cmds.setAttr(self.controllerProp+".rotateY", 0)
        cmds.setAttr(self.controllerProp+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setKeyframe(self.controllerProp)
        cmds.setKeyframe("{}.space".format(self.controlGroup))


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        marker = RigNode.getPlug(node, "markerProp")
        cmds.matchTransform(self.controllerProp, marker, pos=1, rot=1, scl=0)