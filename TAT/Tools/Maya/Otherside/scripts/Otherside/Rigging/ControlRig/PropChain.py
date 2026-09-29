import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase
import maya.api.OpenMaya as OpenMaya
from functools import partial

#Constants
COLOR_ACTIVE = (.9,.9,.9)
COLOR_INACTIVE = (.5,.5,.5)
COLOR_KEY = (.9,.6,.6)
SEPARATOR_SPACE = 3

RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.PropChain"
CLASS_NAME = "PropChain"
CONTROLLER_SIZE = [3,3,3]


class PropChain(ControlRigBase):
    def __init__(self, name="propChain", *args, **kwargs):
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
        self.boneStart = None
        self.boneEnd = None
        #
        self.boneChain = []
        self.markerChain = []
        self.controllerChain = []


    @staticmethod
    def load(node):
        instance = PropChain()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        #
        spacesString = cmds.getAttr(node+".spaces")
        if not spacesString:
            spacesString = "{}"
        instance.spaces = eval(spacesString)
        #
        instance.side = cmds.getAttr(node+".side")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        #Bone
        instance.boneStart = RigNode.getPlug(node, "boneStart")
        instance.boneEnd = RigNode.getPlug(node, "boneEnd")
        #Chains
        instance.boneChain = RigNode.getMultiPlug(node, "boneChain")
        instance.markerChain = RigNode.getMultiPlug(node, "markerChain")
        instance.controllerChain = RigNode.getMultiPlug(node, "controllerChain")
        #
        return instance


    def getBoneList(self, *args, **kwargs):
        '''
        boneList = []
        boneList.append(self.boneStart)
        for bone in self.boneChain:
            boneList.append(bone)
        boneList.append(self.boneEnd)
        return boneList
        '''
        return [self.boneStart, self.boneEnd]


    def getBoneNames(self, *args, **kwargs):
        return ["boneStart", "boneEnd"]


    def getKeyable(self):
        output = [self.controlGroup]
        for ctrl in self.controllerChain:
            output.append(ctrl)
        return output


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerChain"
            ]

    def setBoneList(self, boneList, *args, **kwargs):
        self.boneStart = boneList[0]
        self.boneEnd = boneList[1]


    def getMarkerList(self, *args, **kwargs):
        return self.markerChain


    def createNode(self, *args, **kwargs):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneStart", "message")
        nodeBuilder.addAttr("boneEnd", "message")
        nodeBuilder.addAttr("boneChain", "messageMulti")
        nodeBuilder.addAttr("markerChain", "messageMulti")
        nodeBuilder.addAttr("controllerChain", "messageMulti")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, *args, **kwargs):
        parentGroup = kwargs.get("p", None)
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
        #Bone Chain
        self.boneChain = RigUtility.getBoneChain(self.boneStart, self.boneEnd)
        #Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerChain = []
        for i in range(0,len(self.boneChain)):
            marker = Marker.create(n="marker_"+self.instanceName+str(i), t=self.boneChain[i], p=self.markerGroup)
            self.markerChain.append(marker)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneStart", self.boneStart)
        RigNode.setPlug(self.node, "boneEnd", self.boneEnd)
        #Connect Bone Chain
        for i in range(0,len(self.boneChain)):
            RigNode.setPlug(self.node, "boneChain["+str(i)+"]", self.boneChain[i])
        #
        for i in range(0,len(self.markerChain)):
            RigNode.setPlug(self.node, "markerChain["+str(i)+"]", self.markerChain[i])


    def rig(self, *args, **kwargs):
        RigUtility.modifyTransformChannels(self.controlGroup)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        # Create Controller Chain
        self.controllerChain = []
        parentTransform = self.controlGroup
        for i in range(0,len(self.markerChain)):
            #Create Controller
            control = Controller.create(
                name = Controller.buildName(self.instanceName+"_"+str(i+1), None, "FK"),
                parent = parentTransform,
                shape = Controller.Shape.CIRCLE,
                color = Controller.Color.CYAN,
                normal = (1,0,0),
                size = CONTROLLER_SIZE)
            control = cmds.ls(control, l=True)[-1]
            cmds.matchTransform(control, self.markerChain[i])
            RigUtility.bakeOffsetParentMatrix(control)
            # Setup Space Switch
            boneParent = RigUtility.firstParentOf(self.boneChain[i])
            spaceSwitch = SpaceSwitch2.create(control, boneParent)
            spaceSwitch.addSpace("parent", boneParent)
            spaceSwitch.addSpace("world", None)
            spaceSwitch.build(parent=self.systemGroup)
            spaceSwitch.addProxy(control, "space")
            spaceSwitch.addProxy(self.controlGroup, "controller"+str(i)+"Space")
            spaceSwitch.addProxy(self.systemGroup, "controller"+str(i)+"Space")
            #Create Constraint
            cmds.parentConstraint(control, self.boneChain[i])
            # Store value and prep next loop
            self.controllerChain.append(control)
            parentTransform = control
        # Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode - Controller List String
        for i in range(0,len(self.controllerChain)):
            RigNode.setPlug(self.node, "controllerChain["+str(i)+"]", self.controllerChain[i])
        for controller in self.controllerChain:
            RigUtility.modifyTransformChannels(controller, r=False)
        # Set Rigged Status
        self.rigged = True
        cmds.setAttr("{}.rigged".format(self.node), True)


    def gui(self, *args, **kwargs):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        cmds.button(l="Select Below", c=self.selectBelow, width=350)
        cmds.separator( height=15)
        for controller in self.controllerChain:
            controllerName = RigUtility.shortNameOf(controller)
            spaceAttr = "{}.space".format(controller)
            Widgets.SpaceSwitch.Create(
                spaceAttr,
                [controller],
                label=controllerName,
                labelWidth=200,
                fieldWidth=150)
        cmds.setParent('..')


    def reset(self, *args, **kwargs):
        for control in self.controllerChain:
            cmds.setAttr("{}.rotate".format(control), 0, 0, 0)
            cmds.setAttr("{}.space".format(control), 0)


    def keyAll(self, *args, **kwargs):
        for control in self.controllerChain:
            cmds.setKeyframe(control)


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        instance = PropChain.load(node)
        for i in range(0,len(instance.markerChain)):
            if i < len(self.controllerChain):
                cmds.matchTransform(instance.markerChain[i], self.controllerChain[i], pos=0, rot=1, scl=0)


    def selectBelow(self, *args, **kwargs):
        selected = cmds.ls(sl=True, fl=True, l=True)
        if selected:
            selected = selected[-1]
            if selected in self.controllerChain:
                index = self.controllerChain.index(selected)
                for i in range(index, len(self.controllerChain)):
                    cmds.select(self.controllerChain[i], add=True)
