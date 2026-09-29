import json 
from collections import OrderedDict
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
MODULE_PATH = "Otherside.Rigging.ControlRig.PropTwoHand"
CLASS_NAME = "PropTwoHand"
CONTROLLER_SIZE = [5, 5, 5]


class PropTwoHand(ControlRigBase):

    def __init__(self, name="PropTwoHand", **kwargs):
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
        # self.bonePropFollow = None
        #
        self.markerProp = None
        #
        self.controllerProp = None


    @staticmethod
    def load(node):
        instance = PropTwoHand()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")

        instance.defaultSpace = cmds.getAttr(node+".defaultSpace")
        if not instance.defaultSpace:
            instance.defaultSpace = ""
        
        #SpaceMap
        spaceString = cmds.getAttr(f"{node}.spaces")
        if not spaceString:
            spaceString = "{}"
        #dump it first to deal with old single quotes formatting
        spaceString = json.dumps(spaceString)
        instance.spaces = json.loads(spaceString, object_pairs_hook=OrderedDict)
        
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneProp = RigNode.getPlug(node, "boneProp")
        # instance.bonePropFollow = RigNode.getPlug(node, "bonePropFollow")
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
        # nodeBuilder.addAttr("bonePropFollow", "message")
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
        # RigNode.setPlug(self.node, "bonePropFollow", self.bonePropFollow)
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
            shape = Controller.Shape.BOX,
            color = Controller.Color.WHITE,
            size = [5, 5, 50])
        # move controller cvs so pivot is at the edge
        shape = cmds.listRelatives(self.controllerProp, shapes=1, noIntermediate=1)[0]
        cmds.move(0,0,25, shape+'.cv[*]', r=1, os=1, wd=1)

        #create alt control shape
        altCurves = Controller.create(
            name = Controller.buildName(self.instanceName, None, "CTRL"),
            parent = self.controlGroup,
            shape = Controller.Shape.BOX,
            color = Controller.Color.WHITE,
            size = [50, 5, 5])
        # move controller cvs so pivot is at the edge
        newShape = cmds.listRelatives(altCurves, shapes=1, noIntermediate=1)[0]
        cmds.move(25, 0, 0, newShape+'.cv[*]', r=1, os=1, wd=1)
        #create attr to control controller shape vis
        cmds.parent(newShape, self.controllerProp, r=1, s=1)
        cmds.addAttr(self.controllerProp, ln='upInsteadOfForward', at='long', min=0, max=1, dv=0)
        cmds.setAttr(self.controllerProp + '.upInsteadOfForward', e=1, k=1)
        rev = cmds.createNode('reverse')
        cmds.connectAttr(self.controllerProp + '.upInsteadOfForward', newShape+'.visibility')
        cmds.connectAttr(self.controllerProp + '.upInsteadOfForward', rev+'.inputX')
        cmds.connectAttr(rev + '.outputX', shape + '.visibility')
        cmds.delete(altCurves)

        # # |- setup follow bone
        # self.bonePropFollow = cmds.createNode(
        #     "joint",
        #     p=self.systemGroup,
        #     n=RigUtility.shortNameOf(self.boneProp) + "_Follow")
        # self.bonePropFollow = cmds.ls(sl=1, long=1)[0]
        # RigNode.setPlug(self.node, "bonePropFollow", self.bonePropFollow)

        cmds.matchTransform(self.controllerProp, self.boneProp)
        RigUtility.bakeOffsetParentMatrix(self.controllerProp)

        # cmds.parentConstraint(self.controllerProp, self.bonePropFollow, mo=False)

        # Space Switching
        spaceSwitch = SpaceSwitch2.create(self.controllerProp, None)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "space")
        spaceSwitch.addProxy(self.controlGroup, "space")
        spaceSwitch.addProxy(self.controllerProp, "space")
        # cmds.parentConstraint(self.controllerProp, self.bonePropFollow, mo=True)
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

        #update characterRN
        characterRN = RigNode.getRelated(self.node)
        spacemapString = cmds.getAttr(characterRN + ".spacemap")
        if not spacemapString:
            spacemapString = "[]"
        spacemap = eval(spacemapString)
        spacemap["propTwoHand"] = self.controllerProp
        #cmds.setAttr(characterRN + ".spacemap", str(spacemap), type="string")
        cmds.setAttr(characterRN+".spacemap", json.dumps(spacemap), type="string")

        # tried to push the update to arm spaces here but the UI was overwriting this.
        # now the arm module takes care of pulling the info it needs if propTwoHand exists.

        # #update arm_l module
        # arm_l = RigNode.getPlug(characterRN, "arm_L")
        # spacesString = cmds.getAttr(arm_l + ".spaces")
        # if not spacesString:
        #     spacesString = "{}"
        # spaces = eval(spacesString)
        # for i,space in enumerate(spaces):
        #     if 'propTwoHand' in space[0]:
        #         spaces[i] = ('propTwoHand', self.bonePropFollow)
        # cmds.setAttr(arm_l + ".spaces", str(spaces), type="string")
        #
        # #update arm_r module
        # arm_r = RigNode.getPlug(characterRN, "arm_R")
        # spacesString = cmds.getAttr(arm_r + ".spaces")
        # if not spacesString:
        #     spacesString = "{}"
        # spaces = eval(spacesString)
        # for i, space in enumerate(spaces):
        #     if 'propTwoHand' in space[0]:
        #         spaces[i] = ('propTwoHand', self.bonePropFollow)
        # cmds.setAttr(arm_r + ".spaces", str(spaces), type="string")

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