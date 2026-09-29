from collections import OrderedDict
import os
import maya.cmds as cmds
import maya.mel as mel
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Controller as Controller
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.UI.Widgets as Widgets
import Otherside.Rigging.SpaceSwitch2 as SpaceSwitch2
from Otherside.Rigging.SpaceSwitch import SpaceSwitch
from Otherside.Rigging.ControlRig.ControlRigBase import ControlRigBase
import json


RIG_TYPE = "ControlRig"
MODULE_PATH = "Otherside.Rigging.ControlRig.Camera"
CLASS_NAME = "Camera"
CONTROLLER_SIZE = [5, 5, 5]


class Camera(ControlRigBase):

    def __init__(self, name="Camera", **kwargs):
        self.node = None
        self.instanceName = name
        self.spaces = {}
        self.side = 0
        # self.defaultSpace = "world"
        #
        self.characterized = False
        self.rigged = False
        #
        self.controlGroup = None
        self.systemGroup = None
        self.markerGroup = None
        self.rootSpace = None
        #GameCam
        self.gameCameraTranslationSpace = None
        self.gameCameraRotationSpace = None
        #
        self.boneGameCamera = None
        #
        self.markerGameCamera = None
        #
        self.controllerGameCamera = None
        #
        self.gameCamera = None
        self.gameCameraFocalLength=23.284  #fov=85
        self.gameCameraHorizontalFilmAperture=1.68  #16:9 ratio
        self.gameCameraVerticalFilmAperture=0.9449
        #PlayerCam
        self.playerCameraTranslationSpace = None
        self.playerCameraRotationSpace = None
        #
        self.bonePlayerCamera = None
        #
        self.markerPlayerCamera = None
        #
        self.controllerPlayerCamera = None
        #
        self.playerCamera = None
        self.playerCameraFocalLength=23.284  #fov=85
        self.playerCameraHorizontalFilmAperture=1.68  #16:9 ratio
        self.playerCameraVerticalFilmAperture=0.9449
        #
        self.bonePlayerView = None


    @staticmethod
    def load(node):
        instance = Camera()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")

        # instance.defaultSpace = cmds.getAttr(node+".defaultSpace")
        # if not instance.defaultSpace:
            # instance.defaultSpace = ""

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
        instance.gameCameraTranslationSpace = RigNode.getPlug(node, "gameCameraTranslationSpace")
        instance.gameCameraRotationSpace = RigNode.getPlug(node, "gameCameraRotationSpace")
        instance.boneGameCamera = RigNode.getPlug(node, "boneGameCamera")
        instance.markerGameCamera = RigNode.getPlug(node, "markerGameCamera")
        instance.controllerGameCamera = RigNode.getPlug(node, "controllerGameCamera")
        instance.playerCameraTranslationSpace = RigNode.getPlug(node, "playerCameraTranslationSpace")
        instance.playerCameraRotationSpace = RigNode.getPlug(node, "playerCameraRotationSpace")
        instance.bonePlayerCamera = RigNode.getPlug(node, "bonePlayerCamera")
        instance.markerPlayerCamera = RigNode.getPlug(node, "markerPlayerCamera")
        instance.controllerPlayerCamera = RigNode.getPlug(node, "controllerPlayerCamera")
        instance.gameCamera = RigNode.getPlug(node, "gameCamera")
        instance.playerCamera = RigNode.getPlug(node, "playerCamera")
        instance.bonePlayerView = RigNode.getPlug(node, "bonePlayerView")
        return instance


    def setBoneList(self, boneList):
        self.boneGameCamera = boneList[0]
        self.bonePlayerCamera = boneList[1]
        self.bonePlayerView = boneList[2]


    def getBoneList(self):
        return [self.boneGameCamera, self.bonePlayerCamera, self.bonePlayerView]


    def getBoneNames(self):
        return ["boneGameCamera", "bonePlayerCamera", "bonePlayerView"]


    def getKeyable(self):
        return [
            self.controlGroup,
            self.controllerGameCamera,
            self.controllerPlayerCamera
            ]


    def getKeyableNames(self):
        return [
            "controlGroup",
            "controllerGameCamera",
            "controllerPlayerCamera"
            ]


    def getMarkerList(self):
        return [self.markerGameCamera, self.markerPlayerCamera]


    def getMarkerNames(self):
        return ["markerGameCamera", "markerPlayerCamera"]

    #
    # Setup Functions
    #
    def createNode(self):
        #Build Template Node
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("spaces", "string")
        # nodeBuilder.addAttr("defaultSpace", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        nodeBuilder.addAttr("markerGroup", "message")
        nodeBuilder.addAttr("systemGroup", "message")
        nodeBuilder.addAttr("rootSpace", "message")
        nodeBuilder.addAttr("gameCameraTranslationSpace", "message")
        nodeBuilder.addAttr("gameCameraRotationSpace", "message")
        nodeBuilder.addAttr("boneGameCamera", "message")
        nodeBuilder.addAttr("markerGameCamera", "message")
        nodeBuilder.addAttr("controllerGameCamera", "message")
        nodeBuilder.addAttr("playerCameraTranslationSpace", "message")
        nodeBuilder.addAttr("playerCameraRotationSpace", "message")
        nodeBuilder.addAttr("bonePlayerCamera", "message")
        nodeBuilder.addAttr("markerPlayerCamera", "message")
        nodeBuilder.addAttr("controllerPlayerCamera", "message")
        nodeBuilder.addAttr("gameCamera", "message")
        nodeBuilder.addAttr("playerCamera", "message")
        nodeBuilder.addAttr("bonePlayerView", "message")
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
        # cmds.addAttr(self.systemGroup, ln="ikMode", at="float", min=0, max=1, dv=0, k=1)
        #RootSpace
        self.rootSpace = cmds.createNode("transform", n="rootSpace", p=self.systemGroup)
        self.rootSpace = cmds.ls(self.rootSpace, l=True)[-1]
        #Create Markers
        self.markerGroup = cmds.createNode("transform", p=self.controlGroup, n="markers")
        self.markerGroup = cmds.ls(self.markerGroup, l=True)[-1]
        self.markerGameCamera = Marker.create(n="markerGameCamera", t=self.boneGameCamera, p=self.markerGroup)
        self.markerPlayerCamera = Marker.create(n="markerPlayerCamera", t=self.bonePlayerCamera, p=self.markerGroup)
        # Mark Characterized
        self.characterized = True
        # Set RigNode Plugs
        cmds.setAttr("{}.characterized".format(self.node), True)
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        RigNode.setPlug(self.node, "markerGroup", self.markerGroup)
        RigNode.setPlug(self.node, "systemGroup", self.systemGroup)
        RigNode.setPlug(self.node, "rootSpace", self.rootSpace)
        RigNode.setPlug(self.node, "boneGameCamera", self.boneGameCamera)
        RigNode.setPlug(self.node, "markerGameCamera", self.markerGameCamera)
        RigNode.setPlug(self.node, "bonePlayerCamera", self.bonePlayerCamera)
        RigNode.setPlug(self.node, "markerPlayerCamera", self.markerPlayerCamera)
        RigNode.setPlug(self.node, "gameCamera", self.gameCamera)
        RigNode.setPlug(self.node, "playerCamera", self.playerCamera)
        RigNode.setPlug(self.node, "bonePlayerView", self.bonePlayerView)


    def rig(self, **kwargs):
        spacemap = kwargs.get("spacemap", {})
        RigUtility.modifyTransformChannels(self.controlGroup)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        
        # Controller GameCam
        self.controllerGameCamera = Controller.create(
            name = Controller.buildName("GameCam", None, "CTRL"),
            parent = self.controlGroup,
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.WHITE,
            size = CONTROLLER_SIZE)
        cmds.matchTransform(self.controllerGameCamera, self.boneGameCamera)
        RigUtility.bakeOffsetParentMatrix(self.controllerGameCamera)
        
        # Controller PlayerCam
        self.controllerPlayerCamera = Controller.create(
            name = Controller.buildName("PlayerCam", None, "CTRL"),
            parent = self.controlGroup,
            shape = Controller.Shape.SPHERE,
            color = Controller.Color.GREEN,
            size = [s/2.0 for s in CONTROLLER_SIZE])
        cmds.matchTransform(self.controllerPlayerCamera, self.bonePlayerCamera)
        RigUtility.bakeOffsetParentMatrix(self.controllerPlayerCamera)
        
        #create player cam image plane alpha slider for later use
        cmds.addAttr(self.controllerPlayerCamera, ln="imagePlaneAlpha", at="double", min=0, max=1, dv=0.5)
        cmds.setAttr(self.controllerPlayerCamera+".imagePlaneAlpha", cb=True, k=False)
        
        # grab existing head bone from other module
        # characterRN = RigNode.getRelated(self.node)
        # head = RigNode.getPlug(RigNode.getPlug(characterRN, "head"), "boneHead")
        
        # Game Cam Space Switch
        # | translation
        translate = cmds.createNode("transform", n="gameCameraTranslationSpace", p=self.systemGroup)
        cmds.matchTransform(translate, self.boneGameCamera)
        RigUtility.bakeOffsetParentMatrix(translate)
        spaceSwitch = SpaceSwitch2.create(translate, None)
        # spaceSwitch.addSpace("head", head)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "gameCameraTranslationSpace")
        spaceSwitch.addProxy(self.controlGroup, "gameCameraTranslationSpace")
        spaceSwitch.addProxy(self.controllerGameCamera, "gameCameraTranslationSpace")
        cmds.renameAttr(self.controllerGameCamera + ".gameCameraTranslationSpace", "TranslationSpace")
        # | rotation
        rotate = cmds.createNode("transform", n="gameCameraRotationSpace", p=self.systemGroup)
        cmds.matchTransform(rotate, self.boneGameCamera)
        RigUtility.bakeOffsetParentMatrix(rotate)
        spaceSwitch = SpaceSwitch2.create(rotate, None)
        # spaceSwitch.addSpace("head", head)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "gameCameraRotationSpace")
        spaceSwitch.addProxy(self.controlGroup, "gameCameraRotationSpace")
        spaceSwitch.addProxy(self.controllerGameCamera, "gameCameraRotationSpace")
        cmds.renameAttr(self.controllerGameCamera + ".gameCameraRotationSpace", "RotationSpace")
        
        RigUtility.parentByMatrix(self.controllerGameCamera, translate)

        mm = cmds.listConnections(self.controllerGameCamera+".offsetParentMatrix")[0]
        decompCam = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(mm+".matrixSum", decompCam+".inputMatrix")
        
        decompView = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(self.bonePlayerView+".worldMatrix[0]", decompView+".inputMatrix")
        
        #blend between decompCam and decompView translates
        tx = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateX", tx+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateX", tx+".input[1]")
        ty = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateY", ty+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateY", ty+".input[1]")
        tz = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateZ", tz+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateZ", tz+".input[1]")
        
        # recompose the matrix
        composeMatrix = cmds.createNode("composeMatrix")
        cmds.connectAttr(tx+".output", composeMatrix+".inputTranslateX")
        cmds.connectAttr(ty+".output", composeMatrix+".inputTranslateY")
        cmds.connectAttr(tz+".output", composeMatrix+".inputTranslateZ")
        
        #create the blend driver attrs
        cmds.addAttr(self.systemGroup, ln="gameCameraFollowViewBoneTX", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerGameCamera, ln="followViewBoneTX", proxy=self.systemGroup+".gameCameraFollowViewBoneTX")
        cmds.addAttr(self.controlGroup, ln="gameCameraFollowViewBoneTX", proxy=self.systemGroup+".gameCameraFollowViewBoneTX")
        cmds.addAttr(self.systemGroup, ln="gameCameraFollowViewBoneTY", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerGameCamera, ln="followViewBoneTY", proxy=self.systemGroup+".gameCameraFollowViewBoneTY")
        cmds.addAttr(self.controlGroup, ln="gameCameraFollowViewBoneTY", proxy=self.systemGroup+".gameCameraFollowViewBoneTY")
        cmds.addAttr(self.systemGroup, ln="gameCameraFollowViewBoneTZ", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerGameCamera, ln="followViewBoneTZ", proxy=self.systemGroup+".gameCameraFollowViewBoneTZ")
        cmds.addAttr(self.controlGroup, ln="gameCameraFollowViewBoneTZ", proxy=self.systemGroup+".gameCameraFollowViewBoneTZ")
        #drive the blends
        cmds.connectAttr(self.systemGroup+".gameCameraFollowViewBoneTX", tx+".attributesBlender")
        cmds.connectAttr(self.systemGroup+".gameCameraFollowViewBoneTY", ty+".attributesBlender")
        cmds.connectAttr(self.systemGroup+".gameCameraFollowViewBoneTZ", tz+".attributesBlender")
        cmds.connectAttr(composeMatrix+".outputMatrix", self.controllerGameCamera+".offsetParentMatrix", force=True)
        
        #rotations are handled on a child node
        gameCameraDriver = cmds.createNode("transform", n="gameCameraDriver", p=self.controllerGameCamera)
        decompRot = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(rotate+".worldMatrix[0]", decompRot+".inputMatrix")
        cmds.connectAttr(decompRot+".outputRotate", gameCameraDriver+".rotate")

        # Player Cam Space Switch
        # | translation
        translate = cmds.createNode("transform", n="playerCameraTranslationSpace", p=self.systemGroup)
        cmds.matchTransform(translate, self.bonePlayerCamera)
        RigUtility.bakeOffsetParentMatrix(translate)
        spaceSwitch = SpaceSwitch2.create(translate, None)
        spaceSwitch.addSpace("playerView", self.bonePlayerView)
        spaceSwitch.addSpace("gameCam", self.boneGameCamera)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "playerCameraTranslationSpace")
        spaceSwitch.addProxy(self.controlGroup, "playerCameraTranslationSpace")
        spaceSwitch.addProxy(self.controllerPlayerCamera, "playerCameraTranslationSpace")
        cmds.renameAttr(self.controllerPlayerCamera + ".playerCameraTranslationSpace", "TranslationSpace")
        # | rotation
        rotate = cmds.createNode("transform", n="playerCameraRotationSpace", p=self.systemGroup)
        cmds.matchTransform(rotate, self.bonePlayerCamera)
        RigUtility.bakeOffsetParentMatrix(rotate)
        spaceSwitch = SpaceSwitch2.create(rotate, None)
        spaceSwitch.addSpace("playerView", self.bonePlayerView)
        spaceSwitch.addSpace("gameCam", self.boneGameCamera)
        spaceSwitch.addSpaceMap(self.spaces)
        spaceSwitch.addSpace("world", self.rootSpace)
        spaceSwitch.build(parent=self.systemGroup)
        spaceSwitch.addProxy(self.systemGroup, "playerCameraRotationSpace")
        spaceSwitch.addProxy(self.controlGroup, "playerCameraRotationSpace")
        spaceSwitch.addProxy(self.controllerPlayerCamera, "playerCameraRotationSpace")
        cmds.renameAttr(self.controllerPlayerCamera + ".playerCameraRotationSpace", "RotationSpace")
        
        RigUtility.parentByMatrix(self.controllerPlayerCamera, translate)
        
        mm = cmds.listConnections(self.controllerPlayerCamera+".offsetParentMatrix")[0]
        decompCam = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(mm+".matrixSum", decompCam+".inputMatrix")
        
        decompView = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(self.bonePlayerView+".worldMatrix[0]", decompView+".inputMatrix")
        
        #blend between decompCam and decompView translates
        tx = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateX", tx+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateX", tx+".input[1]")
        ty = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateY", ty+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateY", ty+".input[1]")
        tz = cmds.createNode("blendTwoAttr")
        cmds.connectAttr(decompCam+".outputTranslateZ", tz+".input[0]")
        cmds.connectAttr(decompView+".outputTranslateZ", tz+".input[1]")
        
        # recompose the matrix
        composeMatrix = cmds.createNode("composeMatrix")
        cmds.connectAttr(tx+".output", composeMatrix+".inputTranslateX")
        cmds.connectAttr(ty+".output", composeMatrix+".inputTranslateY")
        cmds.connectAttr(tz+".output", composeMatrix+".inputTranslateZ")
        
        #create the blend driver attrs
        cmds.addAttr(self.systemGroup, ln="playerCameraFollowViewBoneTX", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerPlayerCamera, ln="followViewBoneTX", proxy=self.systemGroup+".playerCameraFollowViewBoneTX")
        cmds.addAttr(self.controlGroup, ln="playerCameraFollowViewBoneTX", proxy=self.systemGroup+".playerCameraFollowViewBoneTX")
        cmds.addAttr(self.systemGroup, ln="playerCameraFollowViewBoneTY", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerPlayerCamera, ln="followViewBoneTY", proxy=self.systemGroup+".playerCameraFollowViewBoneTY")
        cmds.addAttr(self.controlGroup, ln="playerCameraFollowViewBoneTY", proxy=self.systemGroup+".playerCameraFollowViewBoneTY")
        cmds.addAttr(self.systemGroup, ln="playerCameraFollowViewBoneTZ", at="double", min=0, max=1, dv=0, k=True)
        cmds.addAttr(self.controllerPlayerCamera, ln="followViewBoneTZ", proxy=self.systemGroup+".playerCameraFollowViewBoneTZ")
        cmds.addAttr(self.controlGroup, ln="playerCameraFollowViewBoneTZ", proxy=self.systemGroup+".playerCameraFollowViewBoneTZ")
        #drive the blends
        cmds.connectAttr(self.systemGroup+".playerCameraFollowViewBoneTX", tx+".attributesBlender")
        cmds.connectAttr(self.systemGroup+".playerCameraFollowViewBoneTY", ty+".attributesBlender")
        cmds.connectAttr(self.systemGroup+".playerCameraFollowViewBoneTZ", tz+".attributesBlender")
        cmds.connectAttr(composeMatrix+".outputMatrix", self.controllerPlayerCamera+".offsetParentMatrix", force=True)
        
        #rotations are handled on a child node
        playerCameraDriver = cmds.createNode("transform", n="playerCameraDriver", p=self.controllerPlayerCamera)
        decompRot = cmds.createNode("decomposeMatrix")
        cmds.connectAttr(rotate+".worldMatrix[0]", decompRot+".inputMatrix")
        cmds.connectAttr(decompRot+".outputRotate", playerCameraDriver+".rotate")

        cmds.parentConstraint(gameCameraDriver, self.boneGameCamera, mo=True)
        cmds.parentConstraint(playerCameraDriver, self.bonePlayerCamera, mo=True)
        # Lock and Hide Core Groups Transform Values
        RigUtility.modifyTransformChannels(self.controlGroup, v=False)
        RigUtility.modifyTransformChannels(self.systemGroup, v=False)
        cmds.setAttr(self.systemGroup+".v", 0)
        cmds.setAttr(self.systemGroup+".hiddenInOutliner", 1)
        RigUtility.modifyTransformChannels(self.markerGroup, v=False)
        cmds.setAttr(self.markerGroup+".v", 0)
        cmds.setAttr(self.markerGroup+".hiddenInOutliner", 1)
        cmds.setAttr(gameCameraDriver+".hiddenInOutliner", 1)
        cmds.setAttr(playerCameraDriver+".hiddenInOutliner", 1)
        
        #gameCamera
        self.gameCamera = cmds.camera(focalLength=self.gameCameraFocalLength, horizontalFilmAperture=self.gameCameraHorizontalFilmAperture, verticalFilmAperture=self.gameCameraVerticalFilmAperture)[0]
        self.gameCamera = cmds.rename(self.gameCamera, "GameCam")
        if self.gameCamera != "GameCam":
            raise NameError("GameCam is not unique")
        cmds.parent(self.gameCamera, self.controlGroup)
        cmds.matchTransform(self.gameCamera, self.boneGameCamera)
        cmds.setAttr(self.gameCamera+".ry", 180)  #TODO: don't assume 180.  Set it to match world +Z
        cmds.parentConstraint(self.boneGameCamera, self.gameCamera, mo=True)
        #set default spaces
        cmds.setAttr("{}.gameCameraTranslationSpace".format(self.controlGroup), 2)  #world
        cmds.setAttr("{}.gameCameraRotationSpace".format(self.controlGroup), 2)  #world
        #enable film gate and overscan
        cmds.setAttr(self.gameCamera+".displayFilmGate", 1)
        cmds.setAttr(self.gameCamera+".overscan", 1.1)
        #make it unselectable in viewport
        cmds.setAttr(self.gameCamera+".overrideEnabled", 1) #enable
        cmds.setAttr(self.gameCamera+".overrideDisplayType", 2) #reference

        #playerCamera
        self.playerCamera = cmds.camera(focalLength=self.playerCameraFocalLength, horizontalFilmAperture=self.playerCameraHorizontalFilmAperture, verticalFilmAperture=self.playerCameraVerticalFilmAperture)[0]
        self.playerCamera = cmds.rename(self.playerCamera, "PlayerCam")
        if self.playerCamera != "PlayerCam":
            raise NameError("PlayerCam is not unique")
        cmds.parent(self.playerCamera, self.controlGroup)
        cmds.matchTransform(self.playerCamera, self.bonePlayerCamera)
        cmds.setAttr(self.playerCamera+".ry", 180)  #TODO: don't assume 180.  Set it to match world +Z
        cmds.parentConstraint(self.bonePlayerCamera, self.playerCamera, mo=True)
        #set default spaces
        cmds.setAttr("{}.playerCameraTranslationSpace".format(self.controlGroup), 0)  #playerview
        cmds.setAttr("{}.playerCameraRotationSpace".format(self.controlGroup), 1)  #gamecam
        cmds.setAttr(self.systemGroup+".playerCameraFollowViewBoneTX", 1)
        cmds.setAttr(self.systemGroup+".playerCameraFollowViewBoneTY", 1)
        cmds.setAttr(self.systemGroup+".playerCameraFollowViewBoneTZ", 1)
        #enable film gate and overscan
        cmds.setAttr(self.playerCamera+".displayFilmGate", 1)
        cmds.setAttr(self.playerCamera+".overscan", 1.1)
        #make it unselectable in viewport
        cmds.setAttr(self.playerCamera+".overrideEnabled", 1) #enable
        cmds.setAttr(self.playerCamera+".overrideDisplayType", 2) #reference

        #image plane
        project = cmds.workspace(q=1,rd=1).replace("\\", "/")
        png = project+"Animation/_Setup/ImagePlane/imagePlane.png"
        if os.path.isfile(png):
            mel.eval("source importImagePlane.mel;")
            mel.eval('createImportedImagePlane({"'+self.playerCamera+'"}, "' +png+ '", "png");')
            # cmds.imagePlane(self.playerCamera, e=1, showInAllViews=1)
            ip = cmds.imagePlane(self.playerCamera, q=1, name=1)[0]
            cmds.connectAttr(self.controllerPlayerCamera+".imagePlaneAlpha", ip+".alphaGain")
            cmds.setAttr(ip + ".displayOnlyIfCurrent", 1)
        
        #set RigNode
        cmds.setAttr(self.node+".spaces", str(self.spaces), type="string")
        # cmds.setAttr(self.node+".defaultSpace", self.defaultSpace, type="string")        
        RigNode.setPlug(self.node, "gameCameraTranslationSpace", self.gameCameraTranslationSpace)
        RigNode.setPlug(self.node, "gameCameraRotationSpace", self.gameCameraRotationSpace)
        RigNode.setPlug(self.node, "controllerGameCamera", self.controllerGameCamera)        
        RigNode.setPlug(self.node, "playerCameraTranslationSpace", self.playerCameraTranslationSpace)
        RigNode.setPlug(self.node, "playerCameraRotationSpace", self.playerCameraRotationSpace)
        RigNode.setPlug(self.node, "controllerPlayerCamera", self.controllerPlayerCamera)
        RigNode.setPlug(self.node, "gameCamera", self.gameCamera)
        RigNode.setPlug(self.node, "playerCamera", self.playerCamera)
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
        # spaceAttr = "{}.space".format(self.controlGroup)
        # Widgets.WSpaceSwitch2.Create(spaceAttr, label="Camera Space", labelWidth=200, fieldWidth=150, changeCommand=self.setSpace, keyCommand=self.keySpace)
        #gameCam
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".gameCameraTranslationSpace", label="gameCameraTranslationSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setGameCameraTranslationSpace, keyCommand=self.keyGameCameraTranslationSpace)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".gameCameraRotationSpace", label="gameCameraRotationSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setGameCameraRotationSpace, keyCommand=self.keyGameCameraRotationSpace)
        Widgets.WAttributeField.Create(self.controlGroup+'.gameCameraFollowViewBoneTX', label="Game Camera Follow ViewBone TX")
        Widgets.WAttributeField.Create(self.controlGroup+'.gameCameraFollowViewBoneTY', label="Game Camera Follow ViewBone TY")
        Widgets.WAttributeField.Create(self.controlGroup+'.gameCameraFollowViewBoneTZ', label="Game Camera Follow ViewBone TZ")
        cmds.setParent('..')
        #playerCam
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".playerCameraTranslationSpace", label="playerCameraTranslationSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setPlayerCameraTranslationSpace, keyCommand=self.keyPlayerCameraTranslationSpace)
        Widgets.WSpaceSwitch2.Create(self.controlGroup+".playerCameraRotationSpace", label="playerCameraRotationSpace", labelWidth=200, fieldWidth=150, changeCommand=self.setPlayerCameraRotationSpace, keyCommand=self.keyPlayerCameraRotationSpace)
        Widgets.WAttributeField.Create(self.controlGroup+'.playerCameraFollowViewBoneTZ', label="Player Camera Follow ViewBone TX")
        Widgets.WAttributeField.Create(self.controlGroup+'.playerCameraFollowViewBoneTY', label="Player Camera Follow ViewBone TY")
        Widgets.WAttributeField.Create(self.controlGroup+'.playerCameraFollowViewBoneTZ', label="Player Camera Follow ViewBone TZ")
        cmds.setParent('..')


    def setGameCameraTranslationSpace(self, index, *args):
        worldMatrix = cmds.xform(self.controllerGameCamera, q=True, ws=True, m=True)
        cmds.setAttr("{}.gameCameraTranslationSpace".format(self.controlGroup), index)
        cmds.xform(self.controllerGameCamera, ws=True, m=worldMatrix)


    def keyGameCameraTranslationSpace(self, *args):
        cmds.setKeyframe("{}.gameCameraTranslationSpace".format(self.controlGroup))
        cmds.setKeyframe(self.controllerGameCamera)


    def setGameCameraRotationSpace(self, index, *args):
        worldMatrix = cmds.xform(self.controllerGameCamera, q=True, ws=True, m=True)
        cmds.setAttr("{}.gameCameraRotationSpace".format(self.controlGroup), index)
        cmds.xform(self.controllerGameCamera, ws=True, m=worldMatrix)


    def keyGameCameraRotationSpace(self, *args):
        cmds.setKeyframe("{}.gameCameraRotationSpace".format(self.controlGroup))
        cmds.setKeyframe(self.controllerGameCamera)

        
    def setPlayerCameraTranslationSpace(self, index, *args):
        worldMatrix = cmds.xform(self.controllerPlayerCamera, q=True, ws=True, m=True)
        cmds.setAttr("{}.playerCameraTranslationSpace".format(self.controlGroup), index)
        cmds.xform(self.controllerPlayerCamera, ws=True, m=worldMatrix)


    def keyPlayerCameraTranslationSpace(self, *args):
        cmds.setKeyframe("{}.playerCameraTranslationSpace".format(self.controlGroup))
        cmds.setKeyframe(self.controllerPlayerCamera)


    def setPlayerCameraRotationSpace(self, index, *args):
        worldMatrix = cmds.xform(self.controllerPlayerCamera, q=True, ws=True, m=True)
        cmds.setAttr("{}.playerCameraRotationSpace".format(self.controlGroup), index)
        cmds.xform(self.controllerPlayerCamera, ws=True, m=worldMatrix)


    def keyPlayerCameraRotationSpace(self, *args):
        cmds.setKeyframe("{}.playerCameraRotationSpace".format(self.controlGroup))
        cmds.setKeyframe(self.controllerPlayerCamera)


    def reset(self, *args):
        cmds.setAttr("{}.gameCameraTranslationSpace".format(self.controlGroup), 2)  #world
        cmds.setAttr("{}.gameCameraRotationSpace".format(self.controlGroup), 2)  #world
        cmds.setAttr(self.controllerGameCamera+".translateX", 0)
        cmds.setAttr(self.controllerGameCamera+".translateY", 0)
        cmds.setAttr(self.controllerGameCamera+".translateZ", 0)
        cmds.setAttr(self.controllerGameCamera+".rotateX", 0)
        cmds.setAttr(self.controllerGameCamera+".rotateY", 0)
        cmds.setAttr(self.controllerGameCamera+".rotateZ", 0)
        cmds.setAttr(self.gameCamera+".focalLength", self.gameCameraFocalLength)
        cmds.setAttr(self.gameCamera+".horizontalFilmAperture", self.gameCameraHorizontalFilmAperture)
        cmds.setAttr(self.gameCamera+".verticalFilmAperture", self.gameCameraVerticalFilmAperture)
        cmds.setAttr(self.controlGroup+".gameCameraFollowViewBoneTX", 0)
        cmds.setAttr(self.controlGroup+".gameCameraFollowViewBoneTY", 0)
        cmds.setAttr(self.controlGroup+".gameCameraFollowViewBoneTZ", 0)
        #playerCam
        cmds.setAttr("{}.playerCameraTranslationSpace".format(self.controlGroup), 0)  #playerview
        cmds.setAttr("{}.playerCameraRotationSpace".format(self.controlGroup), 1)  #gameCam
        cmds.setAttr(self.controllerPlayerCamera+".translateX", 0)
        cmds.setAttr(self.controllerPlayerCamera+".translateY", 0)
        cmds.setAttr(self.controllerPlayerCamera+".translateZ", 0)
        cmds.setAttr(self.controllerPlayerCamera+".rotateX", 0)
        cmds.setAttr(self.controllerPlayerCamera+".rotateY", 0)
        cmds.setAttr(self.controllerPlayerCamera+".rotateZ", 0)
        cmds.setAttr(self.playerCamera+".focalLength", self.playerCameraFocalLength)
        cmds.setAttr(self.playerCamera+".horizontalFilmAperture", self.playerCameraHorizontalFilmAperture)
        cmds.setAttr(self.playerCamera+".verticalFilmAperture", self.playerCameraVerticalFilmAperture)        
        cmds.setAttr(self.controlGroup+".playerCameraFollowViewBoneTX", 1)
        cmds.setAttr(self.controlGroup+".playerCameraFollowViewBoneTY", 1)
        cmds.setAttr(self.controlGroup+".playerCameraFollowViewBoneTZ", 1)


    def keyAll(self, *args):
        cmds.setKeyframe(self.controllerGameCamera)
        cmds.setKeyframe("{}.gameCameraTranslationSpace".format(self.controlGroup))
        cmds.setKeyframe("{}.gameCameraRotationSpace".format(self.controlGroup))
        cmds.setKeyframe("{}.gameCameraFollowViewBoneTX".format(self.controlGroup))
        cmds.setKeyframe("{}.gameCameraFollowViewBoneTY".format(self.controlGroup))
        cmds.setKeyframe("{}.gameCameraFollowViewBoneTZ".format(self.controlGroup))
        cmds.setKeyframe(self.controllerPlayerCamera)
        cmds.setKeyframe("{}.playerCameraTranslationSpace".format(self.controlGroup))
        cmds.setKeyframe("{}.playerCameraRotationSpace".format(self.controlGroup))        
        cmds.setKeyframe("{}.playerCameraFollowViewBoneTX".format(self.controlGroup))
        cmds.setKeyframe("{}.playerCameraFollowViewBoneTY".format(self.controlGroup))
        cmds.setKeyframe("{}.playerCameraFollowViewBoneTZ".format(self.controlGroup))


    def sync(self, *args, **kwargs):
        node = kwargs.get("node", None)
        if node == None:
            node = self.node
        marker = RigNode.getPlug(node, "markerGameCamera")
        cmds.matchTransform(self.controllerGameCamera, marker, pos=1, rot=1, scl=0)
        marker = RigNode.getPlug(node, "markerPlayerCamera")
        cmds.matchTransform(self.controllerPlayerCamera, marker, pos=1, rot=1, scl=0)