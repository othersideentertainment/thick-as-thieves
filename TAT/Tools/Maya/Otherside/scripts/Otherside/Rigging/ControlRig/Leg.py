import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UtilityNodes as UtilityNodes
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Leg"
CLASS_NAME = "Leg"
CONTROLLER_SIZE = [10, 10, 10]


class Leg(ControlRigBase):
    def __init__(self, instanceName="Leg", **kwargs):
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
        self.boneFoot = None
        self.boneToe = None
        #
        self.markerUpperLeg = None
        self.markerLowerLeg = None
        self.markerFoot = None
        self.markerToe = None
        #
        self.fkSpace = None
        self.fkUpperLeg = None
        self.fkLowerLeg = None
        self.fkFoot = None
        self.fkToe = None
        #
        self.ikSpace = None
        self.ikFoot = None
        self.ikPoleVector = None
        self.ikFootMarker = None
        self.ikHeelController = None
        self.ikFootRollController = None
        self.ikHeelPivotController = None
        self.ikBallPivotController = None
        self.ikToePivotController = None
        self.ikToeController = None


    @staticmethod
    def load(node):
        if node == None:
            print("Warning Unable to load {}.  Provided node is NONE".format(CLASS_NAME))
            return None
        instance = Leg()
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
        instance.boneFoot = RigNode.getPlug(node, "boneFoot")
        instance.boneToe = RigNode.getPlug(node, "boneToe")
        #Marker
        instance.markerUpperLeg = RigNode.getPlug(node, "markerUpperLeg")
        instance.markerLowerLeg = RigNode.getPlug(node, "markerLowerLeg")
        instance.markerFoot = RigNode.getPlug(node, "markerFoot")
        instance.markerToe = RigNode.getPlug(node, "markerToe")
        #FK
        instance.fkSpace = RigNode.getPlug(node, "fkSpace")
        instance.fkUpperLeg = RigNode.getPlug(node, "fkUpperLeg")
        instance.fkLowerLeg = RigNode.getPlug(node, "fkLowerLeg")
        instance.fkFoot = RigNode.getPlug(node, "fkFoot")
        instance.fkToe = RigNode.getPlug(node, "fkToe")
        #IK
        instance.ikSpace = RigNode.getPlug(node, "ikSpace")
        instance.ikFoot = RigNode.getPlug(node, "ikFoot")
        instance.ikPoleVector = RigNode.getPlug(node, "ikPoleVector")
        instance.ikFootMarker = RigNode.getPlug(node, "ikFootMarker")
        return instance


    def setBoneList(self, boneList):
        self.boneUpperLeg = boneList[0]
        self.boneLowerLeg = boneList[1]
        self.boneFoot = boneList[2]
        self.boneToe = boneList[3]


    def getBoneList(self):
        return [self.boneUpperLeg, self.boneLowerLeg, self.boneFoot, self.boneToe]


    def getBoneNames(self):
        return ["boneUpperLeg", "boneLowerLeg", "boneFoot", "boneToe"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkUpperLeg,
            self.fkLowerLeg,
            self.fkFoot,
            self.fkToe,
            self.ikFoot,
            self.ikPoleVector,
            self.ikHeelController,
            self.ikFootRollController,
            self.ikHeelPivotController,
            self.ikBallPivotController,
            self.ikToePivotController,
            self.ikToeController
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkUpperLeg",
            "fkLowerLeg",
            "fkFoot",
            "fkToe",
            "ikFoot",
            "ikPoleVector",
            "ikHeelController",
            "ikFootRollController",
            "ikHeelPivotController",
            "ikBallPivotController",
            "ikToePivotController",
            "ikToeController"
            ]


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
        nodeBuilder.addAttr("boneFoot", "message")
        nodeBuilder.addAttr("boneToe", "message")
        nodeBuilder.addAttr("markerUpperLeg", "message")
        nodeBuilder.addAttr("markerLowerLeg", "message")
        nodeBuilder.addAttr("markerFoot", "message")
        nodeBuilder.addAttr("markerToe", "message")
        nodeBuilder.addAttr("fkSpace","message")
        nodeBuilder.addAttr("fkUpperLeg","message")
        nodeBuilder.addAttr("fkLowerLeg","message")
        nodeBuilder.addAttr("fkFoot","message")
        nodeBuilder.addAttr("fkToe","message")
        nodeBuilder.addAttr("ikSpace","message")
        nodeBuilder.addAttr("ikFoot","message")
        nodeBuilder.addAttr("ikPoleVector","message")
        nodeBuilder.addAttr("ikFootMarker","message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)

    def getMarkerList(self):
        return [self.markerUpperLeg, self.markerLowerLeg, self.markerFoot, self.markerToe]


    def getMarkerNames(self):
        return ["markerUpperLeg", "markerLowerLeg", "markerFoot", "markerToe"]


    def characterize(self, **kwargs):
        parentGroup=kwargs.get("p", None)
        #
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
        self.markerFoot = Marker.create(n="markerFoot", t=self.boneFoot, p=self.markerGroup)
        self.markerToe = Marker.create(n="markerToe", t=self.boneToe, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [-1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        if self.side == 2:
            aimAxis = [ aimAxis[0] * -1, aimAxis[1] * -1, aimAxis[2] * -1]
        endPoint = cmds.xform(self.markerToe, q=True, ws=True, rp=True)
        endPoint = [endPoint[0], endPoint[1], endPoint[2]+1]
        Marker.orient(self.markerUpperLeg, target=self.markerLowerLeg, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerLowerLeg, target=self.markerFoot, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerFoot, target=self.markerToe, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerToe, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
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
        RigNode.setPlug(self.node, "boneFoot", self.boneFoot)
        RigNode.setPlug(self.node, "boneToe", self.boneToe)
        RigNode.setPlug(self.node, "markerUpperLeg", self.markerUpperLeg)
        RigNode.setPlug(self.node, "markerLowerLeg", self.markerLowerLeg)
        RigNode.setPlug(self.node, "markerFoot", self.markerFoot)
        RigNode.setPlug(self.node, "markerToe", self.markerToe)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        boneParent = RigUtility.firstParentOf(self.boneUpperLeg)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneFoot, self.boneToe]
        #-FK Channels
        cmds.addAttr(self.systemGroup, ln="fkLimbScale", k=True, at="float", dv=1, min=.01)
        #-IK Channels
        cmds.addAttr(self.systemGroup, ln="ikPoleVectorFollow", at="float", k=True, min=0, max=1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikStretch", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikLimbScale", at="float", k=True, min=0.1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikSquash", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikSquashScale", at="float", k=True, min=0.01, dv=1)
        attrList = ["heelLift", "heelToe", "heelPivot",  "ballPivot", "toePivot", "twistInOut", "toeUpDown", "toeLeftRight", "toeTwist"]
        for attr in attrList:
            cmds.addAttr(self.systemGroup, ln=attr, at="float", k=1)
        cmds.addAttr(self.systemGroup+".heelLift", e=True, min=0)
        #Setup Controls
        fkBones = self.setupFK()
        ikBones = self.setupIK()
        #Blend IK FK
        weightSwitch = RigUtility.blendJointChain(boneList, fkBones, ikBones)
        cmds.connectAttr(self.systemGroup+".ikMode", weightSwitch+".input", f=True)
        #Stretchy - Scale Constraint
        stretchSetList = [{"FK":fkBones[0], "IK":ikBones[0], "TARGET":boneList[0]}, {"FK":fkBones[1], "IK":ikBones[1], "TARGET":boneList[1]}]
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
        #Toggle Visibility Based On Context - FK
        FKControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkFoot, self.fkToe]
        for ctrl in FKControls:
            cmds.connectAttr(weightSwitch+".outputInverse", ctrl+".v", f=True)
            RigUtility.modifyTransformChannels(ctrl, r=False)
        #Toggle Visibility Based On Context - IK
        IKControls = [self.ikFoot, self.ikPoleVector]
        for ctrl in IKControls:
            cmds.connectAttr(weightSwitch+".output", ctrl+".v", f=True)
            RigUtility.modifyTransformChannels(ctrl, t=False, r=False)
        RigUtility.modifyTransformChannels(self.controlGroup)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        #ProxyAttrs
        # | ControlGroup - FK
        cmds.addAttr(self.controlGroup, ln="ikMode", proxy=self.systemGroup+".ikMode")
        cmds.addAttr(self.controlGroup, ln="FK", k=True)
        cmds.setAttr(self.controlGroup+".FK", l=True)
        cmds.addAttr(self.controlGroup, ln="fkSpace", proxy=self.systemGroup+".fkSpace")
        cmds.addAttr(self.controlGroup, ln="fkLimbScale", proxy=self.systemGroup+".fkLimbScale")
        # |- FK UpperLeg
        cmds.addAttr(self.fkUpperLeg, ln="fkSpace", proxy=self.systemGroup+".fkSpace")
        # | ControlGroup - IK
        cmds.addAttr(self.controlGroup, ln="IK", k=True)
        cmds.setAttr(self.controlGroup+".IK", l=True)
        cmds.addAttr(self.controlGroup, ln="ikSpace", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.controlGroup, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.controlGroup, ln="ikLimbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.controlGroup, ln="ikStretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.controlGroup, ln="ikSquash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.controlGroup, ln="ikSquashScale", proxy=self.systemGroup+".ikSquashScale")
        cmds.addAttr(self.controlGroup, ln="IK_Foot", k=True)
        cmds.setAttr(self.controlGroup+".IK_Foot", l=True)
        cmds.addAttr(self.controlGroup, ln="showFootControls", proxy=self.systemGroup+".showIKFootControls")
        cmds.addAttr(self.controlGroup, ln="heelLift", proxy=self.ikHeelController+".rotateX")
        cmds.addAttr(self.controlGroup, ln="rollHeelToe", proxy=self.ikFootRollController+".rotateX")
        cmds.addAttr(self.controlGroup, ln="rollInOut", proxy=self.ikFootRollController+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="heelPivot", proxy=self.ikHeelPivotController+".rotateY")
        cmds.addAttr(self.controlGroup, ln="ballPivot", proxy=self.ikBallPivotController+".rotateY")
        cmds.addAttr(self.controlGroup, ln="toePivot", proxy=self.ikToePivotController+".rotateY")
        cmds.addAttr(self.controlGroup, ln="toeUpDown", proxy=self.ikToeController+".rotateZ")
        cmds.addAttr(self.controlGroup, ln="toeLeftRight", proxy=self.ikToeController+".rotateY")
        cmds.addAttr(self.controlGroup, ln="toeTwist", proxy=self.ikToeController+".rotateX")
        # | Foot
        cmds.addAttr(self.ikFoot, ln="IK", k=True)
        cmds.setAttr(self.ikFoot+".IK", l=True)
        cmds.addAttr(self.ikFoot, ln="space", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.ikFoot, ln="poleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.ikFoot, ln="limbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.ikFoot, ln="stretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.ikFoot, ln="squash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.ikFoot, ln="squashScale", proxy=self.systemGroup+".ikSquashScale")
        cmds.addAttr(self.ikFoot, ln="Foot", k=True)
        cmds.setAttr(self.ikFoot+".Foot", l=True)
        cmds.addAttr(self.ikFoot, ln="showFootControls", proxy=self.systemGroup+".showIKFootControls")
        cmds.addAttr(self.ikFoot, ln="heelLift", proxy=self.ikHeelController+".rotateX")
        cmds.addAttr(self.ikFoot, ln="rollHeelToe", proxy=self.ikFootRollController+".rotateX")
        cmds.addAttr(self.ikFoot, ln="rollInOut", proxy=self.ikFootRollController+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="heelPivot", proxy=self.ikHeelPivotController+".rotateY")
        cmds.addAttr(self.ikFoot, ln="ballPivot", proxy=self.ikBallPivotController+".rotateY")
        cmds.addAttr(self.ikFoot, ln="toePivot", proxy=self.ikToePivotController+".rotateY")
        cmds.addAttr(self.ikFoot, ln="toeUpDown", proxy=self.ikToeController+".rotateZ")
        cmds.addAttr(self.ikFoot, ln="toeLeftRight", proxy=self.ikToeController+".rotateY")
        cmds.addAttr(self.ikFoot, ln="toeTwist", proxy=self.ikToeController+".rotateX")
        # | Knee
        cmds.addAttr(self.ikPoleVector, ln="poleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        #Hide SystemGroup
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode Values
        cmds.setAttr("{}.rigged".format(self.node), True)
        cmds.setAttr("{}.spaces".format(self.node), str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkSpace", self.fkSpace)
        RigNode.setPlug(self.node, "fkUpperLeg", self.fkUpperLeg)
        RigNode.setPlug(self.node, "fkLowerLeg", self.fkLowerLeg)
        RigNode.setPlug(self.node, "fkFoot", self.fkFoot)
        RigNode.setPlug(self.node, "fkToe", self.fkToe)
        RigNode.setPlug(self.node, "ikSpace", self.ikSpace)
        RigNode.setPlug(self.node, "ikFoot", self.ikFoot)
        RigNode.setPlug(self.node, "ikPoleVector", self.ikPoleVector)
        RigNode.setPlug(self.node, "ikFootMarker", self.ikFootMarker)
        # Set Rigged Status
        self.rigged = True


    #FK SETUP
    def setupFK(self):
        bonePelvis = RigUtility.firstParentOf(self.boneUpperLeg)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneFoot, self.boneToe]
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
            size = CONTROLLER_SIZE
            )
        self.fkLowerLeg = Controller.create(
            name = Controller.buildName("lowerleg", self.side, "FK"),
            parent = self.fkUpperLeg,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        self.fkFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "FK"),
            parent = self.fkLowerLeg,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        self.fkToe = Controller.create(
            name = Controller.buildName("toe", self.side, "FK"),
            parent = self.fkFoot,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        #|- update position/rotation
        cmds.matchTransform(self.fkUpperLeg, self.markerUpperLeg)
        cmds.matchTransform(self.fkLowerLeg, self.markerLowerLeg)
        cmds.matchTransform(self.fkFoot, self.markerFoot)
        cmds.matchTransform(self.fkToe, self.markerToe)
        #|- bake transforms to offset parent matrix
        RigUtility.bakeOffsetParentMatrix(self.fkUpperLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkLowerLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkFoot)
        RigUtility.bakeOffsetParentMatrix(self.fkToe)
        #
        cmds.parentConstraint(self.fkUpperLeg, fkBones[0], mo=True)
        cmds.parentConstraint(self.fkLowerLeg, fkBones[1], mo=True)
        cmds.parentConstraint(self.fkFoot, fkBones[2], mo=True)
        cmds.parentConstraint(self.fkToe, fkBones[3], mo=True)
        # ----------------- Stretch (FK) --------------------
        RigUtility.setupFKStretch(self.fkLowerLeg, self.systemGroup+".fkLimbScale")
        RigUtility.setupFKStretch(self.fkFoot, self.systemGroup+".fkLimbScale")
        #return the bones
        return fkBones


    def setupIK(self):
        cmds.addAttr(self.systemGroup, ln="showIKFootControls", at="bool", k=False, dv=0)
        cmds.setAttr(self.systemGroup+".showIKFootControls", cb=True)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneFoot, self.boneToe]
        ikBones = RigUtility.cloneJointChain(boneList, "IKBone_", self.systemGroup)
        #Create Local Space - IK
        self.ikSpace = RigUtility.createLocalSpace("IK", self.boneUpperLeg, self.controlGroup)
        #Create Controllers
        self.ikFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "IK"),
            shape = Controller.Shape.PLANE,
            color = Controller.Color.YELLOW,
            size = [CONTROLLER_SIZE[0], CONTROLLER_SIZE[1], CONTROLLER_SIZE[2] * 2.75],
            offset = [0,0,CONTROLLER_SIZE[2] * 2 * .5],
            normal = [0,1,0],
            parent = self.ikSpace
            )
        #
        self.ikPoleVector = Controller.create(
            name = Controller.buildName("knee", self.side, "IK"),
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.YELLOW,
            size = [CONTROLLER_SIZE[0] * .25, CONTROLLER_SIZE[1] * .25, CONTROLLER_SIZE[2] * .25],
            normal = [0,1,0],
            parent = self.ikSpace
            )
        self.ikFootMarker = cmds.createNode("transform", n=Controller.buildName("FootMarker", self.side, "IK"), p=self.systemGroup)
        #|- set position (Foot IK Controller)
        pos = cmds.xform(ikBones[2], q=True, ws=True, t=True)
        pos[1] = 0
        cmds.xform(self.ikFoot, ws=True, t=pos)
        RigUtility.bakeOffsetParentMatrix(self.ikFoot)
        #|- Space Switch
        bonePelvis = RigUtility.firstParentOf(self.boneUpperLeg)
        spaceSwitch = SpaceSwitch2.create(self.ikSpace, None)
        spaceSwitch.addSpace("hips", bonePelvis)
        spaceSwitch.addSpace("world", self.rootSpace, True)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "ikSpace")
        #|- set position (Foot IK Marker)
        cmds.matchTransform(self.ikFootMarker, self.ikFoot, position=1, rotation=1, scale=0)
        RigUtility.bakeOffsetParentMatrix(self.ikFootMarker)
        #|-set position (Knee PoleVector Controller)
        pos = RigUtility.calculatePoleVector(ikBones[0], ikBones[1], ikBones[2], defaultVectorDirection=[0,0,1])
        cmds.xform(self.ikPoleVector, ws=True, t=pos, ro=[0,0,0])
        RigUtility.bakeOffsetParentMatrix(self.ikPoleVector)
        #-setup PoleVector Following
        opm = cmds.getAttr(self.ikPoleVector+".offsetParentMatrix")
        opmNode = RigUtility.convertMatrixToNode(opm)
        blendMatrix = cmds.createNode("blendMatrix")
        pvMarker = cmds.createNode("transform", n="ik_knee_Marker", p=self.systemGroup)
        cmds.matchTransform(pvMarker, self.ikPoleVector)
        cmds.parentConstraint(self.ikFoot, pvMarker, mo=True)
        multMatrix = cmds.createNode("multMatrix")
        cmds.connectAttr(pvMarker+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikSpace+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        cmds.connectAttr(opmNode+".matrixSum", blendMatrix+".inputMatrix")
        cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".ikPoleVectorFollow", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.ikPoleVector+".offsetParentMatrix")
        #FOOT SETUP
        pivotGroup = cmds.createNode("transform", n="FootPivots", p=self.systemGroup)
        heelPivot = cmds.createNode("transform", n="heelPivot", p=pivotGroup)
        ballPivot = cmds.createNode("transform", n="ballPivot", p=heelPivot)
        toePivot = cmds.createNode("transform", n="toePivot", p=ballPivot)
        twistLeftPivot = cmds.createNode("transform", n="TwistLeftPivot", p=toePivot)
        twistRightPivot = cmds.createNode("transform", n="TwistRightPivot", p=twistLeftPivot)
        heelLiftPivot = cmds.createNode("transform", n="heelLiftPivot", p=twistRightPivot)
        toeUpDownPivot = cmds.createNode("transform", n="toe_match_pivot", p=twistRightPivot)
        #
        leg_ik_marker = cmds.createNode("transform", n="leg_ik_marker", p=heelLiftPivot)
        ball_ik_marker = cmds.createNode("transform", n="ball_ik_marker", p=twistRightPivot)
        toe_ik_marker = cmds.createNode("transform", n="toe_ik_marker", p=toeUpDownPivot)
        #Create IK Handles
        #-Leg
        leg_ik_handle = cmds.ikHandle(sj=ikBones[0], ee=ikBones[2], solver="ikRPsolver")[0]
        leg_ik_handle = cmds.parent(leg_ik_handle, self.systemGroup)[0]
        #-Ball
        foot_ik_handle = cmds.ikHandle(sol="ikSCsolver", sj=ikBones[2], ee=ikBones[3])[0]
        foot_ik_handle = cmds.parent(foot_ik_handle, self.systemGroup)[0]
        #-Toe
        toe_end_joint = cmds.createNode("joint", n=ikBones[3]+"_end", p=ikBones[3])
        childList = cmds.listRelatives(self.boneToe, c=True)
        if childList == None:
            pointA = cmds.xform(ikBones[3], q=True, ws=True, t=True)
            average_position = [pointA[0], pointA[1], pointA[2]+5]
        else:
            average_position = RigUtility.getAveragePosition(childList)
        #cmds.move(1, toe_end_joint, r=True, ws=True, wd=True, z=True)
        cmds.xform(toe_end_joint, ws=True, t=average_position)
        toe_ik_handle = cmds.ikHandle(sol="ikSCsolver", sj=ikBones[3], ee=toe_end_joint)[0]
        toe_ik_handle = cmds.parent(toe_ik_handle, self.systemGroup)[0]
        #Calculate Positions / rotations
        fwdAxis = [0,0,1]
        upAxis = [0,1,0]
        heelPosition = cmds.xform(ikBones[2], q=True, ws=True, t=True)
        heelPosition[1] = 0
        ballPosition = cmds.xform(ikBones[3], q=True, ws=True, t=True)
        ballPosition[1] = 0
        toePosition = cmds.xform(toe_end_joint, q=True, ws=True, t=True)
        toePosition[1] = 0
        heelRotation = RigUtility.calculateLookAt(heelPosition, ballPosition, fwdAxis, upAxis)
        ballRotation = RigUtility.calculateLookAt(ballPosition, toePosition, fwdAxis, upAxis)
        toeRotation = ballRotation
        #Position the transforms
        #-PivotGroup
        cmds.matchTransform(pivotGroup, self.ikFoot)
        RigUtility.bakeOffsetParentMatrix(pivotGroup)
        #-heelPivot
        cmds.xform(heelPivot, ws=True, t=heelPosition, ro=heelRotation)
        #cmds.move(0, 0,-.05, heelPivot, r=False, os=True, wd=True)
        RigUtility.bakeOffsetParentMatrix(heelPivot)
        #-ballPivot
        cmds.xform(ballPivot, ws=True, t=ballPosition, ro=ballRotation)
        RigUtility.bakeOffsetParentMatrix(ballPivot)
        #-toePivot
        cmds.xform(toePivot, ws=True, t=toePosition, ro=toeRotation)
        RigUtility.bakeOffsetParentMatrix(toePivot)
        #-RollLeft
        cmds.xform(twistLeftPivot, ws=True, t=ballPosition, ro=ballRotation)
        #move -r -os -wd -.5 0 0 twistLeftPivot
        cmds.move(-5, 0, 0, twistLeftPivot, r=True, os=True, wd=True)
        RigUtility.bakeOffsetParentMatrix(twistLeftPivot)
        #-RollRight
        cmds.xform(twistRightPivot, ws=True, t=ballPosition, ro=ballRotation)
        #move -r -os -wd .5 0 0 twistRightPivot
        cmds.move(5, 0, 0, twistRightPivot, r=True, os=True, wd=True)
        RigUtility.bakeOffsetParentMatrix(twistRightPivot)
        #-heelLift
        cmds.xform(heelLiftPivot, ws=True, t=ballPosition, ro=ballRotation)
        RigUtility.bakeOffsetParentMatrix(heelLiftPivot)
        #-toeUpDownPivot
        cmds.matchTransform(toeUpDownPivot, self.boneToe)
        RigUtility.bakeOffsetParentMatrix(toeUpDownPivot)
        ''' UPDATE '''
        #HEEL LIFT CONTROLLER
        self.ikHeelController = Controller.create(
            name = Controller.buildName("heel", self.side, "IK"),
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.CYAN,
            normal = [0,0,1],
            size = [CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2],
            offset = [0,2,-15],
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikHeelController, heelLiftPivot)
        RigUtility.parentByMatrix(self.ikHeelController, twistRightPivot, mo=True)
        RigUtility.resetTransform(self.ikHeelController)
        cmds.connectAttr(self.ikHeelController+".rotateX", heelLiftPivot+".rotateX")
        cmds.transformLimits(self.ikHeelController, rx=(0, 45), erx=(1, 0))
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikHeelController+".v")
        RigUtility.lockChannels(self.ikHeelController,  t=True, s=True, v=True, h=True)
        cmds.setAttr(self.ikHeelController+".rotateY", k=False, l=True, cb=False)
        cmds.setAttr(self.ikHeelController+".rotateZ", k=False, l=True, cb=False)
        #TOE CONTROLLER
        self.ikToeController = Controller.create(
            name = Controller.buildName("toe", self.side, "IK"),
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            normal = [1,0,0],
            size = CONTROLLER_SIZE,
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikToeController, toeUpDownPivot)
        Controller.clampToShapeToFloor(self.ikToeController)
        RigUtility.parentByMatrix(self.ikToeController, twistRightPivot, mo=True)
        RigUtility.resetTransform(self.ikToeController)
        cmds.parentConstraint(self.ikToeController, toeUpDownPivot, mo=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikToeController+".v")
        RigUtility.lockChannels(self.ikToeController,  t=True, s=True, v=True, h=True)
        #ROLL CONTROLLER
        self.ikFootRollController = Controller.create(
            name = Controller.buildName("footroll", self.side, "IK"),
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.CYAN,
            normal = [1,0,0],
            size =  [CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2],
            offset = [0,2,0],
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikFootRollController, self.ikFoot)
        cmds.move(25, self.ikFootRollController, r=True, wd=True, z=True)
        RigUtility.bakeOffsetParentMatrix(self.ikFootRollController)
        RigUtility.parentByMatrix(self.ikFootRollController, self.ikFoot, mo=True)
        RigUtility.resetTransform(self.ikFootRollController)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikFootRollController+".v")
        RigUtility.lockChannels(self.ikFootRollController,  t=True, s=True, v=True, h=True)
        cmds.setAttr(self.ikFootRollController+".rotateY", k=False, l=True, cb=False)
        '''Pivots'''
        pivotOffset = [6,0,0]
        if self.side == 2:
            pivotOffset[0] = -6
        #TOE Pivot CONTROLLER
        self.ikToePivotController = Controller.create(
            name = Controller.buildName("toepivot", self.side, "IK"),
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = [CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2],
            offset=pivotOffset,
            normal=[0,1,0],
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikToePivotController, toePivot)
        RigUtility.parentByMatrix(self.ikToePivotController, RigUtility.firstParentOf(toePivot), mo=True)
        RigUtility.resetTransform(self.ikToePivotController)
        cmds.parentConstraint(self.ikToePivotController, toePivot, mo=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikToePivotController+".v")
        RigUtility.lockChannels(self.ikToePivotController, t=True, s=True, v=True, h=True)
        cmds.setAttr(self.ikToePivotController+".rotateZ", k=False, l=True, cb=False)
        #Ball  Pivot CONTROLLER
        self.ikBallPivotController = Controller.create(
            name = Controller.buildName("ballpivot", self.side, "IK"),
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = [CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2],
            offset=pivotOffset,
            normal=[0,1,0],
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikBallPivotController, ballPivot)
        RigUtility.parentByMatrix(self.ikBallPivotController, RigUtility.firstParentOf(ballPivot), mo=True)
        RigUtility.resetTransform(self.ikBallPivotController)
        cmds.parentConstraint(self.ikBallPivotController, ballPivot, mo=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikBallPivotController+".v")
        RigUtility.lockChannels(self.ikBallPivotController, t=True, s=True, v=True, h=True)
        cmds.setAttr(self.ikBallPivotController+".rotateZ", k=False, l=True, cb=False)
        #Heel  Pivot CONTROLLER
        self.ikHeelPivotController = Controller.create(
            name = Controller.buildName("heelpivot", self.side, "IK"),
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = [CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2, CONTROLLER_SIZE[0] * .2],
            offset=pivotOffset,
            normal=[0,1,0],
            parent = self.ikSpace
        )
        cmds.matchTransform(self.ikHeelPivotController, heelPivot)
        RigUtility.parentByMatrix(self.ikHeelPivotController, RigUtility.firstParentOf(heelPivot), mo=True)
        RigUtility.resetTransform(self.ikHeelPivotController)
        cmds.parentConstraint(self.ikHeelPivotController, heelPivot, mo=True)
        cmds.connectAttr(self.systemGroup+".showIKFootControls", self.ikHeelPivotController+".v")
        RigUtility.lockChannels(self.ikHeelPivotController, t=True, s=True, v=True, h=True)
        cmds.setAttr(self.ikHeelPivotController+".rotateZ", k=False, l=True, cb=False)
        ''' END UPDATE '''
        #Match the markers to the corresponding joint positions
        cmds.matchTransform(leg_ik_marker, ikBones[2], pos=True)
        cmds.matchTransform(ball_ik_marker, ikBones[3], pos=True)
        cmds.matchTransform(toe_ik_marker, toe_end_joint, pos=True)
        #Heel-Toe Roll
        heelToe_pns = RigUtility.createPositiveNegativeSwitch()
        cmds.connectAttr(self.ikFootRollController+".rotateX", heelToe_pns+".inputValue", f=True)
        cmds.connectAttr(heelToe_pns+".outputNegative", heelPivot+".rotateX", f=True)
        cmds.connectAttr(heelToe_pns+".outputPositive", toePivot+".rotateX", f=True)
        #Twist
        twist_pns = RigUtility.createPositiveNegativeSwitch()
        cmds.connectAttr(self.ikFootRollController+".rotateZ", twist_pns+".inputValue", f=True)
        cmds.connectAttr(twist_pns+".outputPositive", twistLeftPivot+".rotateZ", f=True)
        cmds.connectAttr(twist_pns+".outputNegative", twistRightPivot+".rotateZ", f=True)
        #Constrain the ik_handles
        cmds.parentConstraint( self.ikFoot, pivotGroup, mo=True)
        cmds.parentConstraint(leg_ik_marker, leg_ik_handle, mo=True)
        cmds.parentConstraint(ball_ik_marker, foot_ik_handle, mo=True)
        cmds.parentConstraint(toe_ik_marker, toe_ik_handle, mo=True)
        #CONSTRAINTS
        boneParent = RigUtility.firstParentOf(self.boneUpperLeg)
        if boneParent != None:
            cmds.parentConstraint(boneParent, ikBones[0], mo=True)
        cmds.poleVectorConstraint(self.ikPoleVector, leg_ik_handle)
        cmds.orientConstraint(self.ikFoot, ikBones[2], mo=True)
        cmds.parentConstraint(self.boneFoot, self.ikFootMarker, mo=True)
        #-------------------- IK Stretch --------------------------
        #|-calculate default length of chain
        lengthA = RigUtility.calculateDistanceBetweenTransforms(ikBones[0], ikBones[1])
        lengthB = RigUtility.calculateDistanceBetweenTransforms(ikBones[1], ikBones[2])
        defaultLength = lengthA + lengthB
        #|-create distance node to get the 'current' length of chain
        distanceNode = UtilityNodes.createDistanceNode(n=self.instanceName+"_IK_CurrentDistance", transformA=ikBones[0], transformB=leg_ik_marker, p=self.systemGroup)
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
        #-Bone0
        cmds.connectAttr(stretchMaxNode+".outFloat", ikBones[0]+".scaleX")
        cmds.connectAttr(squashScaleNode+".outFloat", ikBones[0]+".scaleY")
        cmds.connectAttr(squashScaleNode+".outFloat", ikBones[0]+".scaleZ")
        #-Bone1
        cmds.connectAttr(stretchMaxNode+".outFloat", ikBones[1]+".scaleX")
        cmds.connectAttr(squashScaleNode+".outFloat", ikBones[1]+".scaleY")
        cmds.connectAttr(squashScaleNode+".outFloat", ikBones[1]+".scaleZ")
        #return data
        return ikBones


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        '''Globals'''
        cmds.text(l="  Global", width=350, align="left", bgc=(.4,.4,.4))
        #IK FK Switch Widget
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikMode", label="IK Mode", labelWidth=200, fieldWidth=150, buttonLabel0="FK", buttonLabel1="IK", changeCommand=self.setIKMode, keyCommand=self.keyIKMode)
        '''FK Group'''
        cmds.text(l="  FK", width=350, align="left", bgc=(.4,.4,.4))
        fkSpaceAttr = "{}.fkSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(fkSpaceAttr, label="FK Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKSpace, keyCommand=self.keyFKSpace)
        #FK Attributes
        Widgets.WAttributeField.Create(self.controlGroup+'.fkLimbScale', label="FK Limb Scale")
        cmds.separator( height=15)
        '''IK GROUP'''
        cmds.text(l="  IK", width=350, align="left", bgc=(.4,.4,.4))
        #IK Space Switch
        ikSpaceAttr = "{}.ikSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(ikSpaceAttr, label="IK Space", labelWidth=200, fieldWidth=150, changeCommand=self.setIKSpace, keyCommand=self.keyIKSpace)
        #IK Pole Vector Follow
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikPoleVectorFollow", label="IK Knee Follow", labelWidth=200, fieldWidth=150, changeCommand=self.setIKPoleVectorFollow, keyCommand=self.keyIKPoleVectorFollow)
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikStretch", label="IK Stretch", labelWidth=200, fieldWidth=150)
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikSquash", label="IK Squash", labelWidth=200, fieldWidth=150)
        #IK Leg Attributes
        ikAttrList = [
            ["IK Limb Scale", "ikLimbScale"],
            ["IK Squash Scale", "ikSquashScale"]]
        for attr in ikAttrList:
            label = attr[0]
            attributePath = "{0}.{1}".format(self.controlGroup, attr[1])
            Widgets.WAttributeField.Create(attributePath, label=label)
        cmds.separator( height=15)
        #IK Foot Attributes
        cmds.text(l="  IK Foot", width=350, align="left", bgc=(.4,.4,.4))
        Widgets.WAttributeToggle.Create(self.controlGroup+".showFootControls", label="Show Foot Controls", keyable=False, labelWidth=200, fieldWidth=178)
        ikAttrList = [
            ["Heel Lift", "heelLift"],
            ["Heel-Toe", "rollHeelToe"],
            ["Heel Pivot", "heelPivot"],
            ["Ball Pivot", "ballPivot"],
            ["Toe Pivot", "toePivot"],
            ["Toe Up-Down", "toeUpDown"],
            ["Toe Left-Right", "toeLeftRight"],
            ["Toe Twist", "toeTwist"],
            ["Roll In Out", "rollInOut"]]
        for attr in ikAttrList:
            label = attr[0]
            attributePath = "{0}.{1}".format(self.controlGroup, attr[1])
            Widgets.WAttributeField.Create(attributePath, label=label)
        cmds.setParent('..')


    def reset(self, *args):
        #reset attributes
        cmds.setAttr(self.controlGroup+".fkSpace", 0)
        cmds.setAttr(self.controlGroup+".fkLimbScale", 1)
        cmds.setAttr(self.controlGroup+".ikMode", 1)
        cmds.setAttr(self.controlGroup+".ikSpace", 0)
        cmds.setAttr(self.controlGroup+".ikPoleVectorFollow", 1)
        cmds.setAttr(self.controlGroup+".ikLimbScale", 1)
        cmds.setAttr(self.controlGroup+".ikStretch", 0)
        cmds.setAttr(self.controlGroup+".ikSquash", 1)
        cmds.setAttr(self.controlGroup+".showFootControls", 0)
        cmds.setAttr(self.controlGroup+".heelLift", 0)
        cmds.setAttr(self.controlGroup+".rollHeelToe", 0)
        cmds.setAttr(self.controlGroup+".rollInOut", 0)
        cmds.setAttr(self.controlGroup+".heelPivot", 0)
        cmds.setAttr(self.controlGroup+".ballPivot", 0)
        cmds.setAttr(self.controlGroup+".toePivot", 0)
        cmds.setAttr(self.controlGroup+".toeUpDown", 0)
        cmds.setAttr(self.controlGroup+".toeLeftRight", 0)
        cmds.setAttr(self.controlGroup+".toeTwist", 0)
        #reset IK control list
        controlList = [self.ikFoot, self.ikPoleVector]
        for control in controlList:
            cmds.setAttr(control+".translateX", 0)
            cmds.setAttr(control+".translateY", 0)
            cmds.setAttr(control+".translateZ", 0)
            cmds.setAttr(control+".rotateX", 0)
            cmds.setAttr(control+".rotateY", 0)
            cmds.setAttr(control+".rotateZ", 0)
        #reset FK control list
        controlList = [self.fkUpperLeg, self.fkLowerLeg, self.fkFoot, self.fkToe ]
        for control in controlList:
            cmds.setAttr(control+".rotateX", 0)
            cmds.setAttr(control+".rotateY", 0)
            cmds.setAttr(control+".rotateZ", 0)


    def sync(self, *args, **kwargs):
        orgIKValue = cmds.getAttr(self.systemGroup+".ikMode")
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        # If it's an external node - temporarily switch us to FK
        if node != self.node:
            cmds.setAttr(self.controlGroup+".ikMode", 0)
        '''--- FK ---'''
        #Note: Using RigNode.GetPlug because FK controls can be used to sync to EXTERNAL bone heirarchies.
        # The ik contorls only reference the internal bones so it is read directly from the current instance
        markerList = [
            RigNode.getPlug(node, "markerUpperLeg"),
            RigNode.getPlug(node, "markerLowerLeg"),
            RigNode.getPlug(node, "markerFoot"),
            RigNode.getPlug(node, "markerToe")]
        fkControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkFoot, self.fkToe]
        for i in range(0,len(fkControls)):
            cmds.matchTransform(fkControls[i], markerList[i], pos=0, rot=1, scl=0)
        #==== IK ====#
        pvTarget = RigUtility.calculatePoleVector(self.markerUpperLeg, self.markerLowerLeg, self.markerFoot)
        cmds.matchTransform(self.ikFoot, self.ikFootMarker)
        cmds.xform(self.ikPoleVector, ws=True, t=pvTarget)
        rot = cmds.xform(self.markerToe, q=True, os=True, ro=True)
        cmds.setAttr(self.systemGroup+".toeUpDown", rot[2])
        cmds.setAttr(self.systemGroup+".toeLeftRight", rot[1])
        cmds.setAttr(self.systemGroup+".toeTwist", rot[0])
        cmds.setAttr(self.systemGroup+".ikMode", orgIKValue)


    def setIKMode(self, value, *args):
        self.sync()
        cmds.setAttr(self.controlGroup+".ikMode", value)


    def setFKSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkUpperLeg, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpace", index)
        cmds.xform(self.fkUpperLeg, ws=True, m=worldMatrix)


    def setIKSpace(self, index, *args):
        foot_worldMatrix = cmds.xform(self.ikFoot, q=True, ws=True, m=True)
        pv_worldMatrix = cmds.xform(self.ikPoleVector, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikSpace", index)
        cmds.xform(self.ikFoot, ws=True, m=foot_worldMatrix)
        cmds.xform(self.ikPoleVector, ws=True, m=pv_worldMatrix)


    def setIKPoleVectorFollow(self, value, *args):
        pv_worldMatrix = cmds.xform(self.ikPoleVector, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikPoleVectorFollow", value)
        cmds.xform(self.ikPoleVector, ws=True, m=pv_worldMatrix)


    def keyIKMode(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikMode")
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikPoleVector)
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.fkLowerLeg)
        cmds.setKeyframe(self.fkFoot)
        cmds.setKeyframe(self.fkToe)


    def keyIKPoleVectorFollow(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikPoleVectorFollow")
        cmds.setKeyframe(self.ikPoleVector)


    def keyIKSpace(self, *args):
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikPoleVector)
        cmds.setKeyframe(self.controlGroup+".ikSpace")


    def keyFKSpace(self, *args):
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.controlGroup+".fkSpace")


    def keyAll(self, *args):
        attrList = ["fkSpace", "fkLimbScale",
        "ikMode", "ikSpace", "ikPoleVectorFollow", "ikLimbScale", "ikStretch", "ikSquash",
        "heelLift", "rollHeelToe", "rollInOut", "heelPivot", "ballPivot",
        "toePivot", "toeUpDown", "toeLeftRight", "toeTwist"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)
        controlList = [self.fkUpperLeg, self.fkLowerLeg, self.fkFoot, self.fkToe, self.ikFoot, self.ikPoleVector]
        for control in controlList:
            cmds.setKeyframe(control)