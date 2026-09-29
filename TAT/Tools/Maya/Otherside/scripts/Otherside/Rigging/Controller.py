import math
import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import Otherside.Rigging.RigUtility as RigUtility


class Color():
    CYAN = [0, 1, 1]
    YELLOW = [1, 1, 0]
    RED = [1, 0, 0]
    GREEN = [0, 1, 0]
    BLUE = [0, 0, 1]
    WHITE = [1, 1, 1]
    PINK = [.8, .25, .8]


class Shape():
    SPHERE = 1
    CYLINDER = 2
    CIRCLE = 3
    PLANE = 4
    BOX = 5
    TRIANGLE = 6


def buildName(name, side, type):
    elements = []
    elements.append(name)
    if side == 1:
        elements.append("L")
    elif side == 2:
        elements.append("R")
    if type != None and len(type) > 0:
        elements.append(type)
    output = ""
    for i in range(0, len(elements)):
        if i == 0:
            output = elements[i]
        else:
            output = output+"_"+elements[i]
    return output


def isController(transform):
    if cmds.objExists(transform+".controller"):
        return True
    else:
        return False


def create(**kwargs):
    #load options
    name = kwargs.get("name", "controller")
    parent = kwargs.get("parent", None)
    #Parse Complex
    if "size" in kwargs:
        kwargs["sizeX"] = kwargs["size"][0]
        kwargs["sizeY"] = kwargs["size"][1]
        kwargs["sizeZ"] = kwargs["size"][2]
    if "normal" in kwargs:
        kwargs["normalX"] = kwargs["normal"][0]
        kwargs["normalY"] = kwargs["normal"][1]
        kwargs["normalZ"] = kwargs["normal"][2]
    if "offset" in kwargs:
        kwargs["offsetX"] = kwargs["offset"][0]
        kwargs["offsetY"] = kwargs["offset"][1]
        kwargs["offsetZ"] = kwargs["offset"][2]
    #Set Options
    shape = kwargs.get("shape", Shape.SPHERE)
    color = kwargs.get("color", Color.CYAN)
    sizeX = kwargs.get("sizeX", 1)
    sizeY = kwargs.get("sizeY", 1)
    sizeZ = kwargs.get("sizeZ", 1)
    offsetX = kwargs.get("offsetX", 0)
    offsetY = kwargs.get("offsetY", 0)
    offsetZ = kwargs.get("offsetZ", 0)
    normalX = kwargs.get("normalX", 1)
    normalY = kwargs.get("normalY", 0)
    normalZ = kwargs.get("normalZ", 0)
    paddingIn = kwargs.get("paddingIn", 0)
    paddingOut = kwargs.get("paddingOut", 0)
    #create scene object
    transform = cmds.createNode("transform", n=name)
    if parent != None:
        transform = cmds.parent(transform, parent)[0]
    #add attributes
    cmds.addAttr(transform, ln="controller", at="message")
    cmds.addAttr(transform, ln="rigParent", at="message")
    cmds.addAttr(transform, ln="shape", at="long", dv=shape)
    cmds.addAttr(transform, ln="colorR", at="float", dv=color[0])
    cmds.addAttr(transform, ln="colorG", at="float", dv=color[1])
    cmds.addAttr(transform, ln="colorB", at="float", dv=color[2])
    #
    cmds.addAttr(transform, ln="size", at="double3")
    cmds.addAttr(transform, parent="size", ln="sizeX", at="double", dv=sizeX)
    cmds.addAttr(transform, parent="size", ln="sizeY", at="double", dv=sizeY)
    cmds.addAttr(transform, parent="size", ln="sizeZ", at="double", dv=sizeZ)
    #
    cmds.addAttr(transform, ln="offset", at="double3")
    cmds.addAttr(transform, parent="offset", ln="offsetX", at="double", dv=offsetX)
    cmds.addAttr(transform, parent="offset", ln="offsetY", at="double", dv=offsetY)
    cmds.addAttr(transform, parent="offset", ln="offsetZ", at="double", dv=offsetZ)
    #
    cmds.addAttr(transform, ln="normal", at="double3")
    cmds.addAttr(transform, parent="normal", ln="normalX", at="double", dv=normalX)
    cmds.addAttr(transform, parent="normal", ln="normalY", at="double", dv=normalY)
    cmds.addAttr(transform, parent="normal", ln="normalZ", at="double", dv=normalZ)
    #
    cmds.addAttr(transform, ln="padding", at="double2")
    cmds.addAttr(transform, parent="padding", ln="paddingIn", at="double", dv=paddingIn)
    cmds.addAttr(transform, parent="padding", ln="paddingOut", at="double", dv=paddingOut)
    #rebuild
    rebuild(transform)
    #return
    return transform


