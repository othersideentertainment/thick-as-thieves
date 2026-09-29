from collections import OrderedDict
import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UtilityNodes as UtilityNodes
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Torso"
CLASS_NAME = "Torso"
CONTROLLER_SIZE = [.1, 10, 10]


class Torso(ControlRigBase):
    def __init__(self, instanceName="Torso", **kwargs):
        self.node = None
        self.instanceName = instanceName
        self.spaces = {}
        self.side = 0
        #
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        #
        self.characterized = False
        self.rigged = False
        #
        self.bonePelvis = None
        self.boneSpine1 = None
        self.boneSpine2 = None
        self.boneSpine3 = None
        #
        self.markerPelvis = None
        self.markerSpine1 = None
        self.markerSpine2 = None
        self.markerSpine3 = None
        #
        self.fkSpace = None
        self.fkPelvis = None
        self.fkHips = None
        self.fkSpine1 = None
        self.fkSpine2 = None
        self.fkSpine3 = None
        #
        self.ikHips = None
        self.ikSpace = None
        self.ikPelvis = None
        self.ikSpine = None
        self.ikChest = None
        self.ikHipsMarker= None
        self.ikPelvisMarker = None
        self.ikMidMarker = None
        self.ikChestMarker = None


    @staticmethod
    def load(node):
        instance = Torso()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.spaces = {}
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        #
        instance.bonePelvis = RigNode.getPlug(node, "bonePelvis")
        instance.boneSpine1 = RigNode.getPlug(node, "boneSpine1")
        instance.boneSpine2 = RigNode.getPlug(node, "boneSpine2")
        instance.boneSpine3 = RigNode.getPlug(node, "boneSpine3")
        #
        instance.markerPelvis = RigNode.getPlug(node, "markerPelvis")
        instance.markerSpine1 = RigNode.getPlug(node, "markerSpine1")
        instance.markerSpine2 = RigNode.getPlug(node, "markerSpine2")
        instance.markerSpine3 = RigNode.getPlug(node, "markerSpine3")
        #
        instance.fkSpace = RigNode.getPlug(node, "fkSpace")
        instance.fkHips = RigNode.getPlug(node, "fkHips")
        instance.fkPelvis = RigNode.getPlug(node, "fkPelvis")
        instance.fkSpine1 = RigNode.getPlug(node, "fkSpine1")
        instance.fkSpine2 = RigNode.getPlug(node, "fkSpine2")
        instance.fkSpine3 = RigNode.getPlug(node, "fkSpine3")
        #
        instance.ikSpace = RigNode.getPlug(node, "ikSpace")
        instance.ikHips = RigNode.getPlug(node, "ikHips")
        instance.ikPelvis = RigNode.getPlug(node, "ikPelvis")
        instance.ikSpine = RigNode.getPlug(node, "ikSpine")
        instance.ikChest = RigNode.getPlug(node, "ikChest")
        instance.ikHipsMarker = RigNode.getPlug(node, "ikHipsMarker")
        instance.ikPelvisMarker = RigNode.getPlug(node, "ikPelvisMarker")
        instance.ikMidMarker = RigNode.getPlug(node, "ikMidMarker")
        instance.ikChestMarker = RigNode.getPlug(node, "ikChestMarker")
        return instance


    def setBoneList(self, boneList):
        self.bonePelvis = boneList[0]
        self.boneSpine1 = boneList[1]
        self.boneSpine2 = boneList[2]
        self.boneSpine3 = boneList[3]


    def getBoneList(self):
        return [self.bonePelvis, self.boneSpine1, self.boneSpine2, self.boneSpine3]


    def getBoneNames(self):
        return ["bonePelvis", "boneSpine1", "boneSpine2", "boneSpine3"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkPelvis,
            self.fkHips,
            self.fkSpine1,
            self.fkSpine2,
            self.fkSpine3,
            self.ikHips,
            self.ikPelvis,
            self.ikSpine,
            self.ikChest
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkPelvis",
            "fkHips",
            "fkSpine1",
            "fkSpine2",
            "fkSpine3",
            "ikHips",
            "ikPelvis",
            "ikSpine",
            "ikChest"
            ]


    def getMarkerList(self):
        return [self.markerPelvis, self.markerSpine1, self.markerSpine2, self.markerSpine3]


    def getMarkerNames(self):
        return ["markerPelvis", "markerSpine1", "markerSpine2", "markerSpine3"]


    def createNode(self):
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("bonePelvis", "message")
        nodeBuilder.addAttr("boneSpine1", "message")
        nodeBuilder.addAttr("boneSpine2", "message")
        nodeBuilder.addAttr("boneSpine3", "message")
        nodeBuilder.addAttr("markerPelvis", "message")
        nodeBuilder.addAttr("markerSpine1", "message")
        nodeBuilder.addAttr("markerSpine2", "message")
        nodeBuilder.addAttr("markerSpine3", "message")
        nodeBuilder.addAttr("fkSpace", "message")
        nodeBuilder.addAttr("fkHips", "message")
        nodeBuilder.addAttr("fkPelvis", "message")
        nodeBuilder.addAttr("fkSpine1", "message")
        nodeBuilder.addAttr("fkSpine2", "message")
        nodeBuilder.addAttr("fkSpine3", "message")
        nodeBuilder.addAttr("ikSpace", "message")
        nodeBuilder.addAttr("ikHips", "message")
        nodeBuilder.addAttr("ikPelvis", "message")
        nodeBuilder.addAttr("ikSpine", "message")
        nodeBuilder.addAttr("ikChest", "message")
        nodeBuilder.addAttr("ikHipsMarker", "message")
        nodeBuilder.addAttr("ikPelvisMarker", "message")
        nodeBuilder.addAttr("ikMidMarker", "message")
        nodeBuilder.addAttr("ikChestMarker", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, **kwargs):
        # Kwargs
        parentGroup=kwargs.get("p", None)
        '''Setup Control Rig Group'''
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # Setup System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        cmds.addAttr(self.systemGroup, ln="ikMode", at="float", min=0, max=1, dv=1, k=1)
        # RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        # Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerPelvis = Marker.create(n="markerPelvis", t=self.bonePelvis, p=self.markerGroup)
        self.markerSpine1 = Marker.create(n="markerSpine1", t=self.boneSpine1, p=self.markerGroup)
        self.markerSpine2 = Marker.create(n="markerSpine2", t=self.boneSpine2, p=self.markerGroup)
        self.markerSpine3 = Marker.create(n="markerSpine3", t=self.boneSpine3, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [-1,0,0]
        endPoint = cmds.xform(self.markerSpine3, q=True, ws=True, rp=True)
        endPoint = [endPoint[0], endPoint[1]+1, endPoint[2]]
        Marker.orient(self.markerPelvis, target=self.markerSpine1, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerSpine1, target=self.markerSpine2, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerSpine2, target=self.markerSpine3, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        Marker.orient(self.markerSpine3, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldUpAxis=worldUpAxis)
        # CreateNode
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "bonePelvis", self.bonePelvis)
        RigNode.setPlug(self.node, "boneSpine1", self.boneSpine1)
        RigNode.setPlug(self.node, "boneSpine2", self.boneSpine2)
        RigNode.setPlug(self.node, "boneSpine3", self.boneSpine3)
        RigNode.setPlug(self.node, "markerPelvis", self.markerPelvis)
        RigNode.setPlug(self.node, "markerSpine1", self.markerSpine1)
        RigNode.setPlug(self.node, "markerSpine2", self.markerSpine2)
        RigNode.setPlug(self.node, "markerSpine3", self.markerSpine3)
        cmds.setAttr("{}.characterized".format(self.node), True)
        # SEt Characterized
        self.characterized = True


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        # Setup IK/FK
        ikBones = self.setupIK()
        fkBones = self.setupFK()
        boneList = [self.bonePelvis, self.boneSpine1, self.boneSpine2, self.boneSpine3]
        # Blend IK FK
        weightSwitch = RigUtility.blendJointChain(boneList, fkBones, ikBones)
        cmds.connectAttr(self.systemGroup+".ikMode", weightSwitch+".input", f=True)
        # Stretchy - Scale Constraint
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
        # Toggle Visibility Based On Context - FK / IK
        FKControls = [self.fkPelvis, self.fkHips, self.fkSpine1, self.fkSpine2, self.fkSpine3]
        IKControls = [self.ikHips, self.ikPelvis, self.ikSpine, self.ikChest]
        for ctrl in FKControls:
            cmds.connectAttr(weightSwitch+".outputInverse", ctrl+".v", f=True)
            if ctrl == self.fkHips:
                RigUtility.modifyTransformChannels(ctrl, t=False, r=False)
            else:
                RigUtility.modifyTransformChannels(ctrl, r=False)
        for ctrl in IKControls:
            cmds.connectAttr(weightSwitch+".output", ctrl+".v", f=True)
            RigUtility.modifyTransformChannels(ctrl, t=False, r=False)
        # Proxy Attributes
        cmds.addAttr(self.controlGroup, ln="ikMode", proxy=self.systemGroup+".ikMode")
        cmds.addAttr(self.controlGroup, ln="FK", at="float", k=True)
        cmds.setAttr(self.controlGroup+".FK", l=True)
        cmds.addAttr(self.controlGroup, ln="fkSpine1Space", proxy="{}.fkSpine1Space".format(self.systemGroup))
        cmds.addAttr(self.controlGroup, ln="fkSpine2Space", proxy="{}.fkSpine2Space".format(self.systemGroup))
        cmds.addAttr(self.controlGroup, ln="fkSpine3Space", proxy="{}.fkSpine3Space".format(self.systemGroup))
        cmds.addAttr(self.controlGroup, ln="IK", at="float", k=True)
        cmds.setAttr(self.controlGroup+".IK", l=True)
        cmds.addAttr(self.controlGroup, ln="ikStretch", proxy=self.systemGroup+".stretch")
        cmds.addAttr(self.controlGroup, ln="ikAutoSpine", proxy=self.systemGroup+".autoSpine")
        # Set RigNode Plugs
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkSpace", self.fkSpace)
        RigNode.setPlug(self.node, "fkHips", self.fkHips)
        RigNode.setPlug(self.node, "fkPelvis", self.fkPelvis)
        RigNode.setPlug(self.node, "fkSpine1", self.fkSpine1)
        RigNode.setPlug(self.node, "fkSpine2", self.fkSpine2)
        RigNode.setPlug(self.node, "fkSpine3", self.fkSpine3)
        RigNode.setPlug(self.node, "ikSpace", self.ikSpace)
        RigNode.setPlug(self.node, "ikHips", self.ikHips)
        RigNode.setPlug(self.node, "ikPelvis", self.ikPelvis)
        RigNode.setPlug(self.node, "ikSpine", self.ikSpine)
        RigNode.setPlug(self.node, "ikChest", self.ikChest)
        RigNode.setPlug(self.node, "ikHipsMarker", self.ikHipsMarker)
        RigNode.setPlug(self.node, "ikPelvisMarker", self.ikPelvisMarker)
        RigNode.setPlug(self.node, "ikMidMarker", self.ikMidMarker)
        RigNode.setPlug(self.node, "ikChestMarker", self.ikChestMarker)
        cmds.setAttr("{}.rigged".format(self.node), True)
        #Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set Rigged Status
        self.rigged = True


    def setupFK(self):
        #Bone Lists
        markerList = [self.markerPelvis, self.markerSpine1, self.markerSpine2, self.markerSpine3]
        boneList = [self.bonePelvis, self.boneSpine1, self.boneSpine2, self.boneSpine3]
        fkBones = RigUtility.cloneJointChain(boneList, "fkbone_", self.systemGroup)
        # FK Space
        self.fkSpace = RigUtility.createLocalSpace("FK", self.bonePelvis, self.controlGroup)
        # Create Controllers
        normal = [1,0,0]
        shape = Controller.Shape.BOX
        pelvisOffset = RigUtility.calculateDistanceBetweenTransforms(self.bonePelvis, self.boneSpine1) * 1.25
        # | hips
        self.fkHips = Controller.create(
            name = Controller.buildName("hips",  None, "FK"),
            parent = self.fkSpace,
            shape = shape,
            color = Controller.Color.GREEN,
            size = [CONTROLLER_SIZE[0] * 2, CONTROLLER_SIZE[1] * 2, CONTROLLER_SIZE[2] * 2],
            normal = normal
            )
        # | pelvis
        self.fkPelvis = Controller.create(
            name = Controller.buildName("pelvis", None, "FK"),
            parent = self.fkHips,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = [CONTROLLER_SIZE[0] * 2.5, CONTROLLER_SIZE[1] * 2.5, CONTROLLER_SIZE[2] * 2.5],
            offsetX = -pelvisOffset,
            normal = normal
            )
        # | spine1
        self.fkSpine1 = Controller.create(
            name = Controller.buildName("spine1", None, "FK"),
            parent = self.fkHips,
            shape = shape,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE,
            normal = normal
            )
        # | spine2
        self.fkSpine2 = Controller.create(
            name = Controller.buildName("spine2", None, "FK"),
            parent = self.fkHips,
            shape = shape,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE,
            normal = normal
            )
        # | spine3
        self.fkSpine3 = Controller.create(
            name = Controller.buildName("spine3", None, "FK"),
            parent = self.fkHips,
            shape = shape,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE,
            normal = normal
            )
        # Match Transforms
        cmds.matchTransform(self.fkHips, markerList[0])
        cmds.matchTransform(self.fkPelvis, markerList[1])
        cmds.matchTransform(self.fkSpine1, markerList[1])
        cmds.matchTransform(self.fkSpine2, markerList[2])
        cmds.matchTransform(self.fkSpine3, markerList[3])
        # Bake Transforms
        RigUtility.bakeOffsetParentMatrix(self.fkHips)
        RigUtility.bakeOffsetParentMatrix(self.fkPelvis)
        RigUtility.bakeOffsetParentMatrix(self.fkSpine1)
        RigUtility.bakeOffsetParentMatrix(self.fkSpine2)
        RigUtility.bakeOffsetParentMatrix(self.fkSpine3)
        RigUtility.parentByMatrix(self.fkHips, self.rootSpace, mo=True)
        RigUtility.parentByMatrix(self.fkPelvis, self.fkHips, mo=True)
        # Space Switch
        # | spine1
        spaceSwitch = SpaceSwitch2.create(self.fkSpine1, self.fkHips)
        spaceSwitch.addSpace("hips", self.fkHips)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "fkSpine1Space")
        # | spine2
        spaceSwitch = SpaceSwitch2.create(self.fkSpine2, self.boneSpine1)
        spaceSwitch.addSpace("spine1", self.boneSpine1)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "fkSpine2Space")
        # | spine3
        spaceSwitch = SpaceSwitch2.create(self.fkSpine3, self.boneSpine2)
        spaceSwitch.addSpace("spine2", self.boneSpine2)
        spaceSwitch.addSpace("world", None)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "fkSpine3Space")
        # Creat Constraints
        cmds.parentConstraint(self.fkPelvis, fkBones[0], mo=True)
        cmds.parentConstraint(self.fkSpine1, fkBones[1], mo=True)
        cmds.parentConstraint(self.fkSpine2, fkBones[2], mo=True)
        cmds.parentConstraint(self.fkSpine3, fkBones[3], mo=True)
        return fkBones


    def setupIK(self):
        #Bone Lists
        markerList = [self.markerPelvis, self.markerSpine1, self.markerSpine2, self.markerSpine3]
        boneList = [self.bonePelvis, self.boneSpine1, self.boneSpine2, self.boneSpine3]
        ikBones = RigUtility.cloneJointChain(boneList, "ikbone_", self.systemGroup)
        # IK Space
        self.ikSpace = RigUtility.createLocalSpace("IK", self.bonePelvis, self.controlGroup)
        #-- CREATE CONTROL CURVE
        resolution_scale = 4
        chain_length = len(ikBones)
        path = [self.boneSpine3]
        while path[-1] != self.bonePelvis:
            parent = cmds.listRelatives(path[-1], p=True, f=True)[0]
            path.append(parent)
        path.reverse()
        points = []
        for obj in path:
            point = cmds.xform(obj, q=True, ws=True, t=True)
            points.append(point)
        splineCurve = cmds.curve(d=1, p=points)
        cmds.parent(splineCurve, self.systemGroup)
        #-- CREATE SPLINE JOINTS
        splineBoneList = []
        bone = cmds.createNode("joint", n="ik_spline0", p=self.systemGroup)
        splineBoneList.append(bone)
        cmds.xform(bone, ws=True, t=points[0])
        resolution = chain_length * resolution_scale
        for i in range(1, (resolution)):
            bone = cmds.createNode("joint", n="ik_spline"+str(i), p=self.systemGroup)
            splineBoneList.append(bone)
            param = float(i) / (resolution-1)
            pos = cmds.pointOnCurve(splineCurve, pr=param, top=True, p=True)
            cmds.xform(bone, ws=True, t=pos)
        for i in range(1, len(splineBoneList)):
            splineBoneList[i] = cmds.parent(splineBoneList[i], splineBoneList[i-1])[0]
        cmds.joint(splineBoneList[0], e=True, oj="xyz", secondaryAxisOrient="zdown", ch=True)
        cmds.setAttr(splineBoneList[-1]+".jointOrient", 0,0,0)
        #-- CREATE CONTROL JOINTS
        position_hips = cmds.xform(splineBoneList[0], q=True, ws=True, t=True)
        position_chest = cmds.xform(splineBoneList[-1], q=True, ws=True, t=True)
        midpoint = [
            (position_hips[0] + position_chest[0]) * .5,
            (position_hips[1] + position_chest[1]) * .5,
            (position_hips[2] + position_chest[2]) * .5
            ]
        control_joint_hips = cmds.createNode("joint", n="control_joint_hips", p=self.systemGroup)
        control_joint_spine = cmds.createNode("joint", n="control_joint_spine", p=self.systemGroup)
        control_joint_chest = cmds.createNode("joint", n="control_joint_chest", p=self.systemGroup)
        cmds.matchTransform(control_joint_hips, splineBoneList[0])
        cmds.matchTransform(control_joint_spine, splineBoneList[0])
        cmds.matchTransform(control_joint_chest, splineBoneList[-1])
        cmds.xform(control_joint_spine, ws=True, t=midpoint)
        #-- CREATE CONTROLLERS
        size = [15,5,15]
        normal = [0,1,0]
        #hips
        self.ikHips = Controller.create(
            name="hips_IK",
            parent=self.ikSpace,
            shape=Controller.Shape.BOX,
            color=Controller.Color.GREEN,
            normal=[0,1,0],
            size=[25,0,25],
            offset=[0,-2,0])
        #chest
        self.ikChest = Controller.create(
            name="chest_IK",
            parent=self.ikHips,
            shape=Controller.Shape.BOX,
            color=Controller.Color.YELLOW,
            normal=normal,
            size=size,
            offset=[0,5,0])
        cmds.addAttr(self.systemGroup, ln="stretch", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="autoSpine", at="float", k=True, min=0, max=1, dv=1)
        #mid
        self.ikSpine = Controller.create(
            name="mid_IK",
            parent=self.ikHips,
            shape=Controller.Shape.PLANE,
            color=Controller.Color.YELLOW,
            normal=normal,
            size=size)
        #pevlis
        self.ikPelvis = Controller.create(
            name="pelvis_IK",
            parent=self.ikHips,
            shape=Controller.Shape.BOX,
            color=Controller.Color.YELLOW,
            normal=normal,
            size=size,
            offset=[0,-5,0])
        #-- POSITION CONTROLLERS
        cmds.matchTransform(self.ikHips, boneList[0], pos=True, rot=False, scl=False)
        cmds.xform(self.ikPelvis, ws=True, t=midpoint)
        cmds.xform(self.ikChest, ws=True, t=midpoint)
        cmds.xform(self.ikSpine, ws=True, t=midpoint)
        RigUtility.bakeOffsetParentMatrix(self.ikHips)
        RigUtility.bakeOffsetParentMatrix(self.ikChest)
        RigUtility.bakeOffsetParentMatrix(self.ikSpine)
        RigUtility.bakeOffsetParentMatrix(self.ikPelvis)
        RigUtility.parentByMatrix(self.ikHips, self.rootSpace, mo=True)
        #-- BUILD CONTRAINTS
        cmds.parentConstraint(self.ikChest, control_joint_chest, mo=True)
        cmds.parentConstraint(self.ikSpine, control_joint_spine, mo=True)
        cmds.parentConstraint(self.ikPelvis, control_joint_hips, mo=True)
        #-- IK SPLINE
        ik_data = cmds.ikHandle(sj=splineBoneList[0], ee=splineBoneList[-1], sol="ikSplineSolver", numSpans=2, n="ik_spline_handle")
        control_curve = ik_data[-1]
        ikhandle = cmds.parent("|"+ik_data[0], self.systemGroup)[-1]
        control_curve = cmds.rename(control_curve, "ik_splineCurve")
        curveInfo = cmds.createNode("curveInfo")
        cmds.connectAttr(control_curve+".worldSpace[0]", curveInfo+".inputCurve")
        curve_length = cmds.getAttr(curveInfo+".arcLength")
        floatMath = cmds.createNode("floatMath")
        cmds.setAttr(floatMath+".floatB", curve_length)
        cmds.connectAttr(curveInfo+".arcLength", floatMath+".floatA")
        cmds.setAttr(floatMath+".operation", 3)
        lerpNode = UtilityNodes.createLerpNode()
        cmds.setAttr(lerpNode+".valueA", 1)
        cmds.connectAttr(floatMath+".outFloat", lerpNode+".valueB")
        cmds.connectAttr(self.systemGroup+".stretch", lerpNode+".weight")
        for splineBone in splineBoneList:
            cmds.connectAttr(lerpNode+".output", splineBone+".scaleX")
        #-- SKIN THE CONTROL CURVE TO THE CONTROL JOINTS
        cmds.skinCluster(control_curve, control_joint_hips, control_joint_spine, control_joint_chest)
        for i in range(0, len(ikBones)):
            if i == 0:
                cmds.parentConstraint(self.ikPelvis, ikBones[i], mo=True)
            elif ikBones[i] == ikBones[-1]:
                cmds.pointConstraint(splineBoneList[-1], ikBones[i], mo=True)
                cmds.orientConstraint(self.ikChest, ikBones[i], mo=True)
            else:
                if resolution_scale == 1:
                    cmds.parentConstraint(splineBoneList[i], ikBones[i], mo=True)
                else:
                    nearestList = RigUtility.listNearestTransforms(ikBones[i], splineBoneList)
                    cmds.parentConstraint(nearestList[0], ikBones[i], mo=True)
                    cmds.parentConstraint(nearestList[1], ikBones[i], mo=True)
        #-- IK SPLINE TWIST
        cmds.setAttr(ikhandle+".dTwistControlEnable", 1)
        cmds.setAttr(ikhandle+".dWorldUpType", 4)
        cmds.setAttr(ikhandle+".dForwardAxis", 0)
        cmds.setAttr(ikhandle+".dWorldUpAxis", 4)
        cmds.setAttr(ikhandle+".dWorldUpVectorX", 0)
        cmds.setAttr(ikhandle+".dWorldUpVectorY", 0)
        cmds.setAttr(ikhandle+".dWorldUpVectorZ", -1)
        cmds.setAttr(ikhandle+".dWorldUpVectorEndX", 0)
        cmds.setAttr(ikhandle+".dWorldUpVectorEndY", 0)
        cmds.setAttr(ikhandle+".dWorldUpVectorEndZ", -1)
        cmds.setAttr(ikhandle+".dTwistValueType", 0)
        cmds.connectAttr(control_joint_hips+".worldMatrix[0]", ikhandle+".dWorldUpMatrix")
        cmds.connectAttr(control_joint_chest+".worldMatrix[0]", ikhandle+".dWorldUpMatrixEnd")
        #-- AUTO SPINE POSITION (AVERAGE HIPS & CHEST)
        autoTorsoFree = cmds.createNode("transform", n="autoTorsoFree", p=self.systemGroup)
        autoTorsoFollow = cmds.createNode("transform", n="ikMidMarker", p=self.systemGroup)
        cmds.matchTransform(autoTorsoFree, self.ikSpine)
        cmds.matchTransform(autoTorsoFollow, self.ikSpine)
        cmds.parentConstraint(self.ikChest, autoTorsoFollow, mo=True)
        cmds.parentConstraint(self.ikPelvis, autoTorsoFollow, mo=True)
        cmds.parentConstraint(self.ikHips, autoTorsoFree, mo=True)
        #Free
        autoTorsoFreeMatrix = cmds.createNode("multMatrix", n="autoTorsoFreeMatrix")
        cmds.connectAttr(autoTorsoFree+".worldMatrix[0]", autoTorsoFreeMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikHips+".worldInverseMatrix[0]", autoTorsoFreeMatrix+".matrixIn[1]")
        #Follow
        autoTorsoFollowMatrix = cmds.createNode("multMatrix", n="autoTorsoFreeMatrix")
        cmds.connectAttr(autoTorsoFollow+".worldMatrix[0]", autoTorsoFollowMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikHips+".worldInverseMatrix[0]", autoTorsoFollowMatrix+".matrixIn[1]")
        #Blend between the two
        blendMatrix = cmds.createNode("blendMatrix")
        cmds.connectAttr(autoTorsoFreeMatrix+".matrixSum", blendMatrix+".inputMatrix")
        cmds.connectAttr(autoTorsoFollowMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".autoSpine", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.ikSpine+".offsetParentMatrix", f=True)
        self.ikMidMarker = autoTorsoFollow
        #
        self.ikHipsMarker = Marker.create(n="ikHipsMarker", p=self.systemGroup)
        self.ikPelvisMarker = Marker.create(n="ikPelvis", p=self.systemGroup)
        self.ikChestMarker = Marker.create(n="ikChestMarker", p=self.systemGroup)
        Marker.attach(self.ikHipsMarker, self.bonePelvis)
        Marker.attach(self.ikPelvisMarker, self.bonePelvis)
        Marker.attach(self.ikChestMarker, self.boneSpine3)
        cmds.matchTransform(self.ikHipsMarker, self.ikHips)
        cmds.matchTransform(self.ikPelvisMarker, self.ikPelvis)
        cmds.matchTransform(self.ikChestMarker, self.ikChest)
        #
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
        '''IK GROUP'''
        cmds.text(l="  IK", width=350, align="left", bgc=(.4,.4,.4))
        #IK Pole Vector Follow
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikStretch", label="IK Stretch", labelWidth=200, fieldWidth=150)
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikAutoSpine", label="IK Auto Spine", labelWidth=200, fieldWidth=150, changeCommand=self.setIKAutoSpine, keyCommand=self.keyIKAutoSpine)
        cmds.separator( height=15)
        '''FK Group'''
        cmds.text(l="  FK", width=350, align="left", bgc=(.4,.4,.4))
        #FK Space Switches
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".fkSpine3Space", label="FK Spine3 Parent Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKSpine3Space, keyCommand=self.keyFKSpine3Space)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".fkSpine2Space", label="FK Spine2 Parent Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKSpine2Space, keyCommand=self.keyFKSpine2Space)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".fkSpine1Space", label="FK Spine1 Parent Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKSpine1Space, keyCommand=self.keyFKSpine1Space)
        #Widgets.WAttributeToggle.Create(self.controlGroup+".fkPelvisSpace", label="FK Pelvis Parent Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKPelvisSpace, keyCommand=self.keyFKPelvisSpace)
        cmds.setParent('..')


    def sync(self, *args, **kwargs):
        usePositionValues = 0
        orgIKValue = cmds.getAttr(self.systemGroup+".ikMode")
        node = kwargs.get("node", None)
        if not node:
            node = self.node
        # Get Markers
        markerPelvis = RigNode.getPlug(node, "markerPelvis")
        markerSpine1 = RigNode.getPlug(node, "markerSpine1")
        markerSpine2 = RigNode.getPlug(node, "markerSpine2")
        markerSpine3 = RigNode.getPlug(node, "markerSpine3")
        # Height Scale
        heightScale = 1
        if node != self.node:
            if cmds.objExists("{}.restPosition".format(self.markerPelvis)) and cmds.objExists("{}.restPosition".format(markerPelvis)):
                localMarkerPelvis = self.markerPelvis
                heightExternal = cmds.getAttr("{}.restPositionY".format(markerPelvis))
                heightInternal = cmds.getAttr("{}.restPositionY".format(localMarkerPelvis))
                heightScale = heightInternal / heightExternal
        # ----- FK -----
        cmds.setAttr(self.controlGroup+".ikMode", 0)
        cmds.matchTransform(self.fkHips, markerPelvis, position=0, rotation=1, scale=0)
        hipsPosition = cmds.xform(markerPelvis, q=True, ws=True,t=True)
        hipsPosition[1] = hipsPosition[1] * heightScale
        cmds.xform(self.fkHips, ws=True, t=hipsPosition)
        cmds.xform(self.fkPelvis, os=True, t=[0,0,0], ro=[0,0,0])
        #cmds.matchTransform(self.fkPelvis, markerPelvis, position=1, rotation=1, scale=0)
        cmds.matchTransform(self.fkSpine1, markerSpine1, position=usePositionValues, rotation=1, scale=0)
        cmds.matchTransform(self.fkSpine2, markerSpine2, position=usePositionValues, rotation=1, scale=0)
        cmds.matchTransform(self.fkSpine3, markerSpine3, position=usePositionValues, rotation=1, scale=0)
        # ----- IK -----
        #cmds.setAttr(self.controlGroup+".ikMode", 1)
        cmds.matchTransform(self.ikHips, self.ikHipsMarker, position=1, rotation=0, scale=0)
        cmds.matchTransform(self.ikPelvis, self.ikPelvisMarker, position=1, rotation=1, scale=0)
        cmds.matchTransform(self.ikChest, self.ikChestMarker, position=1, rotation=1, scale=0)
        cmds.matchTransform(self.ikSpine, self.ikMidMarker, position=1, rotation=1, scale=0)
        cmds.setAttr(self.systemGroup+".ikMode", orgIKValue)


    def setIKMode(self, value, *args):
        self.sync()
        cmds.setAttr(self.controlGroup+".ikMode", value)


    def setFKSpine3Space(self, value, *args):
        worldMatrix = cmds.xform(self.fkSpine3, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpine3Space", value)
        cmds.xform(self.fkSpine3, ws=True, m=worldMatrix)


    def setFKSpine2Space(self, value, *args):
        worldMatrix = cmds.xform(self.fkSpine2, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpine2Space", value)
        cmds.xform(self.fkSpine2, ws=True, m=worldMatrix)


    def setFKSpine1Space(self, value, *args):
        worldMatrix = cmds.xform(self.fkSpine1, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkSpine1Space", value)
        cmds.xform(self.fkSpine1, ws=True, m=worldMatrix)


    def setFKPelvisSpace(self, value, *args):
        worldMatrix = cmds.xform(self.fkSpine1, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".fkPelvisSpace", value)
        cmds.xform(self.fkSpine1, ws=True, m=worldMatrix)


    def setIKAutoSpine(self, value, *args):
        worldMatrix = cmds.xform(self.ikSpine, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikAutoSpine", value)
        cmds.xform(self.ikSpine, ws=True, m=worldMatrix)


    def keyIKMode(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikMode")
        cmds.setKeyframe(self.ikHips)
        cmds.setKeyframe(self.ikPelvis)
        cmds.setKeyframe(self.ikSpine)
        cmds.setKeyframe(self.ikChest)
        cmds.setKeyframe(self.fkHips)
        cmds.setKeyframe(self.fkPelvis)
        cmds.setKeyframe(self.fkSpine1)
        cmds.setKeyframe(self.fkSpine2)
        cmds.setKeyframe(self.fkSpine3)


    def keyIKAutoSpine(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikAutoSpine")
        cmds.setKeyframe(self.ikSpine)


    def keyFKPelvisSpace(self, *args):
        cmds.setKeyframe(self.controlGroup+".fkPelvisSpace")
        cmds.setKeyframe(self.fkSpine1)


    def keyFKSpine1Space(self, *args):
        cmds.setKeyframe(self.controlGroup+".fkSpine1Space")
        cmds.setKeyframe(self.fkSpine1)


    def keyFKSpine2Space(self, *args):
        cmds.setKeyframe(self.controlGroup+".fkSpine2Space")
        cmds.setKeyframe(self.fkSpine2)


    def keyFKSpine3Space(self, *args):
        cmds.setKeyframe(self.controlGroup+".fkSpine3Space")
        cmds.setKeyframe(self.fkSpine3)


    def reset(self, *args):
        #reset attributes
        cmds.setAttr(self.controlGroup+".fkSpine1Space", 0)
        cmds.setAttr(self.controlGroup+".fkSpine2Space", 0)
        cmds.setAttr(self.controlGroup+".fkSpine3Space", 0)
        cmds.setAttr(self.controlGroup+".ikMode", 1)
        cmds.setAttr(self.controlGroup+".ikStretch", 0)
        cmds.setAttr(self.controlGroup+".ikAutoSpine", 0)
        #reset control list
        controlList = [self.ikHips, self.ikPelvis, self.ikSpine, self.ikChest, self.fkHips, self.fkPelvis, self.fkSpine1, self.fkSpine2, self.fkSpine3 ]
        for control in controlList:
            cmds.xform(control, os=True, t=[0,0,0], ro=[0,0,0])


    def keyAll(self, *args):
        controlList = [self.ikHips, self.ikPelvis, self.ikSpine, self.ikChest, self.fkHips, self.fkPelvis, self.fkSpine1, self.fkSpine2, self.fkSpine3 ]
        for control in controlList:
            cmds.setKeyframe(control)
        attrList = ["fkSpine1Space", "fkSpine2Space", "fkSpine3Space", "ikMode", "ikStretch", "ikAutoSpine"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)



