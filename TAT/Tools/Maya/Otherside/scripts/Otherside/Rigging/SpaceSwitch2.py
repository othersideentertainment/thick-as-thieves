import maya.cmds as cmds
import Otherside.Rigging.RigUtility as RigUtility

def create(node, pivot):
    instance = SpaceSwitch2(node=node, pivot=pivot)
    return instance


class SpaceSwitch2():
    def __init__(self, **kwargs):
        #input
        self.node = kwargs.get("node", None)
        self.pivot = kwargs.get("pivot", None)
        self.spaceList = []
        self.defaultSpaceLabel = None
        #output
        self.group = None
        self.driverGroup = None
        self.resultTransform = None
        self.channelList = []


    def setDefaultSpace(self, label):
        self.defaultSpaceLabel =label


    def addSpaceMap(self, spacemap):
        for i in range(0, len(spacemap)):
            label = spacemap[i][0]
            target = findSpace(spacemap, label)
            if target:
                if target != self.node:
                    self.addSpace(label, target)

    def addSpace(self, label, target, defaultSpace=False):
        if self.defaultSpaceLabel == None or defaultSpace:
            self.defaultSpaceLabel = label
        self.spaceList.append((label, target))


    def addProxyChannels(self, objList=[], prefix=""):
        proxyAttrList = []
        for channel in self.channelList:
            proxyAttrLabel = prefix+channel.capitalize()
            proxyAttrList.append(proxyAttrLabel)
            for obj in objList:
                sourceAttr = str("{}.{}".format(self.group, channel))
                cmds.addAttr(obj, ln=proxyAttrLabel, proxy=sourceAttr)
        return proxyAttrList

    def addProxy(self, object, proxyAttr):
        cmds.addAttr(object, ln=proxyAttr, proxy="{}.space".format(self.group))


    def build(self, **kwargs):
        # get kwargs
        name = kwargs.get("name", "SpaceSwitch")
        parent = kwargs.get("parent", None)
        # space switch group
        self.group = cmds.createNode("transform", n=name, p=parent)
        lockAttrList = ["tx", "ty", "tz", "rx", "ry", "rz", "sx", "sy", "sz", "v"]
        for attr in lockAttrList:
            cmds.setAttr("{}.{}".format(self.group, attr), l=True, k=False, cb=False)
        # internal pivots/spaces if needed
        internalGroup = cmds.createNode("transform", n="internal", p=self.group)
        # setup space drivers
        self.driverGroup = cmds.createNode("transform", n="driverGroup", p=self.group)
        # result transform
        self.resultTransform = cmds.createNode("transform", n="resultTransform", p=self.group)
        cmds.matchTransform(self.resultTransform, self.node, pos=1, rot=1, scl=0)
        self.channelList = []
        if self.pivot:
            cmds.parentConstraint(self.pivot, self.resultTransform, sr=["x","y","z"], mo=True)
        #Add Switch Attribute
        labels = [space[0] for space in self.spaceList]
        cmds.addAttr(self.group, ln="space", at="enum", en=":".join(labels), k=True, dv=labels.index(self.defaultSpaceLabel))
        self.channelList = ["space"]
        #Build the individual space drivers
        for i in range(0,len(self.spaceList)):
            label = self.spaceList[i][0]
            transform = self.spaceList[i][1]
            defaultSpace = self.defaultSpaceLabel == label
            constraint = None
            weightAttr = None
            driver = cmds.createNode("transform", n=label, p=self.driverGroup)
            #FK Mode
            if self.pivot:
                cmds.matchTransform(driver, self.pivot, pos=1, rot=1, scl=0)
                cmds.pointConstraint(self.pivot, driver, mo=True)
                if transform:
                    cmds.orientConstraint(transform, driver, mo=True)
                constraint = cmds.orientConstraint(driver, self.resultTransform, mo=True)[-1]
                weightAttr = cmds.orientConstraint(constraint, q=True, wal=True)[-1]
            #IK Mode
            else:
                if transform:
                    transformName = RigUtility.shortNameOf(transform)
                    anchor = cmds.createNode("transform", n=(transformName+"_anchor"), p=internalGroup)
                    cmds.pointConstraint(transform, anchor)
                    cmds.orientConstraint(transform, anchor)
                    cmds.parentConstraint(anchor, driver, mo=True)
                constraint = cmds.parentConstraint(driver, self.resultTransform, mo=True)[-1]
                weightAttr = cmds.parentConstraint(constraint, q=True, wal=True)[-1]
            # Connect to Enum
            node = isEqualNode(
                inputAttr="{}.space".format(self.group),
                outputAttr="{}.{}".format(constraint, weightAttr),
                value=i
                )
        #Connect via offset parent matrix
        RigUtility.parentByMatrix(self.node, self.resultTransform, mo=True)



def isEqualNode(**kwargs):
    #
    nodeName = kwargs.get("name", "IsEqual")
    value = kwargs.get("value", 0)
    inputValue = kwargs.get("inputValue", 0)
    inputAttr = kwargs.get("inputAttr", None)
    outputAttr = kwargs.get("outputAttr", None)
    #
    node = cmds.createNode("condition", n=nodeName)
    cmds.addAttr(node, ln="input", at="float")
    cmds.addAttr(node,ln="output", at="float")
    cmds.setAttr("{}.input".format(node), inputValue)
    cmds.setAttr("{}.secondTerm".format(node), value)
    cmds.setAttr("{}.operation".format(node), 0)
    cmds.setAttr("{}.colorIfTrueR".format(node), 1)
    cmds.setAttr("{}.colorIfFalseR".format(node), 0)
    #
    cmds.connectAttr("{}.input".format(node), "{}.firstTerm".format(node))
    cmds.connectAttr("{}.outColorR".format(node), "{}.output".format(node))
    #
    if inputAttr:
        cmds.connectAttr(inputAttr, "{}.input".format(node))
    #
    if outputAttr:
        cmds.connectAttr("{}.output".format(node), outputAttr)
    return node


def findSpace(spacemap, label):
    output = None
    for i in range(0, len(spacemap)):
        if spacemap[i][0] == label:
            output = spacemap[i][1]
            break
    return output