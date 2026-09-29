import maya.cmds as cmds
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.Controller as Controller
reload(RigUtility)
reload(Controller)


class SpaceSwitchResult():
    def __init__(self):
        self.spaceSwitchGroup = None
        self.channelList = []
        self.supportedSpacesString = ""


class SpaceSwitch():
    @staticmethod
    def Create(controller, bone):
        instance = SpaceSwitch()
        instance.controller = controller
        instance.bone = bone
        return instance

    def __init__(self):
        #inputs
        self.controller = None
        self.bone = None
        #
        self.pinned = True
        self.spaceList = []
        self.spaceSwitchGroup = None
        self.spaceSwitchTransform = None
        self.channelList = []

    def addTargetSpace(self, name, transform):
        if transform != None:
            self.spaceList.append([name, transform])

    def build(self, **kwargs):
        parent = kwargs.get("parent", None)
        name =  RigUtility.shortNameOf(self.controller)
        boneParent = RigUtility.firstParentOf(self.bone)
        controllerParent = RigUtility.firstParentOf(self.controller)
        self.chanelList = []
        #Setup Space Switch Group
        self.spaceSwitchGroup = cmds.createNode("transform", n=name+"_spaceSwitch", p=parent)
        cmds.addAttr(self.spaceSwitchGroup, ln="world", at="float", k=True, min=0, max=1, dv=0)
        RigUtility.lockChannels(self.spaceSwitchGroup, t=True, r=True, s=True, v=True, h=True)
        weightSumNode = cmds.createNode("plusMinusAverage", n=name+"WeightSum")
        oneMinusNode = cmds.createNode("floatMath", n=name+"_OneMinus")
        cmds.setAttr(oneMinusNode+".operation", 1)
        cmds.setAttr(oneMinusNode+".floatA", 1)
        clampNode = cmds.createNode("floatMath", n=name+"_ClampZero")
        cmds.setAttr(clampNode+".operation", 5)
        cmds.setAttr(clampNode+".floatA", 0)
        cmds.connectAttr(weightSumNode+".output1D", oneMinusNode+".floatB")
        cmds.connectAttr(oneMinusNode+".outFloat", clampNode+".floatB")
        cmds.connectAttr(clampNode+".outFloat", self.spaceSwitchGroup+".world")
        #create the local/world space markers
        self.spaceSwitchTransform = RigUtility.createLocalSpace(name+"_SpaceSwitch_Transform", self.bone, self.spaceSwitchGroup)
        localspace_marker = RigUtility.createLocalSpace(name+"_Marker_Local", self.controller, self.spaceSwitchGroup)
        worldspace_marker = RigUtility.createLocalSpace(name+"_Marker_World", self.controller, self.spaceSwitchGroup)
        if self.pinned:
            cmds.parentConstraint(boneParent , self.spaceSwitchTransform, mo=True, sr=["x","y","z"])
            cmds.parentConstraint(boneParent, localspace_marker, mo=True)
            cmds.pointConstraint(localspace_marker, worldspace_marker, mo=True)
            #constrain the world space marker
            master_contraint = cmds.orientConstraint(worldspace_marker, self.spaceSwitchTransform, mo=True)[0]
            wal = cmds.orientConstraint(master_contraint, q=True, wal=True)
            cmds.connectAttr(self.spaceSwitchGroup+".world", master_contraint+"."+wal[-1])
            #Add the Individual Spaces
            inputCount = 0
            for space in self.spaceList:
                attrName = space[0]
                target = space[1]
                cmds.addAttr(self.spaceSwitchGroup, ln=attrName, at="float", k=True, min=0, max=1, dv=0)
                cmds.connectAttr(self.spaceSwitchGroup+"."+attrName, weightSumNode+".input1D["+str(inputCount)+"]")
                target_marker = RigUtility.createLocalSpace(name+"_Marker_"+attrName, self.controller, self.spaceSwitchGroup)
                cmds.pointConstraint(localspace_marker, target_marker)
                cmds.orientConstraint(target, target_marker, mo=True)
                cmds.orientConstraint(target_marker, self.spaceSwitchTransform, mo=True)
                wal = cmds.orientConstraint(master_contraint, q=True, wal=True)
                cmds.connectAttr(self.spaceSwitchGroup+"."+attrName, master_contraint+"."+wal[-1])
                self.channelList.append(attrName)
                inputCount += 1
        else:
            cmds.parentConstraint(boneParent , self.spaceSwitchTransform, mo=True, sr=["x","y","z"])
            cmds.parentConstraint(boneParent, localspace_marker, mo=True)
            #constrain the world space marker
            master_contraint = cmds.parentConstraint(worldspace_marker, self.spaceSwitchTransform, mo=True)[0]
            wal = cmds.parentConstraint(master_contraint, q=True, wal=True)
            cmds.connectAttr(self.spaceSwitchGroup+".world", master_contraint+"."+wal[-1])
            #Add the Individual Spaces
            inputCount = 0
            for space in self.spaceList:
                attrName = space[0]
                target = space[1]
                cmds.addAttr(self.spaceSwitchGroup, ln=attrName, at="float", k=True, min=0, max=1, dv=0)
                cmds.connectAttr(self.spaceSwitchGroup+"."+attrName, weightSumNode+".input1D["+str(inputCount)+"]")
                target_marker = RigUtility.createLocalSpace(name+"_Marker"+attrName, self.controller, self.spaceSwitchGroup)
                cmds.parentConstraint(target, target_marker, mo=True)
                cmds.parentConstraint(target_marker, self.spaceSwitchTransform, mo=True)
                wal = cmds.parentConstraint(master_contraint, q=True, wal=True)
                cmds.connectAttr(self.spaceSwitchGroup+"."+attrName, master_contraint+"."+wal[-1])
                self.channelList.append(attrName)
                inputCount += 1
        #Create output Offset Parent Matrix Node
        multMatrix = cmds.createNode("multMatrix", n=name+"_SpaceSwitchMatrix")
        cmds.connectAttr(self.spaceSwitchTransform+".worldMatrix[0]", multMatrix+".matrixIn[0]")
        if controllerParent != None:
            cmds.connectAttr(controllerParent+".worldInverseMatrix[0]", multMatrix+".matrixIn[1]")
        cmds.connectAttr(multMatrix+".matrixSum", self.controller+".offsetParentMatrix")
        #Add Space Switchin Attributes to The Controller
        spaceListString = ""
        self.channelList.insert(0, "world")
        for attr in self.channelList:
            spaceListString += ","+attr
        cmds.addAttr(self.controller, ln="supportedSpaceList", dt="string")
        cmds.setAttr(self.controller+".supportedSpaceList", spaceListString, type="string")
        cmds.setAttr(self.controller+".supportedSpaceList", l=True)
        result = SpaceSwitchResult()
        result.spaceSwitchGroup = self.spaceSwitchGroup
        result.channelList = self.channelList
        result.supportedSpacesString = spaceListString
        return result
