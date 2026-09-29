import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase

RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Head"
CLASS_NAME = "Head"
CONTROLLER_SIZE = [5, 5, 5]


class Head(ControlRigBase):
    def __init__(self, name="Head", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = 0
        #
        self.characterized = False
        self.rigged = False
        #
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        #Bones
        self.boneNeck = None
        self.boneHead = None
        #Markers
        self.markerNeck = None
        self.markerHead = None
        #Controls
        self.fkNeck = None
        self.fkHead = None


    @staticmethod
    def load(node):
        instance = Head()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneNeck = RigNode.getPlug(node, "boneNeck")
        instance.boneHead = RigNode.getPlug(node, "boneHead")
        instance.markerNeck = RigNode.getPlug(node, "markerNeck")
        instance.markerHead = RigNode.getPlug(node, "markerHead")
        instance.fkNeck = RigNode.getPlug(node, "fkNeck")
        instance.fkHead = RigNode.getPlug(node, "fkHead")
        #SpaceMap
        spaceString = cmds.getAttr("{}.spaces".format(node))
        if not spaceString:
            spaceString = "{}"
        instance.spaces = eval(spaceString)
        #
        return instance


    def setBoneList(self, boneList):
        self.boneNeck = boneList[0]
        self.boneHead = boneList[1]


    def getBoneList(self):
        return [self.boneNeck, self.boneHead]


    def getBoneNames(self):
        return ["boneNeck", "boneHead"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkNeck,
            self.fkHead
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkNeck",
            "fkHead"
            ]


    def getMarkerList(self):
        return [self.markerNeck, self.markerHead]


    def getMarkerNames(self):
        return ["markerNeck", self.markerHead]


    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneHead", "message")
        nodeBuilder.addAttr("boneNeck", "message")
        nodeBuilder.addAttr("markerNeck", "message")
        nodeBuilder.addAttr("markerHead", "message")
        nodeBuilder.addAttr("fkNeck", "message")
        nodeBuilder.addAttr("fkHead", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, **kwargs):
        #get args
        parentGroup = kwargs.get("p", None)
        '''Setup Control Rig Group'''
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
            self.controlGroup = cmds.ls(self.controlGroup, l=True)[-1]
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
        self.markerNeck = Marker.create(n="markerNeck", t=self.boneNeck, p=self.markerGroup)
        self.markerHead = Marker.create(n="markerHead", t=self.boneHead, p=self.markerGroup)
        aimAxis = [1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        endPoint = cmds.xform(self.boneHead, q=True, ws=True, rp=True)
        endPoint = [endPoint[0], endPoint[1]+1, endPoint[2]]
        Marker.orient(self.markerNeck, target=self.markerHead, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerHead, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneNeck", self.boneNeck)
        RigNode.setPlug(self.node, "boneHead", self.boneHead)
        RigNode.setPlug(self.node, "markerNeck", self.markerNeck)
        RigNode.setPlug(self.node, "markerHead", self.markerHead)


    def rig(self, **kwargs):
        boneChest = RigUtility.firstParentOf(self.boneNeck)
        #Create Controllers
        # | Neck Controller
        self.fkNeck = Controller.create(
            name = Controller.buildName("neck", None, "FK"),
            parent = self.controlGroup,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        cmds.matchTransform(self.fkNeck, self.markerNeck)
        RigUtility.bakeOffsetParentMatrix(self.fkNeck)
        cmds.parentConstraint(self.fkNeck, self.boneNeck, mo=True)
        # | Neck Space Switch
        spaceSwitch = SpaceSwitch2.create(self.fkNeck, boneChest)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "neckSpace")
        spaceSwitch.addProxy(self.controlGroup, "neckSpace")
        # | Head Controller
        self.fkHead = Controller.create(
            name = Controller.buildName("head", None, "FK"),
            parent = self.fkNeck,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        cmds.matchTransform(self.fkHead, self.markerHead)
        RigUtility.bakeOffsetParentMatrix(self.fkHead)
        cmds.parentConstraint(self.fkHead, self.boneHead, mo=True)
        # | Head Space Switch
        spaceSwitch = SpaceSwitch2.create(self.fkHead, self.boneNeck)
        spaceSwitch.addSpace("neck", self.boneNeck, True)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "headSpace")
        spaceSwitch.addProxy(self.controlGroup, "headSpace")
        # Set RigNode Plugs
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkNeck", self.fkNeck)
        RigNode.setPlug(self.node, "fkHead", self.fkHead)
        # Lock unused channels
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        RigUtility.modifyTransformChannels(self.fkHead, r=False)
        RigUtility.modifyTransformChannels(self.fkNeck, r=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set Rigged Status
        self.rigged = True


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        headSpace = "{}.headSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(headSpace, label="Head Space", labelWidth=200, fieldWidth=150, changeCommand=self.setHeadSpace, keyCommand=self.keyHeadSpace)
        neckSpace = "{}.neckSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(neckSpace, label="Neck Space", labelWidth=200, fieldWidth=150, changeCommand=self.setNeckSpace, keyCommand=self.keyNeckSpace)
        cmds.setParent('..')


    def setHeadSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkHead, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".headSpace", index)
        cmds.xform(self.fkHead, ws=True, m=worldMatrix)


    def setNeckSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkNeck, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".neckSpace", index)
        cmds.xform(self.fkNeck, ws=True, m=worldMatrix)


    def keyHeadSpace(self, *args):
        cmds.setKeyframe(self.controlGroup+".headSpace")
        cmds.setKeyframe(self.fkHead)


    def keyNeckSpace(self, *args):
        cmds.setKeyframe(self.controlGroup+".neckspace")
        cmds.setKeyframe(self.fkNeck)


    def reset(self, *args):
        cmds.setAttr(self.controlGroup+".headSpace", 0)
        cmds.setAttr(self.controlGroup+".neckSpace", 0)
        controlList = [self.fkNeck, self.fkHead]
        for control in controlList:
            #cmds.setAttr(control+".translateX", 0)
            #cmds.setAttr(control+".translateY", 0)
            #cmds.setAttr(control+".translateZ", 0)
            cmds.setAttr(control+".rotateX", 0)
            cmds.setAttr(control+".rotateY", 0)
            cmds.setAttr(control+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setKeyframe(self.fkNeck)
        cmds.setKeyframe(self.fkHead)
        attrList = ["headSpace", "neckSpace"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        marker = None
        if not node:
            node = self.node
        markerNeck = RigNode.getPlug(node, "markerNeck")
        markerHead = RigNode.getPlug(node, "markerHead")
        cmds.matchTransform(self.fkNeck, markerNeck, pos=0, rot=1, scl=0)
        cmds.matchTransform(self.fkHead, markerHead, pos=0, rot=1, scl=0)