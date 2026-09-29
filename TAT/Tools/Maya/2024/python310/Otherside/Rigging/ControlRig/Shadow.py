import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.UI.Widgets as Widgets
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase

RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Shadow"
CLASS_NAME = "Shadow"
CONTROLLER_SIZE = [15, 15, 15]


class Shadow(ControlRigBase):
    def __init__(self, name="Shadow", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = 0
        self.characterized = False
        self.rigged = False
        self.controlGroup = None
        self.systemGroup = None
        # self.markerGroup = None
        self.rootSpace = None
        # self.boneRoot = None
        self.boneShadow = None
        # self.markerRoot = None
        # self.controllerPlacement = None
        # self.controllerRoot = None
        self.controllerShadow = None


    @staticmethod
    def load(node):
        instance = Shadow()
        instance.node = node
        instance.spaces = {}
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        # instance.markerGroup = RigNode.getPlug(node, "markerGroup")
        instance.systemGroup = RigNode.getPlug(node, "systemGroup")
        instance.rootSpace = RigNode.getPlug(node, "rootSpace")
        # instance.boneRoot = RigNode.getPlug(node, "boneRoot")
        instance.boneShadow = RigNode.getPlug(node, "boneShadow")
        # instance.markerRoot = RigNode.getPlug(node, "markerRoot")
        # instance.controllerPlacement = RigNode.getPlug(node, "controllerPlacement")
        # instance.controllerRoot = RigNode.getPlug(node, "controllerRoot")
        instance.controllerShadow = RigNode.getPlug(node, "controllerShadow")
        return instance


    def setBoneList(self, boneList):
        pass


    def getBoneList(self):
        return []


    def getBoneNames(self):
        return []


    def getKeyable(self):
        return [self.controllerShadow]


    def getKeyableNames(self):
        return ["controllerShadow"]


    def getMarkerList(self):
        return []


    def getMarkerNames(self):
        return []


    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        # nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        # nodeBuilder.addAttr("boneRoot", "message")
        nodeBuilder.addAttr("boneShadow", "message")
        # nodeBuilder.addAttr("markerRoot", "message")
        # nodeBuilder.addAttr("controllerPlacement", "message")
        # nodeBuilder.addAttr("controllerRoot", "message")
        nodeBuilder.addAttr("controllerShadow", "message")
        self.node = nodeBuilder.write()
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")


    def characterize(self, **kwargs):
        #kwargs
        parentGroup = kwargs.get("p", None)
        # Setup Control Group
        if self.controlGroup == None:
            self.controlGroup = cmds.createNode("transform", n=self.instanceName, p=parentGroup)
        # Marker Gorup
        # self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        # self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        # System Group
        if self.systemGroup == None:
            self.systemGroup = cmds.createNode("transform", n="system", p=self.controlGroup)
            self.systemGroup = cmds.ls(self.systemGroup, l=True)[-1]
        # RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        # Create Markers
        # self.markerRoot = Marker.create(n="markerRoot", t=self.boneRoot, p=self.markerGroup)
        # Marker.orient(self.markerRoot, targetPoint=[0,0,10], aimAxis=[0,0,1], upAxis=[0,1,0], worldAxis=[0,1,0])
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        # RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        # RigNode.setPlug(self.node, "boneRoot", self.boneRoot)
        RigNode.setPlug(self.node, "boneShadow", self.boneShadow)
        # RigNode.setPlug(self.node, "markerRoot", self.markerRoot)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        #Controller - Placement
        # self.controllerPlacement = Controller.create(
            # name = "placement_controller",
            # parent = self.controlGroup,
            # shape = Controller.Shape.TRIANGLE,
            # color = Controller.Color.GREEN,
            # size = [50,50,50],
            # normal = [0,1,0]
            # )
        #Controller - Root
        # self.controllerRoot = Controller.create(
            # name = "root_controller",
            # parent = self.controlGroup,
            # shape = Controller.Shape.TRIANGLE,
            # color = Controller.Color.RED,
            # size = [5,5,5],
            # normal = [0,1,0]
            # )
        #Controller - Shadow
        self.controllerShadow = Controller.create(
            name = "shadow_controller",
            parent = self.controlGroup,
            shape = Controller.Shape.TRIANGLE,
            color = Controller.Color.BLUE,
            size = [3,3,3],
            normal = [0,1,0]
            )
        #    
        characterRN = RigNode.getRelated(self.node)
        #read from placement module
        mod = RigNode.getPlug(characterRN, "placement")
        controlGroup = RigNode.getPlug(mod, "controlGroup")
        controllerRoot = RigNode.getPlug(mod, "controllerRoot")
        controllerPlacement = RigNode.getPlug(mod, "controllerPlacement")
        #read from torso module
        mod = RigNode.getPlug(characterRN, "torso")
        controlGroup = RigNode.getPlug(mod, "controlGroup")
        # systemGroup = RigNode.getPlug(mod, "systemGroup")
        fkHips = RigNode.getPlug(mod, "fkHips")
        ikHips = RigNode.getPlug(mod, "ikHips")
            
        
        # cmds.addAttr(self.systemGroup, ln="follow", at="float", min=0, max=1, dv=1, k=True)
        # cmds.addAttr(self.controlGroup, ln="follow", proxy=self.systemGroup+".follow")
        # multMatrix= cmds.createNode("multMatrix")
        # cmds.connectAttr(self.controllerPlacement+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        # cmds.connectAttr(self.controlGroup+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        # blendMatrix = cmds.createNode("blendMatrix")
        # if not self.boneShadow:
            # cmds.connectAttr(multMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix")
        # cmds.connectAttr(self.systemGroup+".follow", blendMatrix+".envelope")
        # cmds.connectAttr(blendMatrix+".outputMatrix", self.controllerRoot+".offsetParentMatrix")
        # cmds.parentConstraint(self.controllerRoot, self.boneRoot)
        #Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        # RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        # cmds.setAttr(self.markerGroup+".v", 0)
        # cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        # Set RigNode Plugs        
        # RigNode.setPlug(self.node, "controllerPlacement", self.controllerPlacement)
        # RigNode.setPlug(self.node, "controllerRoot", self.controllerRoot)
        

        # create channel attrs
        cmds.addAttr(controllerRoot, ln="followX", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(controllerRoot, ln="followY", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(controllerRoot, ln="followZ", at="float", min=0, max=1, dv=0, k=True)
        cmds.addAttr(controllerRoot, ln="followYaw", at="float", min=0, max=1, dv=0, k=True)
        # get shadow in placement space
        shadowMultMatrix = cmds.createNode("multMatrix", n="shadowInPlacementSpace_multMatrix")
        cmds.connectAttr(self.controllerShadow+".worldMatrix[0]", shadowMultMatrix+".matrixIn[0]")
        cmds.connectAttr(controllerPlacement+".worldInverseMatrix[0]", shadowMultMatrix+".matrixIn[1]")
        # decompose so we can manipulate each channel
        decomposeMatrix = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(shadowMultMatrix+".matrixSum", decomposeMatrix+".inputMatrix")
        # manipulate each translation channel
        multDivT = cmds.createNode("multiplyDivide", n="multiplyDivideT")
        cmds.connectAttr(decomposeMatrix+".outputTranslate", multDivT+".input1")
        cmds.connectAttr(controllerRoot+".followX", multDivT+".input2X")
        cmds.connectAttr(controllerRoot+".followY", multDivT+".input2Y")
        cmds.connectAttr(controllerRoot+".followZ", multDivT+".input2Z")
        # manipulate yaw
        multDivR = cmds.createNode("multiplyDivide", n="multiplyDivideR")
        cmds.connectAttr(decomposeMatrix+".outputRotateY", multDivR+".input1Y")
        cmds.connectAttr(controllerRoot+".followYaw", multDivR+".input2Y")
        # recompose the matrix
        composeMatrix = cmds.createNode("composeMatrix")
        cmds.connectAttr(multDivT+".output", composeMatrix+".inputTranslate")
        cmds.connectAttr(multDivR+".outputY", composeMatrix+".inputRotateY")
        
        # get connections
        blendMatrix = cmds.listConnections(controllerRoot+".offsetParentMatrix")[0]
        multMatrix = cmds.listConnections(blendMatrix+".target[0].targetMatrix")[0]
        # weightSwitch = cmds.listConnections(systemGroup+".ikMode")[0]
        weightSwitch = cmds.listConnections(ikHips+".v")[0]
        
        # hook up the result
        finalMultMatrix = cmds.createNode("multMatrix")
        cmds.connectAttr(composeMatrix+".outputMatrix", finalMultMatrix+".matrixIn[0]")
        cmds.connectAttr(multMatrix+".matrixSum", finalMultMatrix+".matrixIn[1]")        

        # override connection
        cmds.connectAttr(finalMultMatrix+".matrixSum", blendMatrix+".target[0].targetMatrix", force=True)
        
        # hook up shadow
        pelvisFollow = cmds.createNode("transform", n="pelvisFollow", p=self.systemGroup)
        
        pc = cmds.parentConstraint(fkHips, ikHips, pelvisFollow, mo=False)[0]
        # cmds.connectAttr(weightSwitch+".output", constraint+".v", f=True)
        # cmds.connectAttr(weightSwitch+".outputInverse", constraint+".v", f=True)
        weightList = cmds.parentConstraint(pc, q=True, wal=True)
        cmds.connectAttr(weightSwitch+".output", pc+"."+weightList[1], f=True)
        cmds.connectAttr(weightSwitch+".outputInverse", pc+"."+weightList[0], f=True)
        # align to world so constraints work better
        pelvisPos = cmds.createNode("transform", n="pelvisPos", p=pelvisFollow)
        #cmds.setAttr(pelvisPos+".translate", 0, 0, 0)
        cmds.xform(pelvisPos, ws=True, ro=[0,0,0])
        # constrain only what we need
        pelvisShadow = cmds.createNode("transform", n="pelvisShadow", p=self.systemGroup)
        cmds.pointConstraint(pelvisPos, pelvisShadow, skip="y")
        cmds.orientConstraint(pelvisPos, pelvisShadow, skip=["x","z"])
        # drive shadow
        shadowMatrix = cmds.createNode("multMatrix", n="shadowMatrix")
        cmds.connectAttr(pelvisShadow+".worldMatrix[0]", shadowMatrix+".matrixIn[0]")
        
        cmds.connectAttr(self.controlGroup+".worldInverseMatrix[0]", shadowMatrix+".matrixIn[1]")
        #connect to shadowCtrl
        cmds.connectAttr(shadowMatrix+".matrixSum", self.controllerShadow+".offsetParentMatrix")
        
        #set pelvisShadow rotate order to yzx
        cmds.setAttr(pelvisShadow + '.rotateOrder', 1)
        
        # Set RigNode Plugs
        RigNode.setPlug(self.node, "controllerShadow", self.controllerShadow)
        cmds.setAttr(self.node+".rigged", True)
        self.rigged = True


    def gui(self):
        cmds.columnLayout(co=("both", 5))
        cmds.button(l="Reset", c=self.reset, width=350)
        cmds.button(l="Key All", c=self.keyAll, width=350)
        cmds.separator( height=15)
        # Widgets.WAttributeToggle.Create(self.controlGroup+".follow", label="Root Follow", labelWidth=200, fieldWidth=150, changeCommand=self.setRootFollow, keyCommand=self.keyRootFollow)
        cmds.setParent('..')


    def reset(self, *args):
        cmds.setAttr(self.controllerShadow+".translateX", 0)
        cmds.setAttr(self.controllerShadow+".translateY", 0)
        cmds.setAttr(self.controllerShadow+".translateZ", 0)
        cmds.setAttr(self.controllerShadow+".rotateX", 0)
        cmds.setAttr(self.controllerShadow+".rotateY", 0)
        cmds.setAttr(self.controllerShadow+".rotateZ", 0)
        
        # cmds.setAttr(self.controllerRoot+".followX", 0)
        # cmds.setAttr(self.controllerRoot+".followY", 0)
        # cmds.setAttr(self.controllerRoot+".followZ", 0)
        # cmds.setAttr(self.controllerRoot+".followYaw", 0)



    def keyAll(self, *args):
        cmds.setKeyframe(self.controllerShadow)
        # cmds.setKeyframe(self.controllerRoot+".followX")
        # cmds.setKeyframe(self.controllerRoot+".followY")
        # cmds.setKeyframe(self.controllerRoot+".followZ")
        # cmds.setKeyframe(self.controllerRoot+".followYaw")


    def sync(self, *args, **kwargs):
        pass
