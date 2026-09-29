import importlib
import maya.cmds as cmds
import Otherside.Rigging.Controller as Controller


ATTR_GROUP_NAME = "rigNode"

class Builder():
    def __init__(self, nodeName):
        self.nodeName = nodeName
        self.attributeList = []
        self.addAttr("rigParent", "message")
        self.addAttr("rigType", "string")
        self.addAttr("modulePath", "string")
        self.addAttr("className", "string")
        self.addAttr("characterized", "bool")
        self.addAttr("rigged", "bool")
        self.addAttr("instanceName", "string")

    def addAttr(self, attr, type):
        self.attributeList.append([attr, type])

    def write(self):
        node = cmds.createNode("network", n=self.nodeName)
        cmds.addAttr(node, ln=ATTR_GROUP_NAME, at="compound", nc=len(self.attributeList))
        for attr in self.attributeList:
            attrName = attr[0]
            attrType = attr[1]
            if attrType == "string":
                cmds.addAttr(node, ln=attrName, dt="string", p=ATTR_GROUP_NAME)
            elif attrType == "messageMulti":
                cmds.addAttr(node, ln=attrName, at="message", multi=True, p=ATTR_GROUP_NAME)
            else:
                cmds.addAttr(node, ln=attrName, at=attrType, p=ATTR_GROUP_NAME)
        return node


def load(node):
    if node == None:
        return None
    module_name = cmds.getAttr("{}.modulePath".format(node))
    class_name = cmds.getAttr("{}.className".format(node))
    method_name = "load"
    module = importlib.import_module(module_name)
    cls = getattr(module, class_name)
    method = getattr(cls, method_name)
    output = method(node)
    return output


def loadPlug(node, plug):
    if node:
        subNode = getPlug(node, plug)
        if subNode:
            if isRigNode(subNode):
                return load(subNode)
    return None


def findNodes(type=""):
    nodes  = cmds.ls(type="network")
    output = []
    for node in nodes:
        if isRigNode(node):
            if type == "" or isType(node, type):
                output.append(node)
    return output


def getRelated(transform):
    if cmds.objExists(transform+".rigParent"):
        connections = cmds.listConnections(transform+".rigParent", s=False, d=True)
        if connections != None:
            for c in connections:
                if cmds.objExists(c+".rigNode"):
                    return c
    return None


def isRigNode(node):
    return cmds.objExists(node+".rigNode")


def isType(node, nodeType):
    return nodeType == getNodeType(node)


def isClass(node, className):
    return getNodeClass(node) == className


def isSubRig(node):
    rigParent = getRelated(node)
    return rigParent != None


def getNodeClass(node):
    output = "INVALID"
    if isRigNode(node):
        output = cmds.getAttr(node+".className")
    return output


def getNodeType(node):
    output = "INVALID"
    if isRigNode(node):
        output = cmds.getAttr(node+".rigType")
    return output


def getSubNodes(node, **kwargs):
    allChildren = kwargs.get("allChildren", False)
    subnodes = []
    connections = cmds.listConnections(node, s=True, d=False)
    if connections:
        for con in connections:
            if isRigNode(con):
                subnodes.append(str(con))
                if allChildren:
                    subnodes += getSubNodes(con)
    return subnodes


def setPlug(node, plug, obj):
    if node == None or plug == None or obj == None:
        return
    clearPlug(node, plug)
    plugPath = node+"."+plug
    if cmds.objExists(plugPath):
        objPath = obj+".rigParent"
        if not cmds.objExists(objPath):
            cmds.addAttr(obj, ln="rigParent", at="message")
        cmds.connectAttr(objPath, plugPath, f=True)


def getPlug(node, plug):
    output = None
    con = None
    if cmds.objExists(node+"."+plug):
        con = cmds.listConnections(node+"."+plug, s=True, d=False)
    if con != None:
        output = cmds.ls(con[0], l=True)[-1]
    return output


def clearPlug(node, plug):
    plugPath = "{}.{}".format(node,plug)
    con = cmds.listConnections(plugPath, s=True, d=False, p=True)
    if con:
        cmds.disconnectAttr(con[-1], plugPath)


def getOrphans():
    output = []
    nodes = cmds.ls(type="network")
    for node in nodes:
        if isRigNode(node):
            connections = cmds.listConnections(node, s=True, d=True)
            if len(connections) == 0:
                output.append(node)
    return output



def getMultiPlug(node, plug):
    list = []
    plugPath = "{}.{}".format(node, plug)
    size = cmds.getAttr(plugPath, size=True)
    for i in range(0,size):
        connected = cmds.listConnections(plugPath+"["+str(i)+"]")
        if connected:
            bone = connected[-1]
            bone = cmds.ls(bone, l=True)[-1]
            list.append(str(bone))
    return list


def getRoot(obj):
    rootNode = None
    if isRigNode(obj):
        rootNode = obj
    parentNode = getRelated(obj)
    while parentNode != None:
        rootNode = parentNode
        parentNode = getRelated(rootNode)
    return rootNode