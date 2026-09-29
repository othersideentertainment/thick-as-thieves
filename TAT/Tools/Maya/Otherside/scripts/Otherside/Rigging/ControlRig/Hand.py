from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
from Otherside.Rigging.ControlRig.Finger import Finger
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Hand"
CLASS_NAME = "Hand"
CONTROLLER_SIZE = [.1, 10, 10]
METALCARPAL_CONTROLLER_SIZE = [3,3,3]

class Hand(ControlRigBase):
    def __init__(self, instanceName = "Hand", **kwargs):
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
        self.boneHand = None
        self.boneMetacarpalIndex = None
        self.boneMetacarpalMiddle = None
        self.boneMetacarpalRing = None
        self.boneMetacarpalPinky = None
        #
        self.markerHand = None
        self.markerMetacarpalIndex = None
        self.markerMetacarpalMiddle = None
        self.markerMetacarpalRing = None
        self.markerMetacarpalPinky = None
        #
        self.controllerMetacarpal = None
        self.fkMetacarpalIndex = None
        self.fkMetacarpalMiddle = None
        self.fkMetacarpalRing = None
        self.fkMetacarpalPinky = None
        #
        sideLabel = ""
        if self.side == 1:
            sideLabel = "_L"
        elif self.side == 2:
            sideLabel = "_R"
        #
        self.thumb = Finger("thumb"+sideLabel, fingerID=1, side=self.side)
        self.index = Finger("index"+sideLabel, fingerID=2, side=self.side)
        self.middle = Finger("middle"+sideLabel, fingerID=3, side=self.side)
        self.ring = Finger("ring"+sideLabel, fingerID=4, side=self.side)
        self.pinky = Finger("pinky"+sideLabel, fingerID=5, side=self.side)


    @staticmethod
    def load(node):
        # Load Hand
        instance = Hand()
        instance.node = node
        instance.instanceName = cmds.getAttr("{}.instanceName".format(node))
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.side = cmds.getAttr("{}.side".format(node))
        instance.spaces = {}
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneHand = RigNode.getPlug(node, "boneHand")
        instance.boneMetacarpalIndex = RigNode.getPlug(node, "boneMetacarpalIndex")
        instance.boneMetacarpalMiddle = RigNode.getPlug(node, "boneMetacarpalMiddle")
        instance.boneMetacarpalRing = RigNode.getPlug(node, "boneMetacarpalRing")
        instance.boneMetacarpalPinky = RigNode.getPlug(node, "boneMetacarpalPinky")
        instance.markerMetacarpalIndex = RigNode.getPlug(node, "markerMetacarpalIndex")
        instance.markerMetacarpalMiddle = RigNode.getPlug(node, "markerMetacarpalMiddle")
        instance.markerMetacarpalRing = RigNode.getPlug(node, "markerMetacarpalRing")
        instance.markerMetacarpalPinky = RigNode.getPlug(node, "markerMetacarpalPinky")
        instance.controllerMetacarpal = RigNode.getPlug(node, "controllerMetacarpal")
        instance.fkMetacarpalIndex = RigNode.getPlug(node, "fkMetacarpalIndex")
        instance.fkMetacarpalMiddle = RigNode.getPlug(node, "fkMetacarpalMiddle")
        instance.fkMetacarpalRing = RigNode.getPlug(node, "fkMetacarpalRing")
        instance.fkMetacarpalPinky = RigNode.getPlug(node, "fkMetacarpalPinky")
        # Load Fingers
        instance.thumb = RigNode.loadPlug(node, "thumb")
        instance.index = RigNode.loadPlug(node, "index")
        instance.middle = RigNode.loadPlug(node, "middle")
        instance.ring = RigNode.loadPlug(node, "ring")
        instance.pinky = RigNode.loadPlug(node, "pinky")
        return instance


    def setBoneList(self, boneList):
        self.boneHand = boneList[0]
        self.boneMetacarpalIndex = boneList[1]
        self.boneMetacarpalMiddle = boneList[2]
        self.boneMetacarpalRing = boneList[3]
        self.boneMetacarpalPinky = boneList[4]


    def getBoneList(self):
        return [self.boneHand, self.boneMetacarpalIndex, self.boneMetacarpalMiddle, self.boneMetacarpalRing, self.boneMetacarpalPinky]


    def getBoneNames(self):
        return ["boneHand", "boneMetacarpalIndex", "boneMetacarpalMiddle", "boneMetacarpalRing", "boneMetacarpalPinky"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.controllerMetacarpal,
            self.fkMetacarpalIndex,
            self.fkMetacarpalMiddle,
            self.fkMetacarpalRing,
            self.fkMetacarpalPinky,
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerMetacarpal",
            "fkMetacarpalIndex",
            "fkMetacarpalMiddle",
            "fkMetacarpalRing",
            "fkMetacarpalPinky",
            ]


    def getMarkerList(self):
        return [self.markerHand, self.markerMetacarpalIndex, self.markerMetacarpalMiddle, self.markerMetacarpalRing, self.markerMetacarpalPinky]


    def getMarkerNames(self):
        return ["markerHand", "markerMetacarpalIndex", "markerMetacarpalMiddle", "markerMetacarpalRing", "markerMetacarpalPinky"]


    def createNode(self):
        # Build Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneHand", "message")
        nodeBuilder.addAttr("boneMetacarpalIndex", "message")
        nodeBuilder.addAttr("boneMetacarpalMiddle", "message")
        nodeBuilder.addAttr("boneMetacarpalRing", "message")
        nodeBuilder.addAttr("boneMetacarpalPinky", "message")
        nodeBuilder.addAttr("markerMetacarpalIndex", "message")
        nodeBuilder.addAttr("markerMetacarpalMiddle", "message")
        nodeBuilder.addAttr("markerMetacarpalRing", "message")
        nodeBuilder.addAttr("markerMetacarpalPinky", "message")
        nodeBuilder.addAttr("controllerMetacarpal", "message")
        nodeBuilder.addAttr("fkMetacarpalIndex", "message")
        nodeBuilder.addAttr("fkMetacarpalMiddle", "message")
        nodeBuilder.addAttr("fkMetacarpalRing", "message")
        nodeBuilder.addAttr("fkMetacarpalPinky", "message")
        nodeBuilder.addAttr("thumb", "message")
        nodeBuilder.addAttr("index", "message")
        nodeBuilder.addAttr("middle", "message")
        nodeBuilder.addAttr("ring", "message")
        nodeBuilder.addAttr("pinky", "message")
        self.node = nodeBuilder.write()
        # Set Rig Node Values
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)
        # Fingers
        fingerPlugs = ["thumb", "index", "middle", "ring", "pinky"]
        fingerList = [self.thumb, self.index, self.middle, self.ring, self.pinky]
        for i in range(0, len(fingerList)):
            fingerList[i].createNode()
            RigNode.setPlug(self.node, fingerPlugs[i], fingerList[i].node)


    def characterize(self, **kwargs):
        parentGroup = kwargs.get("p", None)
        ''' Get Required Variables '''
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
        # Mark Characterized
        self.characterized = True
        # Setup Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerMetacarpalIndex = Marker.create(n="markerMetacarpalIndex", t=self.boneMetacarpalIndex, p=self.markerGroup)
        self.markerMetacarpalMiddle = Marker.create(n="markerMetacarpalMiddle", t=self.boneMetacarpalMiddle, p=self.markerGroup)
        self.markerMetacarpalRing = Marker.create(n="markerMetacarpalRing", t=self.boneMetacarpalRing, p=self.markerGroup)
        self.markerMetacarpalPinky = Marker.create(n="markerMetacarpalPinky", t=self.boneMetacarpalPinky, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [0,1,0]
        if self.side == 2:
            aimAxis = [-1,0,0]
            upAxis = [0,0,-1]
        boneList = [self.boneMetacarpalIndex, self.boneMetacarpalMiddle, self.boneMetacarpalRing, self.boneMetacarpalPinky]
        markerList = [self.markerMetacarpalIndex, self.markerMetacarpalMiddle, self.markerMetacarpalRing, self.markerMetacarpalPinky]
        for i in range(0,len(markerList)):
            child = cmds.listRelatives(boneList[i], c=True)
            if child:
                Marker.orient(markerList[i], target=child[0], aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "boneHand", self.boneHand)
        RigNode.setPlug(self.node, "boneMetacarpalIndex", self.boneMetacarpalIndex)
        RigNode.setPlug(self.node, "boneMetacarpalMiddle", self.boneMetacarpalMiddle)
        RigNode.setPlug(self.node, "boneMetacarpalRing", self.boneMetacarpalRing)
        RigNode.setPlug(self.node, "boneMetacarpalPinky", self.boneMetacarpalPinky)
        RigNode.setPlug(self.node, "markerMetacarpalIndex", self.markerMetacarpalIndex)
        RigNode.setPlug(self.node, "markerMetacarpalMiddle", self.markerMetacarpalMiddle)
        RigNode.setPlug(self.node, "markerMetacarpalRing", self.markerMetacarpalRing)
        RigNode.setPlug(self.node, "markerMetacarpalPinky", self.markerMetacarpalPinky)
        # Characterize Fingers
        fingerList = [self.thumb, self.index, self.middle, self.ring, self.pinky]
        for finger in fingerList:
            finger.characterize(p=self.controlGroup)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        boneParent = self.boneHand
        # Setup Constraints to Parent Bone (If Applicable)
        if boneParent != None:
            cmds.matchTransform(self.controlGroup, boneParent)
            pc = cmds.parentConstraint(boneParent, self.controlGroup, mo=True)
            cmds.parent(pc, self.systemGroup)
        #Setup Controls
        self.setupFingerControls()
        self.setupMetacarpalControls()
        cmds.reorder(self.fkMetacarpalPinky, f=True)
        cmds.reorder(self.fkMetacarpalRing, f=True)
        cmds.reorder(self.fkMetacarpalMiddle, f=True)
        cmds.reorder(self.fkMetacarpalIndex, f=True)
        cmds.reorder(self.controllerMetacarpal, f=True)
        #Proxy Attr
        cmds.addAttr(self.controlGroup, ln="Hand", at="float", k=True)
        cmds.setAttr(self.controlGroup+".Hand", l=True)
        cmds.addAttr(self.controlGroup, ln="fist", proxy=self.systemGroup+".fist")
        cmds.addAttr(self.controlGroup, ln="relax", proxy=self.systemGroup+".relax")
        cmds.addAttr(self.controlGroup, ln="curlFingers", proxy=self.systemGroup+".curlFingers")
        cmds.addAttr(self.controlGroup, ln="curlThumb", proxy=self.systemGroup+".curlThumb")
        cmds.addAttr(self.controlGroup, ln="Fingers", at="float", k=True)
        cmds.setAttr(self.controlGroup+".Fingers", l=True)
        cmds.addAttr(self.controlGroup, ln="thumbCurl", proxy=self.thumb.systemGroup+".curl")
        cmds.addAttr(self.controlGroup, ln="indexCurl", proxy=self.index.systemGroup+".curl")
        cmds.addAttr(self.controlGroup, ln="middleCurl", proxy=self.middle.systemGroup+".curl")
        cmds.addAttr(self.controlGroup, ln="ringCurl", proxy=self.ring.systemGroup+".curl")
        cmds.addAttr(self.controlGroup, ln="pinkyCurl", proxy=self.pinky.systemGroup+".curl")
        # Set Rig Node
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "controllerMetacarpal", self.controllerMetacarpal)
        RigNode.setPlug(self.node, "fkMetacarpalIndex", self.fkMetacarpalIndex)
        RigNode.setPlug(self.node, "fkMetacarpalMiddle", self.fkMetacarpalMiddle)
        RigNode.setPlug(self.node, "fkMetacarpalRing", self.fkMetacarpalRing)
        RigNode.setPlug(self.node, "fkMetacarpalPinky", self.fkMetacarpalPinky)
        #
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        #
        RigUtility.modifyTransformChannels(self.fkMetacarpalIndex, r=False)
        RigUtility.modifyTransformChannels(self.fkMetacarpalMiddle, r=False)
        RigUtility.modifyTransformChannels(self.fkMetacarpalRing, r=False)
        RigUtility.modifyTransformChannels(self.fkMetacarpalPinky, r=False)
        #
        cmds.setAttr("{}.rigged".format(self.node), True)
        #
        self.rigged = True


    def setupFingerControls(self):
        #Rig Fingers
        self.thumb.rig()
        self.index.rig()
        self.middle.rig()
        self.ring.rig()
        self.pinky.rig()
        #Create Hand Control
        cmds.addAttr(self.systemGroup, ln="curlThumb", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.systemGroup, ln="curlFingers", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.systemGroup, ln="relax", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.systemGroup, ln="fist", at="float", min=0, max=1, dv=0, k=True)
        #Rig Fingers
        relaxPose = [0, .1, .15, .2, .25]
        fingerList = [self.thumb, self.index, self.middle, self.ring, self.pinky]
        for i in range(0,len(fingerList)):
            fingerControlGroup = fingerList[i].systemGroup
            cmds.setAttr(fingerControlGroup+".relaxPose", relaxPose[i])
            curlAdd = cmds.createNode("plusMinusAverage", n="curlAddNode")
            if i == 0:
                cmds.setAttr(fingerControlGroup+".curlMaxAngleA", 10)
                cmds.connectAttr(self.systemGroup+".curlThumb", curlAdd+".input1D[0]")
            else:
                cmds.connectAttr(self.systemGroup+".curlFingers", curlAdd+".input1D[0]")
            #Fist
            fistNode = cmds.createNode("floatMath", n="Fist")
            cmds.setAttr(fistNode+".operation", 2)
            cmds.connectAttr(self.systemGroup+".fist", fistNode+".floatA")
            cmds.connectAttr(fingerControlGroup+".fistPose", fistNode+".floatB")
            cmds.connectAttr(fistNode+".outFloat", curlAdd+".input1D[1]")
            #Relax
            relaxNode = cmds.createNode("floatMath", n="Relax")
            cmds.setAttr(relaxNode+".operation", 2)
            cmds.connectAttr(self.systemGroup+".relax", relaxNode+".floatA")
            cmds.connectAttr(fingerControlGroup+".relaxPose", relaxNode+".floatB")
            cmds.connectAttr(relaxNode+".outFloat", curlAdd+".input1D[2]")
            #
            cmds.connectAttr(curlAdd+".output1D", fingerControlGroup+".curlAdd")


    def setupMetacarpalControls(self):
        boneList = [self.boneMetacarpalPinky, self.boneMetacarpalRing, self.boneMetacarpalMiddle]
        markerList = [self.markerMetacarpalPinky, self.markerMetacarpalRing, self.markerMetacarpalMiddle]
        boneParent = self.boneHand
        #
        buffer = cmds.getAttr(self.boneMetacarpalPinky+".translate")[0]
        offset = [buffer[0], buffer[1], buffer[2]]
        offset[1] = offset[1] + 2

        #Setup Hand Spread Controller
        self.controllerMetacarpal = Controller.create(
            name = Controller.buildName("hand", self.side, "spread"),
            parent = self.controlGroup,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.YELLOW,
            size = [2,2,2],
            offset = offset,
            normal = [0,1,0]
            )
        cmds.matchTransform(self.controllerMetacarpal, boneParent)
        RigUtility.bakeOffsetParentMatrix(self.controllerMetacarpal)
        RigUtility.parentByMatrix(self.controllerMetacarpal, boneParent, mo=True)
        RigUtility.resetTransform(self.controllerMetacarpal)
        #
        driverGroup = cmds.createNode("transform", n="driverGroup", p=self.systemGroup)
        cmds.matchTransform(driverGroup, boneParent, pos=True)
        cmds.matchTransform(driverGroup, self.markerMetacarpalIndex, rot=True)
        RigUtility.parentByMatrix(driverGroup, boneParent, mo=True)
        RigUtility.resetTransform(driverGroup)
        #
        driverBase = cmds.createNode("transform", p=driverGroup, n="driverBase")
        driverTarget = cmds.createNode("transform", p=driverGroup, n="driverTarget")
        RigUtility.parentByMatrix(driverTarget, self.controllerMetacarpal, mo=True)
        #
        boneCount = len(boneList)
        driverList = []
        #
        for i in range(0,len(boneList)):
            boneName = RigUtility.shortNameOf(boneList[i])
            weight = (boneCount-float(i)) / boneCount
            driverOffset = cmds.createNode("transform", n=boneName+"_driverOffset", p=driverGroup)
            driver = cmds.createNode("transform", n=boneName+"_driver", p=driverOffset)
            #Blend Matrix - Rotation Only
            blendmatrix = cmds.createNode("blendMatrix", n=boneName+"_blendMatrix")
            cmds.connectAttr(driverBase+".worldMatrix", blendmatrix+".inputMatrix")
            cmds.connectAttr(driverTarget+".worldMatrix", blendmatrix+".target[0].targetMatrix")
            cmds.setAttr(blendmatrix+".target[0].weight", weight)
            cmds.setAttr(blendmatrix+".target[0].useTranslate", False)
            cmds.setAttr(blendmatrix+".target[0].useScale", False)
            cmds.setAttr(blendmatrix+".target[0].useShear", False)
            pickMatrixA = cmds.createNode("pickMatrix")
            cmds.setAttr(pickMatrixA+".useTranslate", False)
            cmds.setAttr(pickMatrixA+".useScale", False)
            cmds.setAttr(pickMatrixA+".useShear", False)
            cmds.connectAttr(blendmatrix+".outputMatrix", pickMatrixA+".inputMatrix")
            #Parent Inverse World Rotation Only
            pickMatrixB = cmds.createNode("pickMatrix")
            cmds.setAttr(pickMatrixB+".useTranslate", False)
            cmds.setAttr(pickMatrixB+".useScale", False)
            cmds.setAttr(pickMatrixB+".useShear", False)
            cmds.connectAttr(driverGroup+".worldInverseMatrix[0]", pickMatrixB+".inputMatrix")
            #Multiply OffsetParentMatrix
            offsetParentMatrix = cmds.createNode("multMatrix")
            cmds.connectAttr(pickMatrixA+".outputMatrix", offsetParentMatrix+".matrixIn[2]")
            cmds.connectAttr(pickMatrixB+".outputMatrix", offsetParentMatrix+".matrixIn[3]")
            cmds.connectAttr(offsetParentMatrix+".matrixSum", driver+".offsetParentMatrix")
            #Calculate the offset Parent Matrix to match marker orientation
            cmds.matchTransform(driverOffset, markerList[i])
            RigUtility.resetTransform(driver)
            #create controller to sit under the driver
            driverList.append(driver)
        #Create Controllers to Driver Metacarpal
        driverList.reverse()
        markerList = [self.markerMetacarpalIndex, self.markerMetacarpalMiddle, self.markerMetacarpalRing, self.markerMetacarpalPinky]
        boneList = [self.boneMetacarpalIndex, self.boneMetacarpalMiddle, self.boneMetacarpalRing, self.boneMetacarpalPinky]
        controlNameList = ["metacarpal_index","metacarpal_middle","metacarpal_ring","metacarpal_pinky"]
        controlList = [None, None, None, None]
        for i in range(0,len(boneList)):
            boneName = RigUtility.shortNameOf(boneList[i])
            controlName = controlNameList[i]
            controlList[i] = Controller.create(
                name = Controller.buildName(controlName, self.side, "FK"),
                parent = self.controlGroup,
                shape = Controller.Shape.CIRCLE,
                color = Controller.Color.CYAN,
                size = METALCARPAL_CONTROLLER_SIZE,
                normal = [0,1,0])
            cmds.matchTransform(controlList[i], markerList[i])
            if i == 0:
                parent = RigUtility.firstParentOf(boneList[i])
                cmds.matchTransform(controlList[i], markerList[i])
                RigUtility.parentByMatrix(controlList[i], parent, mo=True)
                RigUtility.resetTransform(controlList[i])
            else:
                control = controlList[i]
                driver = driverList[i-1]
                RigUtility.parentByMatrix(controlList[i], driver, mo=True)
                RigUtility.resetTransform(controlList[i])
                #
            #connect controller to bone
            cmds.parentConstraint(controlList[i], boneList[i], mo=True)
        #
        self.fkMetacarpalIndex = controlList[0]
        self.fkMetacarpalMiddle = controlList[1]
        self.fkMetacarpalRing = controlList[2]
        self.fkMetacarpalPinky = controlList[3]


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        #Hand Poses
        cmds.rowLayout(nc=4, width=350)
        cmds.text("Hand Pose", width=125, align="left")
        cmds.button(l="Default", width=75, c=partial(self.setPose, 0))
        cmds.button(l="Relax", width=75, c=partial(self.setPose, 1))
        cmds.button(l="Fist", width=75, c=partial(self.setPose, 2))
        cmds.setParent("..")
        #Finger Curl Sliders
        cmds.text("Finger Curl Sliders ", width=125, align="left")
        cmds.attrFieldSliderGrp(l="Thumb", at=self.thumb.controlGroup+".curl", smn=0, smx=1)
        cmds.attrFieldSliderGrp(l="Index", at=self.index.controlGroup+".curl", smn=0, smx=1)
        cmds.attrFieldSliderGrp(l="Middle", at=self.middle.controlGroup+".curl", smn=0, smx=1)
        cmds.attrFieldSliderGrp(l="Ring", at=self.ring.controlGroup+".curl", smn=0, smx=1)
        cmds.attrFieldSliderGrp(l="Pinky", at=self.pinky.controlGroup+".curl", smn=0, smx=1)
        cmds.setParent('..')


    def setPose(self, pose_id, *args):
        self.thumb.setPose(pose_id)
        self.index.setPose(pose_id)
        self.middle.setPose(pose_id)
        self.ring.setPose(pose_id)
        self.pinky.setPose(pose_id)


    def reset(self, *args):
        attrList = ["fist", "relax", "curlFingers", "curlThumb",
            "thumbCurl", "indexCurl", "middleCurl", "ringCurl", "pinkyCurl"]
        for attr in attrList:
            cmds.setAttr(self.controlGroup+"."+attr, 0)
        controlList = [self.controllerMetacarpal, self.fkMetacarpalIndex, self.fkMetacarpalMiddle, self.fkMetacarpalRing, self.fkMetacarpalPinky]
        for control in controlList:
            if control != None:
                #cmds.setAttr(control+".translateX", 0)
                #cmds.setAttr(control+".translateY", 0)
                #cmds.setAttr(control+".translateZ", 0)
                cmds.setAttr(control+".rotateX", 0)
                cmds.setAttr(control+".rotateY", 0)
                cmds.setAttr(control+".rotateZ", 0)
        # Reset Fingers
        self.thumb.reset()
        self.index.reset()
        self.middle.reset()
        self.ring.reset()
        self.pinky.reset()


    def keyAll(self, *args):
        # Key Hand Controls
        controlList = [self.controllerMetacarpal, self.fkMetacarpalIndex, self.fkMetacarpalMiddle, self.fkMetacarpalRing, self.fkMetacarpalPinky]
        for control in controlList:
            if control != None:
                cmds.setKeyframe(control)
        attrList = ["fist", "relax", "curlFingers", "curlThumb",
        "thumbCurl", "indexCurl", "middleCurl", "ringCurl", "pinkyCurl"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)
        # Key Fingers
        self.thumb.keyAll()
        self.index.keyAll()
        self.middle.keyAll()
        self.ring.keyAll()
        self.pinky.keyAll()


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if node:
            instance = Hand.load(node)
            if instance:
                self.reset()
                #Add matching for metacarpal controls
                if instance.thumb.node:
                    self.thumb.sync(node=instance.thumb.node)
                if instance.index.node:
                    self.index.sync(node=instance.index.node)
                if instance.middle.node:
                    self.middle.sync(node=instance.middle.node)
                if instance.ring.node:
                    self.ring.sync(node=instance.ring.node)
                if instance.pinky.node:
                    self.pinky.sync(node=instance.pinky.node)