from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UtilityNodes as UtilityNodes
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase
from collections import OrderedDict
import json

RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Arm"
CLASS_NAME = "Arm"
CONTROLLER_SIZE = [15, 15, 15]


class Arm(ControlRigBase):

    def __init__(self, name="Arm", **kwargs):
        #standard
        self.node = None
        self.instanceName = name
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
        #Bones
        self.boneUpperArm = None
        self.boneLowerArm = None
        self.boneHand = None
        #Marker
        self.markerUpperArm = None
        self.markerLowerArm = None
        self.markerHand = None
        #FK
        self.fkSpace = None
        self.fkUpperArm = None
        self.fkLowerArm = None
        self.fkHand = None
        #IK
        self.ikSpace = None
        self.ikHand = None
        self.ikPoleVector = None
        self.ikHandMarker = None
        #freeCtrl
        self.controllerFree = None


    @staticmethod
    def load(node):
        instance = Arm()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        #
        instance.side = cmds.getAttr(node+".side")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        #Bone
        instance.boneUpperArm = RigNode.getPlug(node, "boneUpperArm")
        instance.boneLowerArm = RigNode.getPlug(node, "boneLowerArm")
        instance.boneHand = RigNode.getPlug(node, "boneHand")
        #Marker
        instance.markerUpperArm = RigNode.getPlug(node, "markerUpperArm")
        instance.markerLowerArm = RigNode.getPlug(node, "markerLowerArm")
        instance.markerHand = RigNode.getPlug(node, "markerHand")
        #FK
        instance.fkSpace = RigNode.getPlug(node, "fkSpace")
        instance.fkUpperArm = RigNode.getPlug(node, "fkUpperArm")
        instance.fkLowerArm = RigNode.getPlug(node, "fkLowerArm")
        instance.fkHand = RigNode.getPlug(node, "fkHand")
        #IK
        instance.ikSpace = RigNode.getPlug(node, "ikSpace")
        instance.ikHand = RigNode.getPlug(node, "ikHand")
        instance.ikPoleVector = RigNode.getPlug(node, "ikPoleVector")
        instance.ikHandMarker = RigNode.getPlug(node, "ikHandMarker")
        # freeCtrl
        instance.controllerFree = RigNode.getPlug(node, "controllerFree")
        #SpaceMap
        spaceString = cmds.getAttr(f"{node}.spaces")
        if not spaceString:
            spaceString = "{}"
        #dump it first to deal with old single quotes formatting
        spaceString = json.dumps(spaceString)
        instance.spaces = json.loads(spaceString, object_pairs_hook=OrderedDict)
        #
        return instance


    def setBoneList(self, boneList):
        self.boneUpperArm = boneList[0]
        self.boneLowerArm = boneList[1]
        self.boneHand = boneList[2]


    def getBoneList(self):
        return [self.boneUpperArm, self.boneLowerArm, self.boneHand]


    def getBoneNames(self):
        return ["boneUpperArm", "boneLowerArm", "boneHand"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkUpperArm,
            self.fkLowerArm,
            self.fkHand,
            self.ikHand,
            self.ikPoleVector,
            self.controllerFree
            ]

    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkUpperArm",
            "fkLowerArm",
            "fkHand",
            "ikHand",
            "ikPoleVector",
            "free"
            ]

    def getMarkerList(self):
        return [self.markerUpperArm, self.markerLowerArm, self.markerHand]


    def getMarkerNames(self):
        return ["markerUpperArm", "markerLowerArm", "markerHand"]


    def createNode(self):
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneUpperArm", "message")
        nodeBuilder.addAttr("boneLowerArm", "message")
        nodeBuilder.addAttr("boneHand", "message")
        nodeBuilder.addAttr("markerUpperArm", "message")
        nodeBuilder.addAttr("markerLowerArm", "message")
        nodeBuilder.addAttr("markerHand", "message")
        nodeBuilder.addAttr("fkSpace", "message")
        nodeBuilder.addAttr("fkUpperArm", "message")
        nodeBuilder.addAttr("fkLowerArm", "message")
        nodeBuilder.addAttr("fkHand", "message")
        nodeBuilder.addAttr("ikSpace", "message")
        nodeBuilder.addAttr("ikHand", "message")
        nodeBuilder.addAttr("ikPoleVector", "message")
        nodeBuilder.addAttr("ikHandMarker", "message")
        nodeBuilder.addAttr("controllerFree", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)


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
        # RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        # Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerUpperArm = Marker.create(n="markerUpperArm", t=self.boneUpperArm, p=self.markerGroup)
        self.markerLowerArm = Marker.create(n="markerLowerArm", t=self.boneLowerArm, p=self.markerGroup)
        self.markerHand = Marker.create(n="markerHand", t=self.boneHand, p=self.markerGroup)
        # Auto Orient Markers
        aimAxis = [1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [0,1,0]
        if self.side == 2:
            aimAxis = [-1,0,0]
            upAxis = [0,0,-1]
        endPoint = cmds.xform(self.markerHand, q=True, ws=True, rp=True)
        endPoint = [endPoint[0]+aimAxis[0], endPoint[1], endPoint[2]]
        Marker.orient(self.markerUpperArm, target=self.markerLowerArm, aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        Marker.orient(self.markerLowerArm, target=self.markerHand, aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        Marker.orient(self.markerHand, targetPoint=endPoint, aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneUpperArm", self.boneUpperArm)
        RigNode.setPlug(self.node, "boneLowerArm", self.boneLowerArm)
        RigNode.setPlug(self.node, "boneHand", self.boneHand)
        RigNode.setPlug(self.node, "markerUpperArm", self.markerUpperArm)
        RigNode.setPlug(self.node, "markerLowerArm", self.markerLowerArm)
        RigNode.setPlug(self.node, "markerHand", self.markerHand)
        RigNode.setPlug(self.node, "controllerFree", self.controllerFree)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})

        #test for propTwoHand and update the space
        #we do this bc the control space did not exist until the propTwoHand module was built
        for i, space in enumerate(self.spaces):
            if 'propTwoHand' in space[0]:
                characterRN = RigNode.getRelated(self.node)
                mod = RigNode.getPlug(characterRN, "propTwoHand")
                # bonePropFollow = RigNode.getPlug(mod, "bonePropFollow")
                # self.spaces[i] = ('propTwoHand', bonePropFollow)
                controllerProp = RigNode.getPlug(mod, "controllerProp")
                self.spaces[i] = ('propTwoHand', controllerProp)

        # add freeSpace ctrl and space
        self.controllerFree = Controller.create(
            name=Controller.buildName('free', self.side, "CTRL"),
            parent=self.controlGroup,
            shape=Controller.Shape.SPHERE,
            color=Controller.Color.BLUE,
            size=[10,10,10])
        cmds.matchTransform(self.controllerFree, self.boneHand)
        RigUtility.bakeOffsetParentMatrix(self.controllerFree)
        #self.spaces.append(('free', self.controllerFree))
        spaces = json.loads(self.spaces)
        spaces["free"] = self.controllerFree
        self.spaces = json.dumps(spaces)
        # default to hidden
        cmds.addAttr(self.controlGroup, ln='freeCtrlVis', at='long', min=0, max=1, dv=0)
        cmds.setAttr(self.controlGroup + '.freeCtrlVis', e=1, k=1)
        cmds.connectAttr(self.controlGroup + '.freeCtrlVis', self.controllerFree + '.visibility')

        RigNode.setPlug(self.node, "controllerFree", self.controllerFree)

        boneList = [self.boneUpperArm, self.boneLowerArm, self.boneHand]
        # Build Rigs
        fkBones = self.setupArmFK()
        ikBones = self.setupArmIK()
        # WEIGHT SWITCH
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
        # Toggle Visibility Based On Context
        #- FK
        FKControls = [self.fkUpperArm, self.fkLowerArm, self.fkHand]
        for ctrl in FKControls:
            cmds.connectAttr(weightSwitch+".outputInverse", ctrl+".v", f=True)
            RigUtility.modifyTransformChannels(ctrl, r=False)
        #- IK
        IKControls = [self.ikHand, self.ikPoleVector]
        for ctrl in IKControls:
            cmds.connectAttr(weightSwitch+".output", ctrl+".v", f=True)
            RigUtility.modifyTransformChannels(ctrl, t=False, r=False)
        # Proxy Attr
        cmds.addAttr(self.controlGroup, ln="ikMode", proxy=self.systemGroup+".ikMode")
        # | FK
        cmds.addAttr(self.controlGroup, ln="FK", at="float", k=True)
        cmds.setAttr(self.controlGroup+".FK", l=True)
        cmds.addAttr(self.controlGroup, ln="fkSpace", proxy="{}.fkSpace".format(self.systemGroup))
        cmds.addAttr(self.controlGroup, ln="fkLimbScale", proxy=self.systemGroup+".fkLimbScale")
        # | IK
        cmds.addAttr(self.controlGroup, ln="IK", at="float", k=True)
        cmds.setAttr(self.controlGroup+".IK", l=True)
        cmds.addAttr(self.controlGroup, ln="ikSpace", proxy="{}.ikSpace".format(self.systemGroup))
        cmds.addAttr(self.controlGroup, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        cmds.addAttr(self.controlGroup, ln="ikLimbScale", proxy=self.systemGroup+".ikLimbScale")
        cmds.addAttr(self.controlGroup, ln="ikStretch", proxy=self.systemGroup+".ikStretch")
        cmds.addAttr(self.controlGroup, ln="ikSquash", proxy=self.systemGroup+".ikSquash")
        cmds.addAttr(self.controlGroup, ln="ikSquashScale", proxy=self.systemGroup+".ikSquashScale")
        # |- mirror to ik hand control
        cmds.addAttr(self.ikHand, ln="ikSpace", proxy="{}.ikSpace".format(self.systemGroup))
        cmds.addAttr(self.ikHand, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        # |- mirror to ik polevector control
        cmds.addAttr(self.ikPoleVector, ln="ikPoleVectorFollow", proxy=self.systemGroup+".ikPoleVectorFollow")
        # |- mirror to fk upperarm control
        cmds.addAttr(self.fkUpperArm, ln="fkSpace", proxy="{}.fkSpace".format(self.systemGroup))

        self.setupArmTwist()
        
        #Set upperarm bones rotate order to zxy (3p and 1p)
        cmds.setAttr(self.boneUpperArm + '.rotateOrder', 2)

        # Hide SystemGroup
        RigUtility.modifyTransformChannels(self.fkSpace)
        RigUtility.modifyTransformChannels(self.ikSpace)
        RigUtility.modifyTransformChannels(self.controlGroup)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Lock unused channels
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        RigNode.setPlug(self.node, "fkSpace", self.fkSpace)
        RigNode.setPlug(self.node, "fkUpperArm", self.fkUpperArm)
        RigNode.setPlug(self.node, "fkLowerArm", self.fkLowerArm)
        RigNode.setPlug(self.node, "fkHand", self.fkHand)
        RigNode.setPlug(self.node, "ikSpace", self.ikSpace)
        RigNode.setPlug(self.node, "ikHand", self.ikHand)
        RigNode.setPlug(self.node, "ikPoleVector", self.ikPoleVector)
        RigNode.setPlug(self.node, "ikHandMarker", self.ikHandMarker)
        # Set Rigged Status
        self.rigged = True


    def setupArmFK(self):
        markerList = [self.markerUpperArm, self.markerLowerArm, self.markerHand]
        boneShoulder = RigUtility.firstParentOf(self.boneUpperArm)
        boneList = [self.boneUpperArm, self.boneLowerArm, self.boneHand]
        fkBones = RigUtility.cloneJointChain(boneList, "fkbone_", self.systemGroup)
        #-FK Space
        self.fkSpace = RigUtility.createLocalSpace("FK", self.markerUpperArm, self.controlGroup)
        spaceSwitch = SpaceSwitch2.create(self.fkSpace, boneShoulder)

        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "fkSpace")
        #-UpperArm
        self.fkUpperArm = Controller.create(
            name = Controller.buildName("upperarm", self.side, "FK"),
            parent = self.fkSpace,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        cmds.matchTransform(self.fkUpperArm, markerList[0], position=1, rotation=1, scale=0)
        RigUtility.bakeOffsetParentMatrix(self.fkUpperArm)
        #-LowerArm
        self.fkLowerArm = Controller.create(
            name = Controller.buildName("lowerarm", self.side, "FK"),
            parent = self.fkUpperArm,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        cmds.matchTransform(self.fkLowerArm, markerList[1], position=1, rotation=1, scale=0)
        RigUtility.bakeOffsetParentMatrix(self.fkLowerArm)
        #-Hand
        self.fkHand = Controller.create(
            name = Controller.buildName("hand", self.side, "FK"),
            parent = self.fkLowerArm,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE
            )
        cmds.matchTransform(self.fkHand, markerList[2], position=1, rotation=1, scale=0)
        RigUtility.bakeOffsetParentMatrix(self.fkHand)
        #Build Constraints
        cmds.pointConstraint(self.fkUpperArm, fkBones[0], mo=True)
        cmds.orientConstraint(self.fkUpperArm, fkBones[0], mo=True)
        cmds.orientConstraint(self.fkLowerArm, fkBones[1], mo=True)
        cmds.orientConstraint(self.fkHand, fkBones[2], mo=True)
        # ----------------- Stretch (FK) --------------------
        cmds.addAttr(self.systemGroup, ln="fkLimbScale", k=True, at="float", dv=1, min=.01)
        RigUtility.setupFKStretch(self.fkLowerArm, self.systemGroup+".fkLimbScale")
        RigUtility.setupFKStretch(self.fkHand, self.systemGroup+".fkLimbScale")
        #
        cmds.connectAttr(self.systemGroup+".fkLimbScale", fkBones[0]+".scaleX")
        cmds.connectAttr(self.systemGroup+".fkLimbScale", fkBones[1]+".scaleX")
        #
        return fkBones


    def setupArmIK(self):
        markerList = [self.markerUpperArm, self.markerLowerArm, self.markerHand]
        boneList = [self.boneUpperArm, self.boneLowerArm, self.boneHand]
        ikBones = RigUtility.cloneJointChain(boneList, "ikbone_", self.systemGroup)
        # if chain is straight clone the preferredAngle from the source
        if (RigUtility.isChainStraight(ikBones[0], ikBones[1], ikBones[2])):
            prefAngle = cmds.getAttr(f"{self.boneLowerArm}.preferredAngle")[0]
            cmds.setAttr(f"{ikBones[1]}.preferredAngle", prefAngle[0], prefAngle[1], prefAngle[2])
        #- Create Local Space - IK
        self.ikSpace = RigUtility.createLocalSpace("IK", self.boneUpperArm, self.controlGroup)
        #|- Control:PoleVector
        self.ikPoleVector = Controller.create(
            name = Controller.buildName("elbow", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.YELLOW,
            size = [CONTROLLER_SIZE[0] * .25, CONTROLLER_SIZE[1] * .25, CONTROLLER_SIZE[2] * .25]
            )
        #|- Control:Hand
        hand_offset = [CONTROLLER_SIZE[0] * .5,0,0]
        if self.side == 2:
            hand_offset[0] *= -1
        self.ikHand = Controller.create(
            name = Controller.buildName("hand", self.side, "IK"),
            parent = self.ikSpace,
            shape = Controller.Shape.PLANE,
            color = Controller.Color.YELLOW,
            size = CONTROLLER_SIZE,
            normal = [0,0,1],
            offset = hand_offset
            )
        #|-set position (poleVector)
        pos = RigUtility.calculatePoleVector(markerList[0], markerList[1], markerList[2], defaultVectorDirection=[0,0,-1])
        cmds.xform(self.ikPoleVector, ws=True, t=pos, ro=[0,0,0])
        RigUtility.bakeOffsetParentMatrix(self.ikPoleVector)
        #|- set position (hand)
        cmds.matchTransform(self.ikHand, self.markerHand, position=True, rotation=True, scale=False)
        RigUtility.bakeOffsetParentMatrix(self.ikHand)
        #| - SpaceSwitch
        spaceSwitch = SpaceSwitch2.create(self.ikSpace, None)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace, True)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "ikSpace")
        #|- setup IK Hand Marker
        self.ikHandMarker = cmds.createNode(
            "transform",
            p=self.systemGroup,
            n=RigUtility.shortNameOf(self.ikHand)+"_marker")
        cmds.matchTransform(self.ikHandMarker, self.ikHand)
        cmds.parentConstraint(self.boneHand, self.ikHandMarker, mo=True)
        #-setup PoleVector Following
        cmds.addAttr(self.systemGroup, ln="ikPoleVectorFollow", at="float", k=True, min=0, max=1, dv=0)
        opm = cmds.getAttr(self.ikPoleVector+".offsetParentMatrix")
        opmNode = RigUtility.convertMatrixToNode(opm)
        blendMatrix = cmds.createNode("blendMatrix")
        pvMarker = cmds.createNode("transform", n="ik_elbow_marker", p=self.systemGroup)
        cmds.matchTransform(pvMarker, self.ikPoleVector)
        cmds.parentConstraint(self.ikHand, pvMarker, mo=True)
        multMatrix = cmds.createNode("multMatrix")
        cmds.connectAttr(pvMarker+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        cmds.connectAttr(self.ikSpace+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        cmds.connectAttr(opmNode+".matrixSum", blendMatrix+".inputMatrix")
        cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        cmds.connectAttr(self.systemGroup+".ikPoleVectorFollow", blendMatrix+".envelope")
        cmds.connectAttr(blendMatrix+".outputMatrix", self.ikPoleVector+".offsetParentMatrix")
        #IK HANDLE
        ikhandle = cmds.ikHandle(sj=ikBones[0], ee=ikBones[2], solver="ikRPsolver")[0]
        ikhandle = cmds.parent(ikhandle, self.systemGroup)[0]
        #CONSTRAINTS
        boneParent = RigUtility.firstParentOf(self.boneUpperArm)
        if boneParent != None:
            cmds.parentConstraint(boneParent, ikBones[0], mo=True)
        cmds.poleVectorConstraint(self.ikPoleVector, ikhandle)
        cmds.pointConstraint(self.ikHand, ikhandle, mo=True)
        cmds.orientConstraint(self.ikHand, ikBones[2], mo=True)
        #-------------------- Stretch (IK) --------------------------
        cmds.addAttr(self.systemGroup, ln="ikStretch", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikLimbScale", at="float", k=True, min=0.1, dv=1)
        cmds.addAttr(self.systemGroup, ln="ikSquash", at="float", k=True, min=0, max=1, dv=0)
        cmds.addAttr(self.systemGroup, ln="ikSquashScale", at="float", k=True, min=0.01, dv=1)
        #|-calculate default length of chain
        lengthA = RigUtility.calculateDistanceBetweenTransforms(ikBones[0], ikBones[1])
        lengthB = RigUtility.calculateDistanceBetweenTransforms(ikBones[1], ikBones[2])
        defaultLength = lengthA + lengthB
        #|-create distance node to get the 'current' length of chain
        distanceNode = UtilityNodes.createDistanceNode(n=self.instanceName+"_IK_currentDistance", transformA=ikBones[0], transformB=self.ikHand, p=self.systemGroup)
        #|-wire up the stretchNode
        stretchNode = UtilityNodes.createStretchNode(n=self.instanceName+"_IK_stretchNode")
        cmds.setAttr(stretchNode+".defaultLength", defaultLength)
        cmds.connectAttr(distanceNode+".distance", stretchNode+".currentLength")
        cmds.connectAttr(self.systemGroup+".ikStretch", stretchNode+".weight")
        #|-(Clamp the stretch affect to not 'shrink' the arm)
        stretchMaxNode = cmds.createNode("floatMath", n=self.instanceName+"_IK_stretchFactorMaxNode")
        cmds.setAttr(stretchMaxNode+".operation", 5)
        cmds.connectAttr(self.systemGroup+".ikLimbScale", stretchMaxNode+".floatA")
        cmds.connectAttr(stretchNode+".stretchFactor", stretchMaxNode+".floatB")
        #Squash
        squashNode = UtilityNodes.createSquashNode(n=self.instanceName+"_IK_squashNode")
        cmds.connectAttr(stretchNode+".stretchFactor", squashNode+".stretchFactor")
        cmds.connectAttr(self.systemGroup+".ikSquash", squashNode+".weight")
        #Volume
        squashScaleNode = cmds.createNode("floatMath", n=self.instanceName+"_IK_squashVolumeNode")
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
        #
        return ikBones

    def setupArmTwist(self):
        twistUpperBones = [bone for bone in cmds.listRelatives(self.boneUpperArm, type="joint") if 'twist' in bone]
        twistLowerBones = [bone for bone in cmds.listRelatives(self.boneLowerArm, type="joint") if 'twist' in bone]

        if twistUpperBones:
            #new group
            twistUpperGroup = cmds.createNode("transform", p=self.controlGroup, n="twistUpper")
            cmds.parentConstraint(self.boneUpperArm, twistUpperGroup)
            div = float(len(twistUpperBones) + 1)
            # twistUpperControls = []
            for i, bone in enumerate(twistUpperBones):
                controller = Controller.create(name = Controller.buildName(bone, None, "CTRL"),
                                                parent = twistUpperGroup,
                                                shape = Controller.Shape.CIRCLE,
                                                color = Controller.Color.CYAN,
                                                size = [CONTROLLER_SIZE[0] * .25, CONTROLLER_SIZE[1] * .25, CONTROLLER_SIZE[2] * .25]
                                                )
                cmds.matchTransform(controller, bone, position=1, rotation=1, scale=0)
                RigUtility.bakeOffsetParentMatrix(controller)
                # twistUpperControls.append(controller)

                #move with stretch
                multDivPos = cmds.createNode("multiplyDivide", n=controller+"_twistPos")
                cmds.setAttr(multDivPos+".input1", 1,1,1)
                cmds.connectAttr(self.boneUpperArm+".scaleX", multDivPos+".input1X")
                t = cmds.getAttr(bone+".translate")[0]
                cmds.setAttr(multDivPos+".input2", t[0], t[1], t[2])

                #rotation
                #shoulderNegDivDescending
                multDivRot = cmds.createNode("multiplyDivide", n=controller+"_twistRot")
                cmds.connectAttr(self.boneUpperArm+".rotateX", multDivRot+".input1X")
                cmds.setAttr(multDivRot+".input2X", -(1.0 - (i+1)/div))

                #compose matrix
                compMatrix = cmds.createNode("composeMatrix", n=controller+"_matrix")
                cmds.connectAttr(multDivPos+".output", compMatrix+".inputTranslate")
                cmds.connectAttr(multDivRot+".outputX", compMatrix+".inputRotate.inputRotateX")
                cmds.connectAttr(self.boneUpperArm+".scaleY", compMatrix+".inputScale.inputScaleY")
                cmds.connectAttr(self.boneUpperArm+".scaleZ", compMatrix+".inputScale.inputScaleZ")

                #feed matrix to controller offsetParentMatrix
                cmds.connectAttr(compMatrix+".outputMatrix", controller+".offsetParentMatrix")
                cmds.parentConstraint(controller, bone)
                cmds.scaleConstraint(controller, bone)

        if twistLowerBones:
            #new group
            twistLowerGroup = cmds.createNode("transform", p=self.controlGroup, n="twistLower")
            cmds.parentConstraint(self.boneLowerArm, twistLowerGroup)
            div = float(len(twistLowerBones))
            # twistLowerControls = []
            for i, bone in enumerate(twistLowerBones):
                controller = Controller.create(name = Controller.buildName(bone, None, "CTRL"),
                                                parent = twistLowerGroup,
                                                shape = Controller.Shape.CIRCLE,
                                                color = Controller.Color.CYAN,
                                                size = [CONTROLLER_SIZE[0] * .25, CONTROLLER_SIZE[1] * .25, CONTROLLER_SIZE[2] * .25]
                                                )
                cmds.matchTransform(controller, bone, position=1, rotation=1, scale=0)
                RigUtility.bakeOffsetParentMatrix(controller)
                # twistLowerControls.append(controller)

                #move with stretch
                multDivPos = cmds.createNode("multiplyDivide", n=controller+"_twistPos")
                cmds.setAttr(multDivPos+".input1", 1,1,1)
                cmds.connectAttr(self.boneLowerArm+".scaleX", multDivPos+".input1X")
                t = cmds.getAttr(bone+".translate")[0]
                cmds.setAttr(multDivPos+".input2", t[0], t[1], t[2])

                #rotation
                #wristDivAscending
                multDivRot = cmds.createNode("multiplyDivide", n=controller+"_twistRot")
                cmds.connectAttr(self.boneHand+".rotateX", multDivRot+".input1X")
                cmds.setAttr(multDivRot+".input2X", i/div)

                #compose matrix
                compMatrix = cmds.createNode("composeMatrix", n=controller+"_matrix")
                cmds.connectAttr(multDivPos+".output", compMatrix+".inputTranslate")
                cmds.connectAttr(multDivRot+".outputX", compMatrix+".inputRotate.inputRotateX")
                cmds.connectAttr(self.boneLowerArm+".scaleY", compMatrix+".inputScale.inputScaleY")
                cmds.connectAttr(self.boneLowerArm+".scaleZ", compMatrix+".inputScale.inputScaleZ")

                #feed matrix to controller offsetParentMatrix
                cmds.connectAttr(compMatrix+".outputMatrix", controller+".offsetParentMatrix")
                cmds.parentConstraint(controller, bone)
                cmds.scaleConstraint(controller, bone)

    #
    # Animation Control Functions
    #
    def gui(self, *args, **kwargs):
        cmds.columnLayout(co=("both", 5))
        '''Globals'''
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        cmds.text(l="  Global", width=350, align="left", bgc=(.4,.4,.4))
        #IK FK Switch Widget
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikMode", label="IK Mode", labelWidth=200, fieldWidth=150, buttonLabel0="FK", buttonLabel1="IK", changeCommand=self.setIKMode, keyCommand=self.keyIKMode)
        '''FK Group'''
        cmds.text(l="  FK", width=350, align="left", bgc=(.4,.4,.4))
        #FK Space Switch Widget
        fk_switchAttr = "{}.fkSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(fk_switchAttr, label="FK Space", labelWidth=200, fieldWidth=150, changeCommand=self.setFKSpace, keyCommand=self.keyFKSpace)
        #FK Attribute List
        Widgets.WAttributeField.Create(self.controlGroup+'.fkLimbScale', label="FK Limb Scale")
        cmds.separator( height=15)
        '''IK GROUP'''
        cmds.text(l="  IK", width=350, align="left", bgc=(.4,.4,.4))
        #IK Space Switch Widget
        ik_switchAttr = "{}.ikSpace".format(self.controlGroup)
        Widgets.WSpaceSwitch2.Create(ik_switchAttr, label="IK Space", labelWidth=200, fieldWidth=150, changeCommand=self.setIKSpace, keyCommand=self.keyIKSpace)
        #IK Pole Vector Follow
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikPoleVectorFollow", label="IK Elbow Follow", labelWidth=200, fieldWidth=150, changeCommand=self.setIKPoleVectorFollow, keyCommand=self.keyPoleVectorFollow)
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikStretch", label="IK Stretch", labelWidth=200, fieldWidth=150)
        Widgets.WAttributeToggle.Create(self.controlGroup+".ikSquash", label="IK Squash", labelWidth=200, fieldWidth=150)
        #IK Attr List
        ikAttrList = [
            ["IK Limb Scale", "ikLimbScale"],
            ["IK Squash Scale", "ikSquashScale"]]
        for attr in ikAttrList:
            label = attr[0]
            attributePath = "{0}.{1}".format(self.controlGroup, attr[1])
            Widgets.WAttributeField.Create(attributePath, label=label)
        cmds.setParent('..')


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
            RigNode.getPlug(node, "markerUpperArm"),
            RigNode.getPlug(node, "markerLowerArm"),
            RigNode.getPlug(node, "markerHand")]
        fkControls = [self.fkUpperArm, self.fkLowerArm, self.fkHand]
        for i in range(0,len(fkControls)):
            #cmds.matchTransform(fkControls[i], markerList[i], pos=0, rot=1, scl=0)  #causing 360
            cmds.xform(fkControls[i], ws=1, ro=(cmds.xform(markerList[i], q=1, ws=1, ro=1)))
        '''--- IK ---'''
        cmds.setAttr(self.systemGroup+".ikMode", 0)
        #|- IK Hand
        #cmds.matchTransform(self.ikHand, self.ikHandMarker, pos=1, rot=1, scl=0) #causing 360
        cmds.xform(self.ikHand, ws=1, ro=(cmds.xform(self.ikHandMarker, q=1, ws=1, ro=1)), t=(cmds.xform(self.ikHandMarker, q=1, ws=1, t=1)))
        #|- IK PoleVector
        markerList = [self.markerUpperArm, self.markerLowerArm, self.markerHand]
        pv = RigUtility.calculatePoleVector(markerList[0], markerList[1], markerList[2], defaultVectorDirection=[0,0,-1])
        cmds.xform(self.ikPoleVector, ws=1, t=pv)
        cmds.setAttr(self.systemGroup+".ikMode", orgIKValue)


    def reset(self, *args, **kwargs):
        #reset attributes
        cmds.setAttr(self.controlGroup+".fkLimbScale", 1)
        cmds.setAttr(self.controlGroup+".ikLimbScale", 1)
        #reset control list
        controlList = [self.ikHand, self.ikPoleVector, self.fkUpperArm, self.fkLowerArm, self.fkHand ]
        channelList = ["tx", "ty", "tz", "rx", "ry", "rz"]
        for control in controlList:
            for channel in channelList:
                attrPath = "{}.{}".format(control, channel)
                if cmds.getAttr(attrPath, l=True) == False:
                    cmds.setAttr(attrPath, 0)


    def keyAll(self, *args, **kwargs):
        controlList = [self.ikHand, self.ikPoleVector, self.fkUpperArm, self.fkLowerArm, self.fkHand]
        for control in controlList:
            cmds.setKeyframe(control)
        attrList = ["fkSpace", "fkLimbScale", "ikMode", "ikSpace", "ikLimbScale", "ikStretch",
            "ikSquash", "ikSquashScale", "ikPoleVectorFollow"]
        for attr in attrList:
            cmds.setKeyframe(self.controlGroup+"."+attr)


    def setIKMode(self, value, *args):
        self.sync()
        cmds.setAttr(self.controlGroup+".ikMode", value)


    def keyIKMode(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikMode")
        controlList = [self.fkUpperArm, self.fkLowerArm, self.fkHand, self.ikHand, self.ikPoleVector]
        for control in controlList:
            cmds.setKeyframe(control)


    def setFKSpace(self, index, *args):
        worldMatrix = cmds.xform(self.fkUpperArm, q=True, ws=True, m=True)
        cmds.setAttr("{}.fkSpace".format(self.controlGroup), index)
        cmds.xform(self.fkUpperArm, ws=True, m=worldMatrix)


    def keyFKSpace(self, *args):
        cmds.setKeyframe("{}.fkSpace".format(self.controlGroup))
        cmds.setKeyframe(self.fkUpperArm)


    def setIKSpace(self, index, *args):
        hand_worldMatrix = cmds.xform(self.ikHand, q=True, ws=True, m=True)
        elbow_worldMatrix = cmds.xform(self.ikPoleVector, q=True, ws=True, m=True)
        cmds.setAttr("{}.ikSpace".format(self.controlGroup), index)
        cmds.xform(self.ikHand, ws=True, m=hand_worldMatrix)
        cmds.xform(self.ikPoleVector, ws=True, m=elbow_worldMatrix)


    def keyIKSpace(self, *args):
        cmds.setKeyframe("{}.ikSpace".format(self.controlGroup))
        cmds.setKeyframe(self.ikHand)
        cmds.setKeyframe(self.ikPoleVector)


    def setIKPoleVectorFollow(self, value, *args):
        pv_worldMatrix = cmds.xform(self.ikPoleVector, q=True, ws=True, m=True)
        cmds.setAttr(self.controlGroup+".ikPoleVectorFollow", value)
        cmds.xform(self.ikPoleVector, ws=True, m=pv_worldMatrix)


    def keyPoleVectorFollow(self, *args):
        cmds.setKeyframe(self.controlGroup+".ikPoleVectorFollow")
        cmds.setKeyframe(self.ikPoleVector)