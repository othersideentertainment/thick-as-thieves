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
MODULE_PATH = "Otherside.Rigging.ControlRig.GameIK"
CLASS_NAME = "GameIK"
CONTROLLER_SIZE = [25, 25, 25]


class GameIK(ControlRigBase):

    def __init__(self, name="GameIK", **kwargs):
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
        # self.markerGroup = None
        self.rootSpace = None
        #
        # self.boneProp = None
        self.boneIKHandGun = None
        self.boneIKHand_L = None
        self.boneIKHand_R = None
        self.boneIKFoot_L = None
        self.boneIKFoot_R = None
        #
        self.IKHandGunSpace = None
        self.IKHand_LSpace = None
        self.IKHand_RSpace = None
        self.IKFoot_LSpace = None
        self.IKFoot_RSpace = None
        #
        # self.markerProp = None
        #
        self.controllerGameIK = None


    @staticmethod
    def load(node):
        instance = GameIK()
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
        # instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        # instance.boneProp = RigNode.getPlug(node, "boneProp")
        instance.boneIKHandGun = RigNode.getPlug(node, "boneIKHandGun")
        instance.boneIKHand_L = RigNode.getPlug(node, "boneIKHand_L")
        instance.boneIKHand_R = RigNode.getPlug(node, "boneIKHand_R")
        instance.boneIKFoot_L = RigNode.getPlug(node, "boneIKFoot_L")
        instance.boneIKFoot_R = RigNode.getPlug(node, "boneIKFoot_R")
        instance.IKHandGunSpace = RigNode.getPlug(node, "IKHandGunSpace")
        instance.IKHand_LSpace = RigNode.getPlug(node, "IKHand_LSpace")
        instance.IKHand_RSpace = RigNode.getPlug(node, "IKHand_RSpace")
        instance.IKFoot_LSpace = RigNode.getPlug(node, "IKFoot_LSpace")
        instance.IKFoot_RSpace = RigNode.getPlug(node, "IKFoot_RSpace")
        # instance.markerProp = RigNode.getPlug(node, "markerProp")
        instance.controllerGameIK = RigNode.getPlug(node, "controllerGameIK")
        return instance


    def setBoneList(self, boneList):
        self.boneIKHandGun = boneList[0]
        self.boneIKHand_L = boneList[1]
        self.boneIKHand_R = boneList[2]
        self.boneIKFoot_L = boneList[3]
        self.boneIKFoot_R = boneList[4]


    def getBoneList(self):
        return [self.boneIKHandGun, self.boneIKHand_L, self.boneIKHand_R, self.boneIKFoot_L, self.boneIKFoot_R]


    def getBoneNames(self):
        return ["boneIKHandGun", "boneIKHand_L", "boneIKHand_R", "boneIKFoot_L", "boneIKFoot_R"]



    def getKeyable(self):
        return [
            self.controlGroup,
            self.controllerGameIK
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerGameIK"
            ]


    def getMarkerList(self):
        return[]
        # return [self.markerProp]


    def getMarkerNames(self):
        return[]
        # return ["markerProp"]

    #
    # Setup Functions
    #
    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("defaultSpace", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        # nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        # nodeBuilder.addAttr("boneProp", "message")
        nodeBuilder.addAttr("boneIKHandGun", "message")
        nodeBuilder.addAttr("boneIKHand_L", "message")
        nodeBuilder.addAttr("boneIKHand_R", "message")
        nodeBuilder.addAttr("boneIKFoot_L", "message")
        nodeBuilder.addAttr("boneIKFoot_R", "message")
        nodeBuilder.addAttr("IKHandGunSpace", "message")
        nodeBuilder.addAttr("IKHand_LSpace", "message")
        nodeBuilder.addAttr("IKHand_RSpace", "message")
        nodeBuilder.addAttr("IKFoot_LSpace", "message")
        nodeBuilder.addAttr("IKFoot_RSpace", "message")
        # nodeBuilder.addAttr("markerProp", "message")
        nodeBuilder.addAttr("controllerGameIK", "message")
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
        # self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        # self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        # self.markerProp = Marker.create(n="markerProp", t=self.boneProp, p=self.markerGroup)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        # RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        # RigNode.setPlug(self.node, "boneProp", self.boneProp)
        RigNode.setPlug(self.node, "boneIKHandGun", self.boneIKHandGun)
        RigNode.setPlug(self.node, "boneIKHand_L", self.boneIKHand_L)
        RigNode.setPlug(self.node, "boneIKHand_R", self.boneIKHand_R)
        RigNode.setPlug(self.node, "boneIKFoot_L", self.boneIKFoot_L)
        RigNode.setPlug(self.node, "boneIKFoot_R", self.boneIKFoot_R)
        # RigNode.setPlug(self.node, "markerProp", self.markerProp)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        RigUtility.modifyTransformChannels(self.controlGroup)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        # Controller
        self.controllerGameIK = Controller.create(
            name = Controller.buildName(self.instanceName, None, "CTRL"),
            parent = self.controlGroup,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.WHITE,
            size = CONTROLLER_SIZE, 
            normal = [0,1,0])
        # cmds.matchTransform(self.controllerGameIK, self.boneProp)
        RigUtility.bakeOffsetParentMatrix(self.controllerGameIK)
        
        #grab existing bones from other modules
        characterRN = RigNode.getRelated(self.node)
        wrist_l = RigNode.getPlug(RigNode.getPlug(characterRN, "arm_L"), "boneHand")
        wrist_r = RigNode.getPlug(RigNode.getPlug(characterRN, "arm_R"), "boneHand")
        try:
            ankle_l = RigNode.getPlug(RigNode.getPlug(characterRN, "leg_L"), "boneFoot")
            ankle_r = RigNode.getPlug(RigNode.getPlug(characterRN, "leg_R"), "boneFoot")
        except:
            pass
        
        cmds.matchTransform(self.boneIKHandGun, wrist_r)
        cmds.matchTransform(self.boneIKHand_L, wrist_l)
        cmds.matchTransform(self.boneIKHand_R, wrist_r)
        try:
            cmds.matchTransform(self.boneIKFoot_L, ankle_l)
            cmds.matchTransform(self.boneIKFoot_R, ankle_r)
        except:
            pass
        
        RigUtility.bakeOffsetParentMatrix(self.boneIKHandGun)
        RigUtility.bakeOffsetParentMatrix(self.boneIKHand_L)
        RigUtility.bakeOffsetParentMatrix(self.boneIKHand_R)
        try:
            RigUtility.bakeOffsetParentMatrix(self.boneIKFoot_L)
            RigUtility.bakeOffsetParentMatrix(self.boneIKFoot_R)
        except:
            pass
            
        # Space Switch
        # | ik_hand_gun
        spaceSwitch = SpaceSwitch2.create(self.boneIKHandGun, None)
        spaceSwitch.addSpace("local", wrist_r)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "IKHandGunSpace")
        spaceSwitch.addProxy(self.controlGroup, "IKHandGunSpace")
        spaceSwitch.addProxy(self.controllerGameIK, "IKHandGunSpace")
        # | ik_hand_l
        spaceSwitch = SpaceSwitch2.create(self.boneIKHand_L, None)
        spaceSwitch.addSpace("local", wrist_l)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "IKHand_LSpace")
        spaceSwitch.addProxy(self.controlGroup, "IKHand_LSpace")
        spaceSwitch.addProxy(self.controllerGameIK, "IKHand_LSpace")
        # | ik_hand_r
        spaceSwitch = SpaceSwitch2.create(self.boneIKHand_R, None)
        spaceSwitch.addSpace("local", wrist_r)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "IKHand_RSpace")
        spaceSwitch.addProxy(self.controlGroup, "IKHand_RSpace")
        spaceSwitch.addProxy(self.controllerGameIK, "IKHand_RSpace")
        try:
            # | ik_foot_l
            spaceSwitch = SpaceSwitch2.create(self.boneIKFoot_L, None)
            spaceSwitch.addSpace("local", ankle_l)
            spaceSwitch.addSpace("world", None)
            spaceSwitch.build(parent=self.systemGroup)
            spaceSwitch.addProxy(self.systemGroup, "IKFoot_LSpace")
            spaceSwitch.addProxy(self.controlGroup, "IKFoot_LSpace")
            spaceSwitch.addProxy(self.controllerGameIK, "IKFoot_LSpace")
            # | ik_foot_r
            spaceSwitch = SpaceSwitch2.create(self.boneIKFoot_R, None)
            spaceSwitch.addSpace("local", ankle_r)
            spaceSwitch.addSpace("world", None)
            spaceSwitch.build(parent=self.systemGroup)
            spaceSwitch.addProxy(self.systemGroup, "IKFoot_RSpace")
            spaceSwitch.addProxy(self.controlGroup, "IKFoot_RSpace")
            spaceSwitch.addProxy(self.controllerGameIK, "IKFoot_RSpace")
        except:
            pass
        
        # Space Switching
        # spaceSwitch = SpaceSwitch2.create(self.controllerGameIK, None)
        # spaceSwitch.addSpaceMap(self.spaces)
        # spaceSwitch.addSpace("world", self.rootSpace)
        # spaceSwitch.build(parent=self.systemGroup)
        # spaceSwitch.addProxy(self.systemGroup, "space")
        # spaceSwitch.addProxy(self.controlGroup, "space")
        # spaceSwitch.addProxy(self.controllerGameIK, "space")
        # cmds.parentConstraint(self.controllerGameIK, self.boneProp, mo=True)
        # Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        # RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        # cmds.setAttr(self.markerGroup+".v", 0)
        # cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        cmds.setAttr(self.node+".defaultSpace", self.defaultSpace, type="string")
        RigNode.setPlug(self.node, "IKHandGunSpace", self.IKHandGunSpace)
        RigNode.setPlug(self.node, "IKHand_LSpace", self.IKHand_LSpace)
        RigNode.setPlug(self.node, "IKHand_RSpace", self.IKHand_RSpace)
        try:
            RigNode.setPlug(self.node, "IKFoot_LSpace", self.IKFoot_LSpace)
            RigNode.setPlug(self.node, "IKFoot_RSpace", self.IKFoot_RSpace)
        except:
            pass
        RigNode.setPlug(self.node, "controllerGameIK", self.controllerGameIK)
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
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".IKHandGunSpace", label="IKHandGunSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setIKHandGunSpace, keyCommand=self.keyIKHandGunSpace)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".IKHand_LSpace", label="IKHand_LSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setIKHand_LSpace, keyCommand=self.keyIKHand_LSpace)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".IKHand_RSpace", label="IKHand_RSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setIKHand_RSpace, keyCommand=self.keyIKHand_RSpace)
        try:
            Widgets.WSpaceSwitch2.Create(self.controlGroup+".IKFoot_LSpace", label="IKFoot_LSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setIKFoot_LSpace, keyCommand=self.keyIKFoot_LSpace)
            Widgets.WSpaceSwitch2.Create(self.controlGroup+".IKFoot_RSpace", label="IKFoot_RSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setIKFoot_RSpace, keyCommand=self.keyIKFoot_RSpace)
        except:
            pass
        # spaceAttr = "{}.space".format(self.controlGroup)
        # Widgets.WSpaceSwitch2.Create(spaceAttr, label="GameIK Space", labelWidth=200, fieldWidth=150, changeCommand=self.setSpace, keyCommand=self.keySpace)
        cmds.setParent('..')


    # def setSpace(self, index, *args):
        # worldMatrix = cmds.xform(self.controllerGameIK, q=True, ws=True, m=True)
        # cmds.setAttr(f"{self.controlGroup}.{args[0]}", index)
        # cmds.xform(self.controllerGameIK, ws=True, m=worldMatrix)
        
        
    def setIKHandGunSpace(self, index, *args):
        cmds.setAttr(f"{self.controlGroup}.IKHandGunSpace", index)
        
    def setIKHand_LSpace(self, index, *args):
        cmds.setAttr(f"{self.controlGroup}.IKHand_LSpace", index)

    def setIKHand_RSpace(self, index, *args):
        cmds.setAttr(f"{self.controlGroup}.IKHand_RSpace", index)

    def setIKFoot_LSpace(self, index, *args):
        cmds.setAttr(f"{self.controlGroup}.IKFoot_LSpace", index)

    def setIKFoot_RSpace(self, index, *args):
        cmds.setAttr(f"{self.controlGroup}.IKFoot_RSpace", index)       


    # def keySpace(self, *args):
        # cmds.setKeyframe(f"{self.controlGroup}.{args[0]}")
        # cmds.setKeyframe(self.controllerGameIK)
        
    def keyIKHandGunSpace(self, *args):
        cmds.setKeyframe(f"{self.controlGroup}.IKHandGunSpace")
    def keyIKHand_LSpace(self, *args):
        cmds.setKeyframe(f"{self.controlGroup}.IKHand_LSpace") 
    def keyIKHand_RSpace(self, *args):
        cmds.setKeyframe(f"{self.controlGroup}.IKHand_RSpace") 
    def keyIKFoot_LSpace(self, *args):
        cmds.setKeyframe(f"{self.controlGroup}.IKFoot_LSpace") 
    def keyIKFoot_RSpace(self, *args):
        cmds.setKeyframe(f"{self.controlGroup}.IKFoot_RSpace")         


    def reset(self, *args):
        cmds.setAttr("{}.IKHandGunSpace".format(self.controlGroup), 0)
        cmds.setAttr("{}.IKHand_LSpace".format(self.controlGroup), 0)
        cmds.setAttr("{}.IKHand_RSpace".format(self.controlGroup), 0)
        try:
            cmds.setAttr("{}.IKFoot_LSpace".format(self.controlGroup), 0)
            cmds.setAttr("{}.IKFoot_RSpace".format(self.controlGroup), 0)
        except:
            pass
            
        cmds.setAttr(self.controllerGameIK+".translateX", 0)
        cmds.setAttr(self.controllerGameIK+".translateY", 0)
        cmds.setAttr(self.controllerGameIK+".translateZ", 0)
        cmds.setAttr(self.controllerGameIK+".rotateX", 0)
        cmds.setAttr(self.controllerGameIK+".rotateY", 0)
        cmds.setAttr(self.controllerGameIK+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setKeyframe(self.controllerGameIK)
        cmds.setKeyframe("{}.IKHandGunSpace".format(self.controlGroup))
        cmds.setKeyframe("{}.IKHand_LSpace".format(self.controlGroup))
        cmds.setKeyframe("{}.IKHand_RSpace".format(self.controlGroup))
        try:
            cmds.setKeyframe("{}.IKFoot_LSpace".format(self.controlGroup))
            cmds.setKeyframe("{}.IKFoot_RSpace".format(self.controlGroup))
        except:
            pass


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        # marker = RigNode.getPlug(node, "markerProp")
        # cmds.matchTransform(self.controllerGameIK, marker, pos=1, rot=1, scl=0)