def readOptions(transform):
    options = {}
    options["shape"] = cmds.getAttr(transform+".shape")
    options["colorR"] = cmds.getAttr(transform+".colorR")
    options["colorG"] = cmds.getAttr(transform+".colorG")
    options["colorB"] = cmds.getAttr(transform+".colorB")
    #
    options["sizeX"] = cmds.getAttr(transform+".sizeX")
    options["sizeY"] = cmds.getAttr(transform+".sizeY")
    options["sizeZ"] = cmds.getAttr(transform+".sizeZ")
    #
    options["offsetX"] = cmds.getAttr(transform+".offsetX")
    options["offsetY"] = cmds.getAttr(transform+".offsetY")
    options["offsetZ"] = cmds.getAttr(transform+".offsetZ")
    #
    options["normalX"] = cmds.getAttr(transform+".normalX")
    options["normalY"] = cmds.getAttr(transform+".normalY")
    options["normalZ"] = cmds.getAttr(transform+".normalZ")
    #
    options["paddingIn"] = cmds.getAttr(transform+".paddingIn")
    options["paddingOut"] = cmds.getAttr(transform+".paddingOut")
    return options


def rebuild(controller):
    org_selection = cmds.ls(sl=True, fl=True)
    #Destroy Previous Control Curves
    curves = cmds.listRelatives(controller, s=True)
    if curves != None and len(curves) > 0:
        cmds.delete(curves)
    shape = cmds.getAttr(controller+".shape")
    #Build New Controller
    if shape == Shape.CYLINDER:
        createCylinder(controller)
    elif shape == Shape.PLANE:
        createPlane(controller)
    elif shape == Shape.CIRCLE:
        createCircle(controller)
    elif shape == Shape.SPHERE:
        createSphere(controller)
    elif shape == Shape.BOX:
        createBox(controller)
    elif shape == Shape.TRIANGLE:
        createTriangle(controller)
    else:
        createSphere(controller)
    #Set color
    color = [1,1,1]
    color[0] = cmds.getAttr(controller+".colorR")
    color[1] = cmds.getAttr(controller+".colorG")
    color[2] = cmds.getAttr(controller+".colorB")
    RigUtility.setOverrideColor(controller, color)
    #reselect original selection
    cmds.select(org_selection, r=True)


def adjustPointByNormal(point, normal):
    defaultNormal = [0,1,0]
    angles = cmds.angleBetween(euler=True, v1=normal, v2=defaultNormal)
    vector = OpenMaya.MVector(point)
    euler = OpenMaya.MEulerRotation( math.radians(angles[0]), math.radians(angles[1]), math.radians(angles[2]))
    output= vector.rotateBy(euler)
    return output


def getCVCount(curveObj):
    curveObj = ""
    numSpans = cmds.getAttr(curveObj + ".spans")
    degree = cmds.getAttr(curveObj + ".degree")
    form = cmds.getAttr(curveObj + ".form")
    cvCount = numSpans + degree
    if form == 2:
        cvCount = cvCount - degree
    return cvCount


def clampToShapeToFloor(curveObj, floorHeight = 0):
    cvCount = getCVCount(curveObj)
    for i in range(0,cvCount):
        cv = curveObj+".cv["+str(i)+"]"
        pos = cmds.xform(cv, q=True, ws=True, t=True)
        if pos[1] < floorHeight:
            cmds.xform(cv, ws=True, t=[pos[0], floorHeight, pos[2]])

def moveCurve(curveObj, delta, *args, **kwargs):    
    orgPos = cmds.xform(curveObj, q=True, ws=True, rp=True)
    position = [ orgPos[0] +delta[0], orgPos[1] +delta[1], orgPos[2] +delta[2] ] 
    cmds.xform(curveObj, ws=True, t=position)
    cmds.xform(curveObj, ws=True, piv=orgPos)
    


############# SHAPES #############
def createCylinder(transform):
    options = readOptions(transform)
    radius = options["sizeZ"] * .5
    length = options["sizeX"]
    #X
    p1 = (0, 0, -radius)
    p2 = (length, 0, -radius)
    p3 = (length, 0, radius)
    p4 = (0, 0, radius)
    temp = cmds.curve(d=1, p=[p1, p2, p3,p4, p1])
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)
    #Z
    p1 = (0, -radius, 0)
    p2 = (length, -radius, 0)
    p3 = (length, radius, 0)
    p4 = (0, radius, 0)
    temp = cmds.curve(d=1, p=[p1, p2, p3,p4, p1])
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)
    #Caps - 0
    temp = cmds.circle(c=(0,0,0), nr=(1,0,0), r=radius)
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)
    #Cap - 1
    temp = cmds.circle(c=(length, 0,0), nr=(1,0,0), r=radius)
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)


