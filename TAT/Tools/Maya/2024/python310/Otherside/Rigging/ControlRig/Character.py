import json
from functools import partial
from collections import OrderedDict
import maya.cmds as cmds
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.Marker as Marker
from Otherside.Rigging.ControlRig.Placement import Placement
from Otherside.Rigging.ControlRig.Head import Head
from Otherside.Rigging.ControlRig.Torso import Torso
from Otherside.Rigging.ControlRig.Arm import Arm
from Otherside.Rigging.ControlRig.Hand import Hand
from Otherside.Rigging.ControlRig.Finger import Finger
from Otherside.Rigging.ControlRig.Leg import Leg
from Otherside.Rigging.ControlRig.Shoulder import Shoulder
from Otherside.Rigging.ControlRig.Prop import Prop

RIG_TYPE = "Character"
MODULE_PATH = "Otherside.Rigging.ControlRig.Character"
CLASS_NAME = "Character"


def load(node):
    return Character.load(node)


class Character():
    @staticmethod
    def load(node):
        instance = Character()
        instance.node = node
        instance.instanceName = cmds.getAttr(node+".instanceName")
        instance.controlGroup = RigNode.getPlug(node, "controlGroup")
        instance.characterized = cmds.getAttr(node+".characterized")
        instance.rigged = cmds.getAttr(node+".rigged")
        instance.moduleListString = cmds.getAttr(node+".moduleListString")
        spacemapString = cmds.getAttr(node+".spacemap")
        if not spacemapString:
            spacemapString = "{}"
        #dump it first to deal with old single quotes formatting
        spacemapString = json.dumps(spacemapString)
        instance.spacemap = json.loads(spacemapString, object_pairs_hook=OrderedDict)
        instance.loadModuleString()
        return instance


    @staticmethod
    def findAll():
        output = []
        nodes = RigNode.findNodes()
        for node in nodes:
            className = cmds.getAttr(node+".className")
            if className == CLASS_NAME:
                output.append(node)
        return output


    def __init__(self):
        self.instanceName  = "CharacterRig"
        self.node = None
        self.controlGroup = None
        #
        self.characterized = False
        self.rigged = False
        self.spacemap = []
        self.moduleListString = ""
        self.moduleList = []
        self.fullModuleList = []


    def loadModuleString(self):
        moduleNames = self.moduleListString.split(";")
        self.moduleList = []
        for name in moduleNames:
            if name:
                instance = RigNode.loadPlug(self.node, name)
                self.moduleList.append(instance)
        self.updateModuleList()


    def updateModuleList(self):
        self.fullModuleList = []
        for mod in self.moduleList:
            self.fullModuleList.append(mod)
            if mod.node:
                subnodes = RigNode.getSubNodes(mod.node)
                for node in subnodes:
                    instance = RigNode.load(node)
                    self.fullModuleList.append(instance)


    def addModule(self, instance):
        self.moduleList.append(instance)
        self.updateModuleList()


    def getModuleByName(self, name):
        output = None
        for mod in self.fullModuleList:
            if mod.instanceName == name:
                output = mod
                break
        return output


    def getKeyable(self):
        output = []
        for mod in self.fullModuleList:
            kobjs = mod.getKeyable()
            output += kobjs
        return output


    #
    # Character Setup Functions
    #
    def createNode(self):
        nodeBuilder = RigNode.Builder(self.instanceName+"_RN")
        nodeBuilder.addAttr("moduleListString", "string")
        nodeBuilder.addAttr("spacemap", "string")
        nodeBuilder.addAttr("controlGroup", "message")
        # Add Plugs for each core module
        for mod in self.moduleList:
            nodeBuilder.addAttr(mod.instanceName, "message")
        self.node = nodeBuilder.write()
        # Set RigNode Info
        cmds.setAttr(self.node+".rigType", RIG_TYPE, type="string")
        cmds.setAttr(self.node+".modulePath", MODULE_PATH, type="string")
        cmds.setAttr(self.node+".className", CLASS_NAME, type="string")
        cmds.setAttr(self.node+".instanceName", self.instanceName, type="string")
        # Connect Plugs for each core module
        self.moduleListString = ""
        for mod in self.moduleList:
            #
            mod.createNode()
            RigNode.setPlug(self.node, mod.instanceName, mod.node)
            #
            if len(self.moduleListString) > 0:
                self.moduleListString += ";"
            self.moduleListString += mod.instanceName
        cmds.setAttr(self.node+".moduleListString", self.moduleListString, type="string")
        cmds.setAttr(self.node+".spacemap", str(self.spacemap), type="string")
        self.updateModuleList()


    def characterize(self):
        self.controlGroup = cmds.createNode("transform", n=self.instanceName)
        for instance in self.moduleList:
            instance.characterize(p=self.controlGroup)
        # Set Rest Position
        self.setRestPosition()
        # Set RigNode Values
        RigNode.setPlug(self.node, "controlGroup", self.controlGroup)
        # Set Characterized Flag
        self.characterized = True
        cmds.setAttr(self.node+".characterized", True)
        self.updateModuleList()


    def rig(self):
        # Start Building Contorl Rig
        # Build Individual Control Rigs
        print("\nBuilding Control Rig")
        for instance in self.moduleList:
            if instance:
                instance.rig()
                print("|- "+instance.instanceName)
        # Connect Root Space to Placement Controller
        placement = self.getModuleByName("placement")
        if placement:
            for instance in self.moduleList:
                if instance.rootSpace:
                    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement, mo=True)
                    
        #set mesh shapes unselectable in viewport
        root = cmds.ls("root", r=1)[0]
        joints = cmds.listRelatives(root, ad=1, type="joint", fullPath=1)
        joints.append(root)
        skinClusters = list(set(cmds.listConnections(joints, type="skinCluster")))
        geo = []
        for sc in skinClusters:
            shapes = cmds.skinCluster(sc, q=1,g=1)
            for shape in shapes:
                if shape not in geo:
                    geo.append(shape)
        for shape in geo:
            #drawing override set to reference
            cmds.setAttr(shape+".overrideEnabled", 1) #enable
            cmds.setAttr(shape+".overrideDisplayType", 2) #reference
        
        for joint in joints:
            # set drawing override to reference
            cmds.setAttr(joint+".overrideEnabled", 1) #enable
            cmds.setAttr(joint+".overrideDisplayType", 2) #reference
            #Hide display of local axes on joints
            cmds.setAttr(joint+".displayLocalAxis", 0)
                    
        # Set Rigged Flag
        self.rigged = True
        cmds.setAttr(self.node+".rigged", True)
        print("Rigging Complete!")


    #
    # Animation Control Functions
    #
    def sync(self, node, placementMode, startFrame):
        target = Character.load(node)
        # start with placement - this affects world position
        targetPlacement = target.getModuleByName("placement")
        modPlacement = ''
        if targetPlacement:
            modPlacement = self.getModuleByName("placement")
            if modPlacement:
                #print(placementMode + " from character.sync")
                modPlacement.sync(node=targetPlacement.node, mode=placementMode, startFrame=startFrame)
        # Then with torso - this affect world position
        targetTorso = target.getModuleByName("torso")
        modTorso = ''
        if targetTorso:
            modTorso = self.getModuleByName("torso")
            if modTorso:
                modTorso.sync(node=targetTorso.node)
        # do the rest of the list
        for targetMod in target.moduleList:
            mod = self.getModuleByName(targetMod.instanceName)
            if mod:
                if mod != modPlacement and mod != modTorso:
                    mod.sync(node=targetMod.node)


    def keyAll(self):
        for instance in self.moduleList:
            if instance:
                instance.keyAll()


    def setRestPosition(self, *args, **kwargs):
        for mod in self.fullModuleList:
            mod = RigNode.load(mod.node)
            mod.setRestPosition()


    def transferAnimation(self, sourceCharacterNode, **kwargs):
        #Get KWargs
        startFrame = kwargs.get("startFrame", cmds.playbackOptions(q=True, min=True))
        endFrame = kwargs.get("endFrame", cmds.playbackOptions(q=True, max=True))
        placementMode = kwargs.get("placementMode", "root")
        #print(placementMode + " from transferAnimation")
        #Bake Pose Matching Over Time
        if startFrame == "inf" or startFrame == "-inf" or endFrame == "inf" or endFrame == "-inf":
            import maya.mel as mel
            mel.eval("source TimeSliderMenu.mel;")
            mel.eval("setPlaybackRangeToMinMax;")
            startFrame = cmds.playbackOptions(q=True, min=True)
            endFrame = cmds.playbackOptions(q=True, max=True)
        print("start: ", startFrame, " end: ", endFrame)
        for i in range(int(startFrame), int(endFrame+1)):
            cmds.currentTime(i)
            if i == int(startFrame):
                self.sync(sourceCharacterNode, placementMode.replace("FirstFrame", ""), startFrame)
            else:
                self.sync(sourceCharacterNode, placementMode, startFrame)
            self.keyAll()


    def getBoneMap(self, *args, **kwargs):
        boneMap = []
        for mod in self.fullModuleList:
            mod = RigNode.load(mod.node)
            name = str(mod.instanceName)
            boneList = mod.getBoneList()
            for i in range(0, len(boneList)):
                boneList[i] = RigUtility.shortNameOf(boneList[i])
                boneList[i] = str(boneList[i])
            boneMap.append( (name, boneList))
        return boneMap


    def getMarkerOffsetMap(self, *args, **kwargs):
        markerOffsetMap = []
        for mod in self.fullModuleList:
            mod = RigNode.load(mod.node)
            name = str(mod.instanceName)
            markerOffsetList = []
            for marker in mod.getMarkerList():
                markerOffset = cmds.xform(marker, q=True, os=True, ro=True)
                markerOffsetList.append(markerOffset)
            markerOffsetMap.append((name, markerOffsetList))
        return markerOffsetMap