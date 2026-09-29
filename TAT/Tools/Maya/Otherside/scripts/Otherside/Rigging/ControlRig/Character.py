import maya.cmds as cmds
from functools import partial
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
        spacemap = cmds.getAttr(node+".spacemap")
        if not spacemap:
            spacemap = "[]"
        instance.spacemap = eval(spacemap)
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


    def rig(self):
        # Start Building Contorl Rig
        # Build Individual Control Rigs
        print("Rigging: "  + self.instanceName)
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
        # Set Rigged Flag
        self.rigged = True
        cmds.setAttr(self.node+".rigged", True)
        print("Rigging Complete!")


    #
    # Animation Control Functions
    #
    def sync(self, node):
        target = Character.load(node)
        # start with placement - this affects world position
        targetPlacement = target.getModuleByName("placement")
        if targetPlacement:
            mod = self.getModuleByName("placement")
            if mod:
                mod.sync(node=targetPlacement.node)
        # Then with torso - this affect world position
        targetTorso = target.getModuleByName("torso")
        if targetTorso:
            mod = self.getModuleByName("torso")
            if mod:
                mod.sync(node=targetTorso.node)
        # do the rest of the list
        for targetMod in target.moduleList:
            mod = self.getModuleByName(targetMod.instanceName)
            if mod:
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
        #Bake Pose Matching Over Time
        for i in range(int(startFrame), int(endFrame+1)):
            cmds.currentTime(i)
            self.sync(sourceCharacterNode)
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