def createPlane(transform):
    options = readOptions(transform)
    normal = [options.get("normalX"), options.get("normalY"), options.get("normalZ")]
    pointList = [
        [-.5, 0, -.5],
        [.5, 0, -.5],
        [.5, 0, .5],
        [-.5, 0, .5],
        [-.5, 0, -.5]]
    for i in range(0,len(pointList)):
        p = pointList[i]
        p[0] = p[0] * options["sizeX"] + options["offsetX"]
        p[1] = p[1] * options["sizeY"] + options["offsetY"]
        p[2] = p[2] * options["sizeZ"] + options["offsetZ"]
        p = adjustPointByNormal(p, normal)
        pointList[i] = p
    temp = cmds.curve(d=1, p=pointList)
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)


def createCircle(transform):
    options = readOptions(transform)
    radius = options["sizeZ"] * .5
    normal = (options["normalX"], options["normalY"], options["normalZ"])
    offset = [options["offsetX"], options["offsetY"], options["offsetZ"]]
    temp = cmds.circle(r=radius, nr=normal)
    shape = cmds.listRelatives(temp, s=True)
    #apply main offset
    cmds.move(offset[0], offset[1], offset[2], temp[0]+".cv[0:7]", r=True, ws=True, wd=True, xyz=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)


def createSphere(transform):
    options = readOptions(transform)
    radius = options["sizeZ"] * .5
    offset = [options["offsetX"], options["offsetY"], options["offsetZ"]]
    #x
    temp = cmds.circle(r=radius, nr=(1,0,0))
    shape = cmds.listRelatives(temp, s=True)
    cmds.move(offset[0], offset[1], offset[2], temp[0]+".cv[0:7]", r=True, ws=True, wd=True, xyz=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)
    #y
    temp = cmds.circle(r=radius, nr=(0,1,0))
    shape = cmds.listRelatives(temp, s=True)
    cmds.move(offset[0], offset[1], offset[2], temp[0]+".cv[0:7]", r=True, ws=True, wd=True, xyz=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)
    #z
    temp = cmds.circle(r=radius, nr=(0,0,1))
    shape = cmds.listRelatives(temp, s=True)
    cmds.move(offset[0], offset[1], offset[2], temp[0]+".cv[0:7]", r=True, ws=True, wd=True, xyz=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)


def createBox(transform):
    options = readOptions(transform)
    pointList = [
        [0.5,   0.5, -0.5],
        [0.5,   0.5,  0.5],
        [-0.5,  0.5,  0.5],
        [-0.5,  0.5, -0.5],
        [0.5,   0.5, -0.5],
        [0.5,  -0.5, -0.5],
        [0.5,  -0.5,  0.5],
        [0.5,   0.5,  0.5],
        [0.5,  -0.5,  0.5],
        [-0.5, -0.5,  0.5],
        [-0.5,  0.5,  0.5],
        [-0.5, -0.5,  0.5],
        [-0.5, -0.5, -0.5],
        [-0.5,  0.5, -0.5],
        [-0.5, -0.5, -0.5],
        [0.5,  -0.5, -0.5]]
    for i in range(0,len(pointList)):
        p = pointList[i]
        p[0] = p[0] * options["sizeX"] + options["offsetX"]
        p[1] = p[1] * options["sizeY"] + options["offsetY"]
        p[2] = p[2] * options["sizeZ"] + options["offsetZ"]
        pointList[i] = p
    temp = cmds.curve(d=1, p=pointList)
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)


def createTriangle(transform):
    options = readOptions(transform)
    pointList = [
        [1, 0, -.65],
        [0, 0, 1.35],
        [-1, 0, -.65],
        [1, 0, -.65]
        ]
    for i in range(0,len(pointList)):
        p = pointList[i]
        p[0] = p[0] * options["sizeX"] + options["offsetX"]
        p[1] = p[1] * options["sizeY"] + options["offsetY"]
        p[2] = p[2] * options["sizeZ"] + options["offsetZ"]
        pointList[i] = p
    temp = cmds.curve(d=1, p=pointList)
    shape = cmds.listRelatives(temp, s=True)
    cmds.parent(shape, transform, r=True, s=True)
    cmds.delete(temp)