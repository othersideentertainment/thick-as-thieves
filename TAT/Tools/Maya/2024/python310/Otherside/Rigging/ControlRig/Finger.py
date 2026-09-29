from collections import OrderedDict
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Marker as Marker
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Finger"
CLASS_NAME = "Finger"
CONTROLLER_SIZE = [3, 3, 3]


class Finger(ControlRigBase):
    def __init__(self, name="Finger", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = kwargs.get("side", 0)
        self.fingerID = kwargs.get("fingerID", 0)
        #
        self.characterized = False
        self.rigged = False
        #
        self.controlGroup = None
        self.markerGroup = None
        self.systemGroup = None
        self.rootSpace = None
        #
        self.boneFingerA = None
        self.boneFingerB = None
        self.boneFingerC = None
        #
        self.markerFingerA = None
        self.markerFingerB = None
        self.markerFingerC = None
        #
        self.fkFingerA = None
        self.fkFingerB = None
        self.fkFingerC = None


    @staticmethod
    def load(node):
        instance = Finger()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.side = cmds.getAttr(node+".side")
        instance.fingerID = cmds.getAttr(node+".fingerID")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        instance.boneFingerA = RigNode.getPlug(node, "boneFingerA")
        instance.boneFingerB = RigNode.getPlug(node, "boneFingerB")
        instance.boneFingerC = RigNode.getPlug(node, "boneFingerC")
        instance.markerFingerA = RigNode.getPlug(node, "markerFingerA")
        instance.markerFingerB = RigNode.getPlug(node, "markerFingerB")
        instance.markerFingerC = RigNode.getPlug(node, "markerFingerC")
        instance.fkFingerA = RigNode.getPlug(node, "fkFingerA")
        instance.fkFingerB = RigNode.getPlug(node, "fkFingerB")
        instance.fkFingerC = RigNode.getPlug(node, "fkFingerC")
        return instance


    def getBoneList(self):
        return [self.boneFingerA, self.boneFingerB, self.boneFingerC]


    def setBoneList(self, boneList):
        self.boneFingerA = boneList[0]
        self.boneFingerB = boneList[1]
        self.boneFingerC = boneList[2]


    def getBoneNames(self):
        return ["boneFingerA", "boneFingerB", "boneFingerC"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.fkFingerA,
            self.fkFingerB,
            self.fkFingerC
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "fkFingerA",
            "fkFingerB",
            "fkFingerC"
            ]


    def getMarkerList(self):
        return [self.markerFingerA, self.markerFingerB, self.markerFingerC]


    def getMarkerNames(self):
        return ["markerFingerA", "markerFingerB", "markerFingerC"]



    def gui(self):
        cmds.columnLayout()
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        cmds.text("not_implemented")
        cmds.setParent('..')


    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("side", "long")
        nodeBuilder.addAttr("fingerID", "long")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("boneFingerA", "message")
        nodeBuilder.addAttr("boneFingerB", "message")
        nodeBuilder.addAttr("boneFingerC", "message")
        nodeBuilder.addAttr("markerFingerA", "message")
        nodeBuilder.addAttr("markerFingerB", "message")
        nodeBuilder.addAttr("markerFingerC", "message")
        nodeBuilder.addAttr("fkFingerA", "message")
        nodeBuilder.addAttr("fkFingerB", "message")
        nodeBuilder.addAttr("fkFingerC", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        cmds.setAttr(self.node+".side", self.side)
        cmds.setAttr(self.node+".fingerID", self.fingerID)


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
        #Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerFingerA = Marker.create(n="markerFingerA", t=self.boneFingerA, p=self.markerGroup)
        self.markerFingerB = Marker.create(n="markerFingerB", t=self.boneFingerB, p=self.markerGroup)
        self.markerFingerC = Marker.create(n="markerFingerC", t=self.boneFingerC, p=self.markerGroup)
        #Auto Orient Markers
        aimAxis = [1,0,0]
        aimAxisInvert = [-1,0,0]
        upAxis = [0,0,1]
        worldUpAxis = [0,1,0]
        if self.side == 2:
            aimAxis = [-1,0,0]
            aimAxisInvert = [1,0,0]
            upAxis = [0,0,-1]
        endPoint = cmds.xform(self.markerFingerC, q=True, ws=True, rp=True)
        endPoint = [endPoint[0]+aimAxis[0], endPoint[1], endPoint[2]]
        Marker.orient(self.markerFingerA, target=self.markerFingerB, aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        Marker.orient(self.markerFingerB, target=self.markerFingerC, aimAxis=aimAxis, upAxis=upAxis, worldAxis=worldUpAxis)
        Marker.orient(self.markerFingerC, target=self.markerFingerB, aimAxis=aimAxisInvert, upAxis=upAxis, worldAxis=worldUpAxis)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneFingerA", self.boneFingerA)
        RigNode.setPlug(self.node, "boneFingerB", self.boneFingerB)
        RigNode.setPlug(self.node, "boneFingerC", self.boneFingerC)
        RigNode.setPlug(self.node, "markerFingerA", self.markerFingerA)
        RigNode.setPlug(self.node, "markerFingerB", self.markerFingerB)
        RigNode.setPlug(self.node, "markerFingerC", self.markerFingerC)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        boneParent = RigUtility.firstParentOf(self.boneFingerA)
        boneList = [self.boneFingerA, self.boneFingerB, self.boneFingerC]
        fingerName = getFingerName(self.fingerID)
        # Setup Constraints to Parent Bone (If Applicable)

        if boneParent != None:
            pc = cmds.parentConstraint(boneParent, self.controlGroup, mo=True)
            cmds.parent(pc, self.systemGroup)

        '''Create Controllers'''
        fkBones = RigUtility.cloneJointChain(boneList, "fkbone_", self.systemGroup)
        #FKFingerA
        self.fkFingerA = Controller.create(
            name = Controller.buildName(fingerName+"_A", self.side, "FK"),
            parent = self.controlGroup,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        #FKFingerB
        self.fkFingerB = Controller.create(
            name = Controller.buildName(fingerName+"_B", self.side, "FK"),
            parent = self.fkFingerA,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)
        #FKFingerC
        self.fkFingerC= Controller.create(
            name = Controller.buildName(fingerName+"_C", self.side, "FK"),
            parent = self.fkFingerB,
            shape = Controller.Shape.CIRCLE,
            color = Controller.Color.CYAN,
            size = CONTROLLER_SIZE)




        #Add Curl Attributes
        cmds.addAttr(self.systemGroup, ln="curl", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.systemGroup, ln="curlAdd", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.systemGroup, ln="curlSettings", at="float", dv=0, k=True)
        cmds.setAttr(self.systemGroup+".curlSettings", l=True)
        if fingerName == "thumb":
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleA", at="float", k=True, dv=-45)
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleB", at="float", k=True, dv=-45)
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleC", at="float", k=True, dv=-75)
        else:
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleA", at="float", k=True, dv=-90)
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleB", at="float", k=True, dv=-90)
            cmds.addAttr(self.systemGroup, ln="curlMaxAngleC", at="float", k=True, dv=-90)
        cmds.addAttr(self.systemGroup, ln="poseSettings", at="float", dv=0, k=True)
        cmds.setAttr(self.systemGroup+".poseSettings", l=True)
        cmds.addAttr(self.systemGroup, ln="relaxPose", at="float", k=True, dv=.1)
        cmds.addAttr(self.systemGroup, ln="fistPose", at="float", k=True, dv=1)
        cmds.addAttr(self.systemGroup, ln="stretchSettings", at="float", dv=0, k=True)
        cmds.setAttr(self.systemGroup+".stretchSettings", l=True)
        cmds.addAttr(self.systemGroup, ln="stretchA", at="float", k=True, dv=1, min=.01)
        cmds.addAttr(self.systemGroup, ln="stretchB", at="float", k=True, dv=1, min=.01)
        cmds.addAttr(self.systemGroup, ln="stretchC", at="float", k=True, dv=1, min=.01)
        #Combine Curl + Curl Add
        curlSum = cmds.createNode("floatMath", n="curlSum")
        cmds.setAttr(curlSum+".operation", 0)
        cmds.connectAttr(self.systemGroup+".curl", curlSum+".floatA")
        cmds.connectAttr(self.systemGroup+".curlAdd", curlSum+".floatB")
        # Attach Curl to Each Knuckle
        controllerList = [self.fkFingerA, self.fkFingerB, self.fkFingerC]
        markerList = [self.markerFingerA, self.markerFingerB, self.markerFingerC]
        maxAngleList = ["curlMaxAngleA","curlMaxAngleB","curlMaxAngleC"]
        stretchFactorList = ["stretchA", "stretchB", "stretchC"]
        # Setup Controllers
        for i in range(0, len(controllerList)):
            #- Get List Elements
            bone = fkBones[i]
            marker = markerList[i]
            controller = controllerList[i]
            maxAngle = maxAngleList[i]
            #-Get target parents
            boneParent = RigUtility.firstParentOf(bone)
            controllerParent = RigUtility.firstParentOf(controller)
            #=== Setup Network
            input, output = createFingerNetwork(controller)
            #-connect inputs
            cmds.connectAttr(curlSum+".outFloat", input+".curlWeight")
            cmds.connectAttr(self.systemGroup+"."+maxAngle, input+".curlMaxAngle")
            cmds.connectAttr(controllerParent+".worldInverseMatrix[0]", input+".worldInverseMatrix", f=True)
            cmds.connectAttr(boneParent+".worldMatrix[0]", input+".worldMatrix", f=True)
            #-connect output
            cmds.connectAttr(output+".matrixSum", controller+".offsetParentMatrix")
            #- Set Offset Parent Matrix
            cmds.matchTransform(controller, marker, scl=False)
            opm = cmds.xform(controller, q=True, os=True, matrix=True)
            cmds.setAttr(input+".offsetParentMatrix", opm, type="matrix")
            cmds.xform(controller, os=True, t=[0,0,0], ro=[0,0,0])
            #- Setup Orient Constraint
            cmds.orientConstraint(controller, bone, mo=True)
            cmds.addAttr(controller, ln="stretch", proxy=self.systemGroup+"."+stretchFactorList[i])
            cmds.connectAttr(self.systemGroup+"."+stretchFactorList[i], bone+".scaleX")
        # Constrain Skeleton to FK Bones
        cmds.orientConstraint(fkBones[0], boneList[0], mo=True)
        cmds.orientConstraint(fkBones[1], boneList[1], mo=True)
        cmds.orientConstraint(fkBones[2], boneList[2], mo=True)
        cmds.connectAttr(fkBones[0]+".scale", self.boneFingerA+".scale")
        cmds.connectAttr(fkBones[1]+".scale", self.boneFingerB+".scale")
        cmds.connectAttr(fkBones[2]+".scale", self.boneFingerC+".scale")
        RigUtility.modifyTransformChannels(self.fkFingerA, r=False)
        RigUtility.modifyTransformChannels(self.fkFingerB, r=False)
        RigUtility.modifyTransformChannels(self.fkFingerC, r=False)
        # Proxy Attributes
        cmds.addAttr(self.controlGroup, ln="stretchA", proxy=self.systemGroup+".stretchA")
        cmds.addAttr(self.controlGroup, ln="stretchB", proxy=self.systemGroup+".stretchB")
        cmds.addAttr(self.controlGroup, ln="stretchC", proxy=self.systemGroup+".stretchC")
        cmds.addAttr(self.controlGroup, ln="curl", proxy=self.systemGroup+".curl")
        cmds.addAttr(self.controlGroup, ln="curlAdd", proxy=self.systemGroup+".curlAdd")
        # Cleanup Channels and Visibility
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set Rig Node Values
        RigNode.setPlug(self.node, "fkFingerA", self.fkFingerA)
        RigNode.setPlug(self.node, "fkFingerB", self.fkFingerB)
        RigNode.setPlug(self.node, "fkFingerC", self.fkFingerC)
        # Set Rigged Status
        self.rigged = True


    def setPose(self, pose_id, *args):
        #default
        curl_value = cmds.getAttr(self.controlGroup+".curl")
        if pose_id == 0:
            curl_value = 0
        #relax
        elif pose_id == 1:
            curl_value = cmds.getAttr(self.systemGroup+".relaxPose")
        #fist
        elif pose_id == 2:
            curl_value = cmds.getAttr(self.systemGroup+".fistPose")
        cmds.setAttr(self.controlGroup+".curl", curl_value)


    def reset(self, *args):
        cmds.setAttr(self.controlGroup+".curl", 0)
        #reset control list
        controlList = [self.fkFingerA, self.fkFingerB, self.fkFingerC]
        for control in controlList:
            cmds.setAttr(control+".rotateX", 0)
            cmds.setAttr(control+".rotateY", 0)
            cmds.setAttr(control+".rotateZ", 0)


    def keyAll(self, *args):
        cmds.setAttr(self.controlGroup+".curl", 0)
        controlList = [self.fkFingerA, self.fkFingerB, self.fkFingerC]
        for control in controlList:
            cmds.setKeyframe(control)


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if not node:
            node = self.node
        #
        self.reset()
        markerList = [
            RigNode.getPlug(node, "markerFingerA"),
            RigNode.getPlug(node, "markerFingerB"),
            RigNode.getPlug(node, "markerFingerC")]
        fkControls = [self.fkFingerA, self.fkFingerB, self.fkFingerC]
        for i in range(0,len(fkControls)):
            cmds.matchTransform(fkControls[i], markerList[i], pos=0, rot=1, scl=0)



def getFingerName(id):
    output = "finger"
    if id == 1:
        output = "thumb"
    elif id==2:
        output = "index"
    elif id==3:
        output = "middle"
    elif id==4:
        output = "ring"
    elif id==5:
        output = "pinky"
    return output



def createFingerNetwork(controller):
    #Input Node
    inputNode = cmds.createNode("network", n="InputParameters")
    cmds.addAttr(inputNode, ln="curlWeight", at="float", k=True)
    cmds.addAttr(inputNode, ln="curlMaxAngle", at="float", k=True)
    #---
    cmds.addAttr(inputNode, ln="worldInverseMatrix", at="matrix")
    cmds.addAttr(inputNode, ln="worldMatrix", at="matrix")
    cmds.addAttr(inputNode, ln="offsetParentMatrix", at="matrix")
    #=== Curl
    #- Compute Angle = weight * maxAngle
    curlComputeAngle = cmds.createNode("floatMath", n="CurlComputeAngle")
    cmds.connectAttr(inputNode+".curlWeight", curlComputeAngle+".floatA")
    cmds.connectAttr(inputNode+".curlMaxAngle", curlComputeAngle+".floatB")
    cmds.setAttr(curlComputeAngle+".operation", 2)
    #- Clamp Weight Max
    curlClamp = cmds.createNode("floatMath", n="CurlClampMax")
    cmds.setAttr(curlClamp+".operation", 5) #max - we want the highest value between the weight and the max limit
    cmds.connectAttr(curlComputeAngle+".outFloat", curlClamp+".floatA")
    cmds.connectAttr(inputNode+".curlMaxAngle", curlClamp+".floatB")
    #- Convert to Vector (allow buffer for flipping if needed)
    curlMult = cmds.createNode("multiplyDivide", n="CurlMult")
    cmds.connectAttr(curlClamp+".outFloat", curlMult+".input1Z") 
    #- Compose Curl Output Matrix
    curlMatrix = cmds.createNode("composeMatrix", n="CurlMatrix")
    cmds.connectAttr(curlMult+".output", curlMatrix+".inputRotate")
    # special case the thumb
    if controller.split("|")[-1].startswith("thumb_A"):
        cmds.connectAttr(curlClamp+".outFloat", curlMult+".input1X")
        cmds.setAttr(curlMult+".input2X", -1) #flip it
    #=== Parent By Matrix
    #Parent Matrix Constraint
    constraintMatrix = cmds.createNode("multMatrix")
    cmds.connectAttr(inputNode+".offsetParentMatrix", constraintMatrix+".matrixIn[0]")
    cmds.connectAttr(inputNode+".worldMatrix", constraintMatrix+".matrixIn[1]")
    cmds.connectAttr(inputNode+".worldInverseMatrix", constraintMatrix+".matrixIn[2]")
    #Filter out scale
    blendMatrix = cmds.createNode("blendMatrix", n="BlendMatrixFilter")
    cmds.connectAttr(constraintMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
    cmds.setAttr(blendMatrix+".target[0].scaleWeight", 0)
    cmds.setAttr(blendMatrix+".target[0].shearWeight", 0)
    #Output
    outputNode = cmds.createNode("multMatrix", n="OutputMatrix")
    cmds.connectAttr(curlMatrix+".outputMatrix", outputNode+".matrixIn[0]")
    cmds.connectAttr(blendMatrix+".outputMatrix", outputNode+".matrixIn[1]")
    return inputNode, outputNode