from collections import OrderedDict
import maya.cmds as cmds
import maya.mel as mel
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UtilityNodes as UtilityNodes
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase



RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.HindLegSpring"
CLASS_NAME = "HindLeg"
CONTROLLER_SIZE = [5, 5, 5]
IK_FOOT_SIZE = [10,10,10]
IK_KNEE_SIZE = [5,5,5]
IK_HOCK_SIZE = [10,10,10]
IK_HEEL_SIZE = [10,10,10]
IK_TOES_SIZE = [10,10,10]


class HindLegSpring(ControlRigBase):
    def __init__(self, instanceName="HindLeg", **kwargs):
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
        self.boneHock = None
        self.boneFoot = None
        self.boneToes = None
        #
        self.markerUpperLeg = None
        self.markerLowerLeg = None
        self.markerHock = None
        self.markerFoot = None
        self.markerToes = None
        #
        self.fkSpace = None
        self.fkUpperLeg = None
        self.fkLowerLeg = None
        self.fkHock = None
        self.fkFoot = None
        self.fkToes = None
        #
        self.ikSpace = None
        self.ikFoot = None
        self.ikKnee = None
        self.ikHock = None
        self.ikHeel = None
        self.ikToes = None
        self.ikFootMarker = None


    @staticmethod
    def load(node):
        instance = HindLegSpring()
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
        instance.boneHock = RigNode.getPlug(node, "boneHock")
        instance.boneFoot = RigNode.getPlug(node, "boneFoot")
        instance.boneToes = RigNode.getPlug(node, "boneToes")
        #Marker
        instance.markerUpperLeg = RigNode.getPlug(node, "markerUpperLeg")
        instance.markerLowerLeg = RigNode.getPlug(node, "markerLowerLeg")
        instance.markerHock = RigNode.getPlug(node, "markerHock")
        instance.markerFoot = RigNode.getPlug(node, "markerFoot")
        instance.markerToes = RigNode.getPlug(node, "markerToes")
        #FK
        instance.fkSpace = RigNode.getPlug(node, "fkSpace")
        instance.fkUpperLeg = RigNode.getPlug(node, "fkUpperLeg")
        instance.fkLowerLeg = RigNode.getPlug(node, "fkLowerLeg")
        instance.fkHock = RigNode.getPlug(node, "fkHock")
        instance.fkFoot = RigNode.getPlug(node, "fkFoot")
        instance.fkToes = RigNode.getPlug(node, "fkToes")
        #IK
        instance.ikSpace = RigNode.getPlug(node, "ikSpace")
        instance.ikFoot = RigNode.getPlug(node, "ikFoot")
        instance.ikKnee = RigNode.getPlug(node, "ikKnee")
        instance.ikHock = RigNode.getPlug(node, "ikHock")
        instance.ikHeel = RigNode.getPlug(node, "ikHeel")
        instance.ikToes = RigNode.getPlug(node, "ikToes")
        instance.ikFootMarker = RigNode.getPlug(node, "ikFootMarker")
        return instance


    def setBoneList(self, boneList):
        self.boneUpperLeg = boneList[0]
        self.boneLowerLeg = boneList[1]
        self.boneHock = boneList[2]
        self.boneFoot = boneList[3]
        self.boneToes = boneList[4]


    def getBoneList(self):
        return [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]


    def getBoneNames(self):
        return ["boneUpperLeg", "boneLowerLeg", "boneHock", "boneFoot", "boneToes"]


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
        nodeBuilder.addAttr("boneHock", "message")
        nodeBuilder.addAttr("boneFoot", "message")
        nodeBuilder.addAttr("boneToes", "message")
        nodeBuilder.addAttr("markerUpperLeg", "message")
        nodeBuilder.addAttr("markerLowerLeg", "message")
        nodeBuilder.addAttr("markerHock", "message")
        nodeBuilder.addAttr("markerFoot", "message")
        nodeBuilder.addAttr("markerToes", "message")
        nodeBuilder.addAttr("fkSpace","message")
        nodeBuilder.addAttr("fkUpperLeg","message")
        nodeBuilder.addAttr("fkLowerLeg","message")
        nodeBuilder.addAttr("fkHock","message")
        nodeBuilder.addAttr("fkFoot","message")
        nodeBuilder.addAttr("fkToes","message")
        nodeBuilder.addAttr("ikSpace","message")
        nodeBuilder.addAttr("ikFoot","message")
        nodeBuilder.addAttr("ikKnee","message")
        nodeBuilder.addAttr("ikHock","message")
        nodeBuilder.addAttr("ikHeel","message")
        nodeBuilder.addAttr("ikToes","message")
        nodeBuilder.addAttr("ikFootMarker","message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)


    def getMarkerList(self):
        return [self.markerUpperLeg, self.markerLowerLeg, self.markerHock, self.markerFoot]


    def getMarkerNames(self):
        return ["markerUpperLeg", "markerLowerLeg", "markerHock", "markerFoot", "markerToes"]


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
        self.markerHock = Marker.create(n="markerHock", t=self.boneHock, p=self.markerGroup)
        self.markerFoot = Marker.create(n="markerFoot", t=self.boneFoot, p=self.markerGroup)
        self.markerToes = Marker.create(n="markerToes", t=self.boneToes, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [-1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        if self.side == 2:
            aimAxis = [ aimAxis[0] * -1, aimAxis[1] * -1, aimAxis[2] * -1]
        endPoint = cmds.xform(self.markerToes, q=True, ws=True, rp=True)
        endPoint = [endPoint[0], endPoint[1], endPoint[2]+1]
        Marker.orient(self.markerUpperLeg, target=self.markerLowerLeg, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerLowerLeg, target=self.markerHock, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerHock, target=self.markerFoot, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerFoot, target=self.markerToes, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerToes, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
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
        RigNode.setPlug(self.node, "boneHock", self.boneHock)
        RigNode.setPlug(self.node, "boneFoot", self.boneFoot)
        RigNode.setPlug(self.node, "boneToes", self.boneToes)
        RigNode.setPlug(self.node, "markerUpperLeg", self.markerUpperLeg)
        RigNode.setPlug(self.node, "markerLowerLeg", self.markerLowerLeg)
        RigNode.setPlug(self.node, "markerHock", self.markerHock)
        RigNode.setPlug(self.node, "markerFoot", self.markerFoot)
        RigNode.setPlug(self.node, "markerToes", self.markerToes)


    def rig(self, **kwargs):
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
        #-FK Channels
        cmds.addAttr(self.systemGroup, ln="fkLimbScale", k=True, at="float", dv=1, min=.01)
        #-IK Channels
        #cmds.addAttr(self.systemGroup, ln="ikSpace", at="enum", en=":world:hips", k=True)
        cmds.addAttr(self.systemGroup, ln="ikPoleVectorFollow", at="float", k=True, min=0, max=1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikStretch", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikLimbScale", at="float", k=True, min=0.1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikSquash", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikSquashScale", at="float", k=True, min=0.01, dv=1)
        #Setup Controls
        fkBones = self.setupFK()
        ikBones = self.setupIK()
        # --- Build Control List ---
        IKControls = [self.ikFoot, self.ikKnee, self.ikHock, self.ikHeel, self.ikToes]
        FKControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes]
        # --- Link IK & FK Skeletons to Control Rig ---
        weightSwitch = RigUtility.blendJointChain(boneList, fkBones, ikBones)
        cmds.connectAttr(self.systemGroup+".ikMode", weightSwitch+".input", f=True)
        # --- Toggle IK & FK Visibility ---
        for ctrl in IKControls:
            cmds.connectAttr(weightSwitch+".output", ctrl+".v", f=True)
        for ctrl in FKControls:
            cmds.connectAttr(weightSwitch+".outputInverse", ctrl+".v", f=True)
        # --- Add Stretchy-Scale Constraint ---
        stretchSetList = [
            {"FK":fkBones[0], "IK":ikBones[0], "TARGET":boneList[0]},
            {"FK":fkBones[1], "IK":ikBones[1], "TARGET":boneList[1]},
            {"FK":fkBones[2], "IK":ikBones[2], "TARGET":boneList[2]},
            {"FK":fkBones[3], "IK":ikBones[3], "TARGET":boneList[3]}]
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
        # --- ProxyAttrs ---
        # |- ControlGroup - FK
        cmds.addAttr(self.controlGroup, ln="ikMode", proxy=self.systemGroup+".ikMode")
        cmds.addAttr(self.controlGroup, ln="FK", k=True)
        cmds.setAttr(self.controlGroup+".FK", l=True)
        cmds.addAttr(self.controlGroup, ln="fkSpace", proxy=self.systemGroup+".fkSpace")
        cmds.addAttr(self.controlGroup, ln="fkLimbScale", proxy=self.systemGroup+".fkLimbScale")
        # |- ControlGroup - IK
        cmds.addAttr(self.controlGroup, ln="IK", k=True)
        cmds.setAttr(self.controlGroup+".IK", l=True)
        cmds.addAttr(self.controlGroup, ln="ikSpace", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.controlGroup, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.controlGroup, ln="ikLimbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.controlGroup, ln="ikStretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.controlGroup, ln="ikSquash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.controlGroup, ln="ikSquashScale", proxy=self.systemGroup+".ikSquashScale")
        # |- Foot
        cmds.addAttr(self.ikFoot, ln="IK", k=True)
        cmds.setAttr(self.ikFoot+".IK", l=True)
        cmds.addAttr(self.ikFoot, ln="space", proxy=self.systemGroup+".ikSpace")
        cmds.addAttr(self.ikFoot, ln="poleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.ikFoot, ln="limbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.ikFoot, ln="stretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.ikFoot, ln="squash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.ikFoot, ln="squashScale", proxy=self.systemGroup+".ikSquashScale")
        # --- Lock Unused Channels on Controls---
        # |- FK
        for ctrl in FKControls:
            RigUtility.modifyTransformChannels(ctrl, r=False)
        # |- IK
        RigUtility.modifyTransformChannels(self.ikFoot, t=False, r=False)
        RigUtility.modifyTransformChannels(self.ikKnee, t=False, r=False)
        RigUtility.modifyTransformChannels(self.ikHock, r=False)
        RigUtility.modifyTransformChannels(self.ikHeel, r=False)
        RigUtility.modifyTransformChannels(self.ikToes, r=False)
        # --- Clean Up Groups ---
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.controlGroup)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        # --- Attach to RigNode ---
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkSpace", self.fkSpace)
        RigNode.setPlug(self.node, "fkUpperLeg", self.fkUpperLeg)
        RigNode.setPlug(self.node, "fkLowerLeg", self.fkLowerLeg)
        RigNode.setPlug(self.node, "fkHock", self.fkHock)
        RigNode.setPlug(self.node, "fkFoot", self.fkFoot)
        RigNode.setPlug(self.node, "fkToes", self.fkToes)
        RigNode.setPlug(self.node, "ikSpace", self.ikSpace)
        RigNode.setPlug(self.node, "ikFoot", self.ikFoot)
        RigNode.setPlug(self.node, "ikKnee", self.ikKnee)
        RigNode.setPlug(self.node, "ikHock", self.ikHock)
        RigNode.setPlug(self.node, "ikHeel", self.ikHeel)
        RigNode.setPlug(self.node, "ikToes", self.ikToes)
        RigNode.setPlug(self.node, "ikFootMarker", self.ikFootMarker)
        # Set Rigged Status
        self.rigged = True


    #FK SETUP
    def setupFK(self):
        bonePelvis = RigUtility.firstParentOf(self.boneUpperLeg)
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
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
        self.fkHock = Controller.create(
            name = Controller.buildName("hock", self.side, "FK"),
            parent = self.fkLowerLeg,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        self.fkFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "FK"),
            parent = self.fkHock,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        self.fkToes = Controller.create(
            name = Controller.buildName("toe", self.side, "FK"),
            parent = self.fkFoot,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        #|- update position/rotation
        cmds.matchTransform(self.fkUpperLeg, self.markerUpperLeg)
        cmds.matchTransform(self.fkLowerLeg, self.markerLowerLeg)
        cmds.matchTransform(self.fkHock, self.markerHock)
        cmds.matchTransform(self.fkFoot, self.markerFoot)
        cmds.matchTransform(self.fkToes, self.markerToes)
        #|- bake transforms to offset parent matrix
        RigUtility.bakeOffsetParentMatrix(self.fkUpperLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkLowerLeg)
        RigUtility.bakeOffsetParentMatrix(self.fkHock)
        RigUtility.bakeOffsetParentMatrix(self.fkFoot)
        RigUtility.bakeOffsetParentMatrix(self.fkToes)
        #
        cmds.parentConstraint(self.fkUpperLeg, fkBones[0], mo=True)
        cmds.parentConstraint(self.fkLowerLeg, fkBones[1], mo=True)
        cmds.parentConstraint(self.fkHock, fkBones[2], mo=True)
        cmds.parentConstraint(self.fkFoot, fkBones[3], mo=True)
        cmds.parentConstraint(self.fkToes, fkBones[4], mo=True)
        # ----------------- Stretch (FK) --------------------
        RigUtility.setupFKStretch(self.fkHock, self.systemGroup+".fkLimbScale")
        RigUtility.setupFKStretch(self.fkFoot, self.systemGroup+".fkLimbScale")
        #return the bones
        return fkBones


    def setupIK(self):
        mel.eval("ikSpringSolver")
        boneList = [self.boneUpperLeg, self.boneLowerLeg, self.boneHock, self.boneFoot, self.boneToes]
        # Setup IK Space
        self.ikSpace = RigUtility.createLocalSpace("IK", boneList[0], self.controlGroup)
        boneParent = RigUtility.firstParentOf(self.boneUpperLeg)
        spaceSwitch = SpaceSwitch2.create(self.ikSpace, None)
        spaceSwitch.addSpace("hips", boneParent)
        spaceSwitch.addSpace("world", self.rootSpace, True)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "ikSpace")
        # IK Spring Group
        springBonesGroup = cmds.createNode("transform", n="IKSpring", p=self.systemGroup)
        springBones = RigUtility.cloneJointChain(boneList, "IKSpring_", springBonesGroup)
        # IK Bone Group
        ikBonesGroup = cmds.createNode("transform", n="IKBones", p=self.systemGroup)
        ikBones = RigUtility.cloneJointChain(boneList, "IKBones_", ikBonesGroup)
        boneParent = RigUtility.firstParentOf(self.boneUpperLeg)
        if boneParent != None:
            cmds.parentConstraint(boneParent, ikBones[0], mo=True)
            cmds.parentConstraint(boneParent, springBones[0], mo=True)
        # IK Spring
        ikhandle_spring = cmds.ikHandle(sol="ikSpringSolver", sj=springBones[0], ee=springBones[3])[0]
        # Offset Chain
        ikhandle_knee = cmds.ikHandle(sol="ikRPsolver", sj=ikBones[0], ee=ikBones[2])[0]
        ikhandle_foot = cmds.ikHandle(sol="ikSCsolver", sj=ikBones[2], ee=ikBones[3])[0]
        ikhandle_toe = cmds.ikHandle(sol="ikSCsolver", sj=ikBones[3], ee=ikBones[4])[0]
        # ------ Controllers ------ #
        # --- IKFoot --- #
        self.ikFoot = Controller.create(
            name = Controller.buildName("foot", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.PLANE,
            color = Controller.Color.YELLOW,
            normal = [0,1,0],
            size = IK_FOOT_SIZE,
            offset = [0,0,2.5])
        foot_pos = cmds.xform(boneList[4], q=True, ws=True, t=True)
        cmds.xform(self.ikFoot, ws=True, t=[foot_pos[0], 0, foot_pos[2]])
        RigUtility.movePivot(self.ikFoot, foot_pos)
        RigUtility.bakeOffsetParentMatrix(self.ikFoot)
        # --- IKKnee --- #
        self.ikKnee = Controller.create(
            name = Controller.buildName("knee", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.YELLOW,
            normal = [0,1,0],
            size = IK_KNEE_SIZE)
        pos = RigUtility.calculatePoleVector(boneList[0], boneList[1], boneList[2])
        cmds.xform(self.ikKnee, ws=True, t=pos)
        RigUtility.bakeOffsetParentMatrix(self.ikKnee)
        # --- IKHock --- #
        self.ikHock = Controller.create(
            name = Controller.buildName("hock", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.PINK,
            normal = [1,0,0],
            offset = [0, 0, 0],
            size = IK_HOCK_SIZE)
        # |- ankle driver
        ankleDriver = cmds.createNode("transform", n="ankle_driver", p=springBones[2])
        cmds.matchTransform(ankleDriver, springBones[3])
        RigUtility.bakeOffsetParentMatrix(ankleDriver)
        # |- ankle pivot
        anklePivot = cmds.createNode("transform", n="ankle_pivot", p=ankleDriver)
        cmds.matchTransform(anklePivot, springBones[3])
        RigUtility.bakeOffsetParentMatrix(anklePivot)
        # |- Position Hock Controller'''
        rootPoint = cmds.xform(self.boneFoot, q=True, ws=True, t=True)
        midpoint = RigUtility.getAveragePosition([self.boneHock, self.boneFoot])
        upAxis = RigUtility.calculatePlaneNormalTransform(self.boneLowerLeg, self.boneHock, self.boneFoot)
        cmds.xform(self.ikHock, ws=True, t=midpoint)
        RigUtility.aim(self.ikHock, target=self.boneHock, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=upAxis)
        cmds.xform(self.ikHock, ws=True, piv=rootPoint)
        # |- make ikHock a child of ankleDriver by matrix
        RigUtility.bakeOffsetParentMatrix(self.ikHock)
        RigUtility.parentByMatrix(self.ikHock, ankleDriver, mo=True)
        RigUtility.parentByMatrix(anklePivot, self.ikHock, mo=True)
        # --- IK Heel --- #
        self.ikHeel = Controller.create(
            name = Controller.buildName("heel", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.PINK,
            normal = [1,0,0],
            offset = [0, 0, 0],
            size = IK_HEEL_SIZE)
        # |- heel pivot
        heelPivot = cmds.createNode("transform", n="heel_pivot", p=springBones[3])
        cmds.matchTransform(heelPivot, springBones[4])
        RigUtility.bakeOffsetParentMatrix(heelPivot)
        # |- position heel controller
        rootPoint = cmds.xform(self.boneToes, q=True, ws=True, t=True)
        midpoint = RigUtility.getAveragePosition([self.boneFoot, self.boneToes])
        upAxis = RigUtility.calculatePlaneNormalTransform(self.boneHock, self.boneFoot, self.boneToes)
        cmds.xform(self.ikHeel, ws=True, t=midpoint)
        RigUtility.aim(self.ikHeel, target=self.boneFoot, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=upAxis)
        cmds.xform(self.ikHeel, ws=True, piv=rootPoint)
        # |- make ikHeel a child of ikFoot by matrix
        RigUtility.bakeOffsetParentMatrix(self.ikHeel)
        RigUtility.parentByMatrix(self.ikHeel, self.ikFoot, mo=True)
        RigUtility.parentByMatrix(heelPivot, self.ikHeel, mo=True)
        # --- IK Toes --- #
        self.ikToes = Controller.create(
            name = Controller.buildName("toes", self.side, "IK"),
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = IK_TOES_SIZE,
            normal = [1,0,0],
            parent = self.ikSpace)
        toePosition = cmds.xform(self.boneToes, q=True, ws=True, rp=True)
        cmds.matchTransform(self.ikToes, self.markerToes, position=1, rotation=1, scale=0)
        Controller.moveCurve(self.ikToes, [0,0,5])
        Controller.clampToShapeToFloor(self.ikToes, toePosition[1])
        RigUtility.bakeOffsetParentMatrix(self.ikToes)
        RigUtility.parentByMatrix(self.ikToes, self.ikFoot, mo=True)
        # --- Reparent IK Handles --- #
        ikhandle_knee = cmds.parent(ikhandle_knee, anklePivot)[0]
        ikhandle_foot = cmds.parent(ikhandle_foot, heelPivot)[0]
        ikhandle_toe = cmds.parent(ikhandle_toe, self.systemGroup)[0]
        ikhandle_spring = cmds.parent(ikhandle_spring, self.systemGroup)[0]
        cmds.parentConstraint(self.ikFoot, ikhandle_toe, mo=True)
        cmds.parentConstraint(self.ikHeel, ikhandle_spring, mo=True)
        # --- Setup Core Constraints --- #
        # |- Knee PoleVector
        cmds.poleVectorConstraint(self.ikKnee, ikhandle_knee)
        # |- Toe Constraints
        cmds.orientConstraint(self.ikToes, ikBones[4], mo=True)
        # IK Foot Marker
        self.ikFootMarker = cmds.createNode("transform", n="IKFootMarker", p=self.systemGroup)
        cmds.matchTransform(self.ikFootMarker, self.ikFoot)
        cmds.parentConstraint(self.boneFoot, self.ikFootMarker, mo=True)
        # ------ IK Knee Following ------ #
        opm = cmds.getAttr(self.ikKnee+".offsetParentMatrix")
        opmNode = RigUtility.convertMatrixToNode(opm)
        blendMatrix = cmds.createNode("blendMatrix")
        pvMarker = cmds.createNode("transform", n="ik_knee_Marker", p=self.systemGroup)
        cmds.matchTransform(pvMarker, self.ikKnee)
        cmds.parentConstraint(self.ikFoot, pvMarker, mo=True)
        multMatrix = cmds.createNode("multMatrix")
        cmds.connectAttr(pvMarker+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikSpace+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        cmds.connectAttr(opmNode+".matrixSum", blendMatrix+".inputMatrix")
        cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".ikPoleVectorFollow", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.ikKnee+".offsetParentMatrix")
        # ------ IK Stretch ------ #
        #|-calculate default length of chain
        defaultLength = RigUtility.calculateLengthOfBoneChain(ikBones[2], ikBones[4])
        #|-create distance node to get the 'current' length of chain
        leg_ik_marker = cmds.createNode("transform", n="leg_ik_marker", p=self.systemGroup)
        cmds.matchTransform(leg_ik_marker, ikBones[4], pos=True)
        cmds.parentConstraint(self.ikFoot, leg_ik_marker, mo=True)
        distanceNode = UtilityNodes.createDistanceNode(n=self.instanceName+"_IK_CurrentDistance", transformA=ikBones[2], transformB=leg_ik_marker, p=self.systemGroup)
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
        def linkSquashStretch(bone, stretchMaxNode, squashScaleNode):
            cmds.connectAttr(stretchMaxNode+".outFloat", bone+".scaleX")
            cmds.connectAttr(squashScaleNode+".outFloat", bone+".scaleY")
            cmds.connectAttr(squashScaleNode+".outFloat", bone+".scaleZ")
        stretchBoneList = [ikBones[2],ikBones[3]]
        for bone in stretchBoneList:
            linkSquashStretch(bone, stretchMaxNode, squashScaleNode)
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
        #reset control list
        controlList = [self.ikFoot, self.ikKnee, self.ikHock, self.ikHeel, self.ikToes, self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes ]
        attributeList = ["translateX", "translateY", "translateZ", "rotateX", "rotateY", "rotateZ"]
        for control in controlList:
            for attr in attributeList:
                attrPath = "{}.{}".format(control, attr)
                if cmds.getAttr(attrPath, l=True) == False:
                    cmds.setAttr(attrPath, 0)


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
            RigNode.getPlug(node, "markerHock"),
            RigNode.getPlug(node, "markerFoot"),
            RigNode.getPlug(node, "markerToes")
            ]
        fkControls = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes]
        for i in range(0,len(fkControls)):
            cmds.matchTransform(fkControls[i], markerList[i], pos=0, rot=1, scl=0)
        #==== IK ====#
        pointList = []
        for i in range(0, len(markerList)):
            pointList.append(cmds.xform(markerList[i], q=True, ws=True, t=True))
        # |- IK Foot
        cmds.matchTransform(self.ikFoot, self.ikFootMarker)
        # |- IK Heel
        upAxis = RigUtility.calculatePlaneNormal(pointList[2], pointList[3], pointList[4])
        RigUtility.aim(self.ikHeel, target=self.boneFoot, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=upAxis)
        # |- IK Hock
        ''' put logic here '''
        upAxis = RigUtility.calculatePlaneNormal(pointList[1], pointList[2], pointList[3])
        RigUtility.aim(self.ikHock, target=self.boneHock, aimAxis=[1,0,0], upAxis=[0,1,0], worldUpAxis=upAxis)
        # |- IK Knee
        pvTarget = RigUtility.calculatePoleVector(self.markerUpperLeg, self.markerLowerLeg, self.markerHock)
        cmds.xform(self.ikKnee, ws=True, t=pvTarget)
        # |- IK Toes
        cmds.matchTransform(self.ikToes, markerList[4], pos=0, rot=1, scl=0)
        #=== Reset IK Mode ===#
        cmds.setAttr(self.systemGroup+".ikMode", orgIKValue)


    def setIKMode(self, value, *args):
        self.sync()
        cmds.setAttr(self.controlGroup+".ikMode", value)


    def keyIKMode(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikMode")
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikKnee)
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.fkLowerLeg)
        cmds.setKeyframe(self.fkHock)
        cmds.setKeyframe(self.fkFoot)


    def setIKPoleVectorFollow(self, value, *args):
        pv_worldMatrix = cmds.xform(self.ikKnee, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikPoleVectorFollow", value)
        cmds.xform(self.ikKnee, ws=True, m=pv_worldMatrix)


    def keyIKPoleVectorFollow(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikPoleVectorFollow")
        cmds.setKeyframe(self.ikKnee)


    def keyAll(self, *args):
        attrList = ["fkSpace", "fkLimbScale",
        "ikMode", "ikSpace", "ikPoleVectorFollow", "ikLimbScale", "ikStretch", "ikSquash"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)
        controlList = [self.fkUpperLeg, self.fkLowerLeg, self.fkHock, self.fkFoot, self.fkToes, self.ikFoot, self.ikKnee, self.ikHock, self.ikHeel, self.ikToes]
        for control in controlList:
            cmds.setKeyframe(control)

    ###
    def setFKSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkUpperLeg, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpace", index)
        cmds.xform(self.fkUpperLeg, ws=True, m=worldMatrix)


    def setIKSpace(self, index, *args):
        foot_worldMatrix = cmds.xform(self.ikFoot, q=True, ws=True, m=True)
        pv_worldMatrix = cmds.xform(self.ikKnee, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikSpace", index)
        cmds.xform(self.ikFoot, ws=True, m=foot_worldMatrix)
        cmds.xform(self.ikKnee, ws=True, m=pv_worldMatrix)


    def keyIKSpace(self, *args):
        cmds.setKeyframe(self.ikFoot)
        cmds.setKeyframe(self.ikKnee)
        cmds.setKeyframe(self.controlGroup+".ikSpace")


    def keyFKSpace(self, *args):
        cmds.setKeyframe(self.fkUpperLeg)
        cmds.setKeyframe(self.controlGroup+".fkSpace")

    ###