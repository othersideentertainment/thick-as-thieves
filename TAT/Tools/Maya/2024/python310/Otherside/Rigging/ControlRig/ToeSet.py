from collections import OrderedDict
import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.ToeSet"
CLASS_NAME = "ToeSet"
CONTROLLER_SIZE = [5, 5, 5]


class ToeSet(ControlRigBase):

    def __init__(self, name="ToeSet", **kwargs):
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
        self.boneToeA = None
        self.boneToeB = None
        self.boneToeC = None
        self.boneToeD = None
        self.boneToeE = None
        #
        self.markerToeA = None
        self.markerToeB = None
        self.markerToeC = None
        self.markerToeD = None
        self.markerToeE = None
        #
        self.fkToeA = None
        self.fkToeB = None
        self.fkToeC = None
        self.fkToeD = None
        self.fkToeE = None


    @staticmethod
    def load(node):
        instance = ToeSet()
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
        #
        instance.boneToeA = RigNode.getPlug(node, "boneToeA")
        instance.boneToeB = RigNode.getPlug(node, "boneToeB")
        instance.boneToeC = RigNode.getPlug(node, "boneToeC")
        instance.boneToeD = RigNode.getPlug(node, "boneToeD")
        instance.boneToeE = RigNode.getPlug(node, "boneToeE")
        #
        instance.markerToeA = RigNode.getPlug(node, "markerToeA")
        instance.markerToeB = RigNode.getPlug(node, "markerToeB")
        instance.markerToeC = RigNode.getPlug(node, "markerToeC")
        instance.markerToeD = RigNode.getPlug(node, "markerToeD")
        instance.markerToeE = RigNode.getPlug(node, "markerToeE")
        #
        instance.fkToeA = RigNode.getPlug(node, "fkToeA")
        instance.fkToeB = RigNode.getPlug(node, "fkToeB")
        instance.fkToeC = RigNode.getPlug(node, "fkToeC")
        instance.fkToeD = RigNode.getPlug(node, "fkToeD")
        instance.fkToeE = RigNode.getPlug(node, "fkToeE")
        return instance


    def setBoneList(self, boneList):
        self.boneToeA = boneList[0]
        self.boneToeB = boneList[1]
        self.boneToeC = boneList[2]
        self.boneToeD = boneList[3]
        self.boneToeE = boneList[4]


    def getBoneList(self):
        return [self.boneToeA, self.boneToeB, self.boneToeC, self.boneToeD, self.boneToeE]


    def getBoneNames(self):
        return ["boneToeA", "boneToeB", "boneToeC", "boneToeD", "boneToeE"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkToeA,
            self.fkToeB,
            self.fkToeC,
            self.fkToeD,
            self.fkToeE
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkToeA",
            "fkToeB",
            "fkToeC",
            "fkToeD",
            "fkToeE",
            ]


    def getMarkerList(self):
        return [self.markerToeA, self.markerToeB, self.markerToeC, self.markerToeD, self.markerToeE]


    def getMarkerNames(self):
        return ["markerToeA", "markerToeB", "markerToeC", "markerToeD", "markerToeE"]

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
        nodeBuilder.addAttr("boneToeA", "message")
        nodeBuilder.addAttr("boneToeB", "message")
        nodeBuilder.addAttr("boneToeC", "message")
        nodeBuilder.addAttr("boneToeD", "message")
        nodeBuilder.addAttr("boneToeE", "message")
        nodeBuilder.addAttr("markerToeA", "message")
        nodeBuilder.addAttr("markerToeB", "message")
        nodeBuilder.addAttr("markerToeC", "message")
        nodeBuilder.addAttr("markerToeD", "message")
        nodeBuilder.addAttr("markerToeE", "message")
        nodeBuilder.addAttr("fkToeA", "message")
        nodeBuilder.addAttr("fkToeB", "message")
        nodeBuilder.addAttr("fkToeC", "message")
        nodeBuilder.addAttr("fkToeD", "message")
        nodeBuilder.addAttr("fkToeE", "message")
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
        #Create Marker Group
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        #Auto Orient Markers
        aimAxis = [-1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        if self.side == 2:
            aimAxis = [ aimAxis[0] * -1, aimAxis[1] * -1, aimAxis[2] * -1]
        #Create Markers
        boneList = self.getBoneList()
        markerNames = self.getMarkerNames()
        markerList = [None, None, None, None, None]
        for i in range(0,len(boneList)):
            if boneList[i] == None:
                continue
            markerList[i] = Marker.create(n=markerNames[i], t=boneList[i], p=self.markerGroup)
            endPoint = cmds.xform(markerList[i], q=True, ws=True, rp=True)
            endPoint = [endPoint[0], endPoint[1], endPoint[2]+1]
            Marker.orient(markerList[i], targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        #assign valid markers
        self.markerToeA = markerList[0]
        self.markerToeB = markerList[1]
        self.markerToeC = markerList[2]
        self.markerToeD = markerList[3]
        self.markerToeE = markerList[4]
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneToeA", self.boneToeA)
        RigNode.setPlug(self.node, "boneToeB", self.boneToeB)
        RigNode.setPlug(self.node, "boneToeC", self.boneToeC)
        RigNode.setPlug(self.node, "boneToeD", self.boneToeD)
        RigNode.setPlug(self.node, "boneToeE", self.boneToeE)
        RigNode.setPlug(self.node, "markerToeA", self.markerToeA)
        RigNode.setPlug(self.node, "markerToeB", self.markerToeB)
        RigNode.setPlug(self.node, "markerToeC", self.markerToeC)
        RigNode.setPlug(self.node, "markerToeD", self.markerToeD)
        RigNode.setPlug(self.node, "markerToeE", self.markerToeE)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        RigUtility.modifyTransformChannels(self.controlGroup)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        #
        boneList = self.getBoneList()
        markerList = self.getMarkerList()
        prefixList = ["toe_a", "toe_b", "toe_c", "toe_d", "toe_e"]
        controlList = [None, None, None, None, None]
        #
        for i in range(0,len(markerList)):
            bone = boneList[i]
            marker = markerList[i]
            prefix = prefixList[i]
            if marker != None:
                boneParent = RigUtility.firstParentOf(bone)
                controlList[i] = Controller.create(
                    name = Controller.buildName(prefix, self.side, "fk"),
                    parent = self.controlGroup,
                    shape = Controller.Shape.CIRCLE,
                    color = Controller.Color.CYAN,
                    size = CONTROLLER_SIZE)
                cmds.matchTransform(controlList[i], marker)
                RigUtility.bakeOffsetParentMatrix(controlList[i])
                RigUtility.parentByMatrix(controlList[i], boneParent, mo=True)
                cmds.parentConstraint(controlList[i], bone, mo=True)
        #
        self.fkToeA = controlList[0]
        self.fkToeB = controlList[1]
        self.fkToeC = controlList[2]
        self.fkToeD = controlList[3]
        self.fkToeE = controlList[4]
        # Set RigNode
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        cmds.setAttr(self.node+".defaultSpace", self.defaultSpace, type="string")
        RigNode.setPlug(self.node, "fkToeA", self.fkToeA)
        RigNode.setPlug(self.node, "fkToeB", self.fkToeB)
        RigNode.setPlug(self.node, "fkToeC", self.fkToeC)
        RigNode.setPlug(self.node, "fkToeD", self.fkToeD)
        RigNode.setPlug(self.node, "fkToeE", self.fkToeE)
        #
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)

        RigUtility.modifyTransformChannels(self.fkToeA, r=False)
        RigUtility.modifyTransformChannels(self.fkToeB, r=False)
        RigUtility.modifyTransformChannels(self.fkToeC, r=False)
        RigUtility.modifyTransformChannels(self.fkToeD, r=False)
        RigUtility.modifyTransformChannels(self.fkToeE, r=False)
        #
        cmds.setAttr("{}.rigged".format(self.node), True)
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
        cmds.setParent('..')


    def setSpace(self, index, *args):
        pass


    def keySpace(self, *args):
        pass


    def reset(self, *args):
        controlList = [self.fkToeA, self.fkToeB, self.fkToeC, self.fkToeD, self.fkToeE]
        for control in controlList:
            if control != None:
                cmds.setAttr(control+".translateX", 0)
                cmds.setAttr(control+".translateY", 0)
                cmds.setAttr(control+".translateZ", 0)
                cmds.setAttr(control+".rotateX", 0)
                cmds.setAttr(control+".rotateY", 0)
                cmds.setAttr(control+".rotateZ", 0)


    def keyAll(self, *args):
        controlList = [self.fkToeA, self.fkToeB, self.fkToeC, self.fkToeD, self.fkToeE]
        for control in controlList:
            if control != None:
                cmds.setKeyframe(control)


    def sync(self, *args, **kwargs):
        pass
