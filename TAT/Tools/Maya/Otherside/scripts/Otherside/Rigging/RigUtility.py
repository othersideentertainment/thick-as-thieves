import math
import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya


#blend Joint Chain
def blendJointChain(target=[], inputA=[], inputB=[]):
    weightSwitch = createWeightSwitch()
    for i in range(0, len(target)):
        #Point Constraint
        pc = cmds.pointConstraint(inputA[i], target[i])[0]
        cmds.pointConstraint(inputB[i], target[i])
        weightList = cmds.pointConstraint(pc, q=True, wal=True)
        cmds.connectAttr(weightSwitch+".output", pc+"."+weightList[1], f=True)
        cmds.connectAttr(weightSwitch+".outputInverse", pc+"."+weightList[0], f=True)
        #Orient Constraint
        oc = cmds.orientConstraint(inputA[i], target[i])[0]
        cmds.orientConstraint(inputB[i], target[i])
        weightList = cmds.orientConstraint(oc, q=True, wal=True)
        cmds.connectAttr(weightSwitch+".output", oc+"."+weightList[1], f=True)
        cmds.connectAttr(weightSwitch+".outputInverse", oc+"."+weightList[0], f=True)
    return weightSwitch


#create Local Space
def createLocalSpace(name, obj, parent=None):
    space = cmds.createNode("transform", n=name, p=parent)
    cmds.matchTransform(space, obj)
    bakeOffsetParentMatrix(space)
    return space


#Create Weight Switch
def createWeightSwitch():
    node = cmds.createNode("plusMinusAverage", n="weightSwitch")
    cmds.setAttr(node+".operation", 2)
    cmds.setAttr(node+".input1D[0]", 1)
    cmds.addAttr(node, ln="input", at="float", min=0, max=1)
    cmds.addAttr(node, ln="output", at="float", min=0, max=1)
    cmds.addAttr(node, ln="outputInverse", at="float", min=0, max=1)
    cmds.connectAttr(node+".input", node+".input1D[1]", f=True)
    cmds.connectAttr(node+".input", node+".output", f=True)
    cmds.connectAttr(node+".output1D", node+".outputInverse", f=True)
    return node


def createPositiveNegativeSwitch():
    node = cmds.createNode("clamp", n="PositiveNegativeSwitch")
    cmds.addAttr(node, ln="inputValue", at="float")
    cmds.addAttr(node, ln="outputNegative", at= "float")
    cmds.addAttr(node, ln="outputPositive", at ="float")
    cmds.setAttr (node+".minR", -999)
    cmds.setAttr (node+".maxR", 0)
    cmds.setAttr (node+".minG", 0)
    cmds.setAttr (node+".maxG", 999)
    cmds.connectAttr(node+".inputValue", node+".inputR", f=True)
    cmds.connectAttr(node+".inputValue", node+".inputG", f=True)
    cmds.connectAttr(node+".outputR", node+".outputNegative", f=True)
    cmds.connectAttr(node+".outputG", node+".outputPositive", f=True)
    return node


#Bake Joint Orientation
def bakeJointOrientation(bone):
    m = cmds.xform(bone, q=True, ws=True, m=True)
    cmds.setAttr(bone+".jointOrient", 0,0,0)
    cmds.xform(bone, ws=True, m=m)
    rot = cmds.getAttr(bone+".rotate")[0]
    cmds.setAttr(bone+".rotate", 0,0,0)
    cmds.setAttr(bone+".jointOrient", rot[0], rot[1], rot[2])


#Bake Offset Parent Matrix
def bakeOffsetParentMatrix(transform):
    #identity matrix
    identityMatrix = m=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
    #get combine worldspace matrix
    worldspaceMatrix = cmds.xform(transform, q=True, ws=True, m=True)
    cmds.setAttr( transform+".opm", identityMatrix, type="matrix")
    cmds.xform(transform, ws=True, m=worldspaceMatrix)
    #get objectspace matrix and bake to offset parent matrix
    objectSpaceMatrix = cmds.xform(transform, q=True, os=True, m=True)
    cmds.setAttr(transform+".opm", objectSpaceMatrix, type="matrix")
    cmds.xform(transform, os=True, m=identityMatrix)


def calculateOffsetParentMatrix(transform):
    transform_parent = firstParentOf(transform)
    transform_world_matrix = OpenMaya.MMatrix( cmds.getAttr("{0}.worldMatrix[0]".format(transform)) )
    parent_world_inverse_matrix = OpenMaya.MMatrix()
    if transform_parent:
        parent_world_inverse_matrix = OpenMaya.MMatrix( cmds.getAttr("{0}.worldInverseMatrix[0]".format(transform_parent)) )
    return transform_world_matrix * parent_world_inverse_matrix


def calculatePoleVector(transformA, transformB, transformC, **kwargs):
    # kwargs
    multiplier = kwargs.get("multiplier", 1.1)
    defaultVectorDirection = kwargs.get("defaultVectorDirection", [0,0,1])
    minAngle = kwargs.get("minAngle", 1)
    # Get the points from provided transforms
    pointA = OpenMaya.MVector(cmds.xform(transformA, q=True, ws=True, rp=True))
    pointB = OpenMaya.MVector(cmds.xform(transformB, q=True, ws=True, rp=True))
    pointC = OpenMaya.MVector(cmds.xform(transformC, q=True, ws=True, rp=True))
    #
    vectorBA = (pointB - pointA)
    vectorCA = (pointC - pointA)
    distanceBA = vectorBA.length()
    distanceCA = vectorCA.length()
    dot = vectorBA.normalize() * vectorCA.normalize()
    angle = math.degrees(math.acos(dot))
    direction = OpenMaya.MVector(defaultVectorDirection)
    # Bent Chain - recalculate direction
    if (angle > minAngle):
        projection = dot / distanceCA
        projection_vector = (vectorCA.normalize() * projection).normalize()
        direction = (vectorBA - projection_vector).normalize()
    polevector = pointB + (direction * distanceBA * multiplier)
    return polevector


#Calculate Pole Vector
def calculatePoleVectorPoints(pointA, pointB, pointC, multiplier=3):
    pointA = OpenMaya.MVector(pointA)
    pointB = OpenMaya.MVector(pointB)
    pointC = OpenMaya.MVector(pointC)
    start_to_mid = (pointB - pointA)
    start_to_end = (pointC - pointA)
    dot = start_to_mid * start_to_end
    start_to_end_normalized = start_to_end.normal()
    projection = dot / start_to_end.length()
    projection_vector = start_to_end_normalized * projection
    arrow = start_to_mid - projection_vector
    arrow *= multiplier
    polevector = arrow + pointB
    return polevector


#Calculate Distance Between Transforms
def calculateDistanceBetweenTransforms(transformA, transformB):
    pointA = OpenMaya.MPoint(cmds.xform(transformA, q=True, ws=True, t=True))
    pointB = OpenMaya.MPoint(cmds.xform(transformB, q=True, ws=True, t=True))
    distance = pointA.distanceTo(pointB)
    return distance


#
def calculateDistanceToChildren(transform):
    children = cmds.listRelatives(transform, c=True)
    maxDistance = 0
    for child in children:
        distance = calculateDistanceBetweenTransforms(transform, child)
        if distance > maxDistance:
            maxDistance = distance
    return maxDistance


def calculateLengthOfBoneChain(start, end):
    boneChain = getBoneChain(start, end)
    length = 0
    for i in range(1, len(boneChain)):
        length += calculateDistanceBetweenTransforms(boneChain[i-1], boneChain[i])
    return length


#Clone Joint Chain
def cloneJointChain(boneList = [], prefix="clone_", parent=None):
    cloneList = []
    for bone in boneList:
        cloneBone = cmds.createNode("joint", n=(prefix+shortNameOf(bone)))
        cmds.matchTransform(cloneBone, bone)
        if len(cloneList) == 0:
            if parent != None:
                cloneBone = cmds.parent(cloneBone, parent)[0]
        else:
            cloneBone = cmds.parent(cloneBone, cloneList[-1])[0]
        bakeJointOrientation(cloneBone)
        cloneList.append(cloneBone)
    return cloneList


#
def convertMatrixToNode(inMatrix, **kwargs):
    nodeName = kwargs.get("n", "matrix")
    multMatrix = cmds.createNode("multMatrix", n=nodeName)
    cmds.setAttr(multMatrix+".matrixIn[1]", inMatrix, type="matrix")
    cmds.setAttr(multMatrix+".matrixIn[1]", l=True)
    return multMatrix


#First Parent Of
def firstParentOf(obj):
    output = None
    temp = cmds.listRelatives(obj, p=True, pa=True)
    if temp != None:
        output = temp[0]
    return output

#
def longNameOf(obj):
    return cmds.ls(obj, l=True)[-1]

#
def shortNameOf(obj):
    #return mel.eval("shortNameOf(\""+obj+"\")")
    name = obj.split("|")[-1]
    name = name.split(":")[-1]
    return name


#
def setOverrideColor(obj, color=[1,1,1]):
    cmds.setAttr(obj+".overrideEnabled", 1)
    cmds.setAttr(obj+".overrideRGBColors", True)
    cmds.setAttr(obj+".overrideColorR", color[0])
    cmds.setAttr(obj+".overrideColorG", color[1])
    cmds.setAttr(obj+".overrideColorB", color[2])
    setOutlinerColor(obj, color)


#
def setOutlinerColor(obj, color=[1,1,1]):
    cmds.setAttr(obj+".useOutlinerColor", 1)
    cmds.setAttr(obj+".outlinerColorR", float(color[0]/2.0) + .5)
    cmds.setAttr(obj+".outlinerColorG", float(color[1]/2.0) + .5)
    cmds.setAttr(obj+".outlinerColorB", float(color[2]/2.0) + .5)


#
def getAveragePosition(transformList=[]):
    count = len(transformList)
    total = [0,0,0]
    for transform in transformList:
        pos = cmds.xform(transform, q=True, ws=True, t=True)
        total[0] += pos[0]
        total[1] += pos[1]
        total[2] += pos[2]
    total[0] = total[0]/float(count)
    total[1] = total[1]/float(count)
    total[2] = total[2]/float(count)
    return total


#
def movePivot(transform, position):
    cmds.move(position[0], position[1], position[2],
    transform+".scalePivot",
    transform+".rotatePivot",
    absolute=True)


#
def lookAtObject(sourceObj, targetObj, aim, up):
    temp = cmds.aimConstraint(targetObj, sourceObj,
        offset = [0, 0, 0],
        weight = 1,
        aimVector = aim,
        upVector = up,
        worldUpType = "vector",
        worldUpVector = [0, 1, 0])
    cmds.delete(temp)


#
def calculateLookAt(pointA, pointB, aim, up):
    markerA = cmds.createNode("transform", n="markerA")
    markerB = cmds.createNode("transform", n="markerB")
    cmds.xform(markerA, ws=True, t=pointA)
    cmds.xform(markerB, ws=True, t=pointB)
    lookAtObject(markerA, markerB, aim, up)
    rot = cmds.xform(markerA, q=True, ws=True, ro=True)
    cmds.delete(markerA,markerB)
    return rot

#
def getDagPath(dagObj):
    sel = OpenMaya.MSelectionList()
    sel.add(dagObj)
    dagPath = sel.getDagPath(0)
    return dagPath

#
def aim(source, **kwargs):
    # Get Keyword Args
    target = kwargs.get("target", None)
    targetPoint = kwargs.get("targetPoint", None)
    aimAxis = kwargs.get("aimAxis", [1,0,0])
    upAxis = kwargs.get("upAxis", [0,1,0])
    worldUpAxis = kwargs.get("worldUpAxis", [0,1,0])
    # Convert Axis Lists to MVectors
    aimAxis = OpenMaya.MVector(aimAxis[0], aimAxis[1], aimAxis[2])
    secondaryAxis = OpenMaya.MVector(upAxis[0], upAxis[1], upAxis[2])
    secondaryAxisWorld = OpenMaya.MVector(worldUpAxis[0], worldUpAxis[1], worldUpAxis[2])
    # Get the Source Object's Position based off its RotatePivot
    sourceDag = getDagPath(source)
    transformFn = OpenMaya.MFnTransform(sourceDag)
    sourcePivot = transformFn.rotatePivot(OpenMaya.MSpace.kWorld)
    # Get the Target Object's Position
    targetPivot = OpenMaya.MPoint()
    # | Using Target Object's Position based off its RotatePivot
    if target:
        targetDag = getDagPath(target)
        transformFn = OpenMaya.MFnTransform(targetDag)
        targetPivot = transformFn.rotatePivot(OpenMaya.MSpace.kWorld)
    # | Using Provided Point (TargetPoint)
    elif targetPoint:
        targetPivot = OpenMaya.MPoint(targetPoint[0], targetPoint[1],  targetPoint[2] )
    #Calculate Axis Vectors
    aimVector = (targetPivot - sourcePivot)
    coordU = aimVector.normal()
    coordV = secondaryAxisWorld
    coordW = (coordU ^ coordV).normal()
    coordV = (coordW ^ coordU)
    # Build Quaternion for the Aim rotation
    quaternion = OpenMaya.MQuaternion()
    quaternionU = OpenMaya.MQuaternion(aimAxis, coordU)
    quaternion = quaternionU
    # Build Quaternion for the Up Rotation
    upRotated = secondaryAxis.rotateBy(quaternion)
    value = (upRotated * coordV)
    if  value < -1:
        value = -1
    if value > 1:
        value = 1
    angle = math.acos(value)
    angle = math.acos(value)
    quaternionV = OpenMaya.MQuaternion(angle, coordU)
    # | flip if needed
    if not coordV.isEquivalent(upRotated.rotateBy(quaternionV), 1.0e-5):
        angle = (2*math.pi) - angle
        quaternionV = OpenMaya.MQuaternion(angle, coordU)
    # Final Quaternion
    quaternion *= quaternionV
    # Apply the transform
    transformFn.setObject(sourceDag)
    transformFn.setRotation(quaternion, OpenMaya.MSpace.kWorld)

#
def modifyTransformChannels(transform, **kwargs):
    if transform==None:
        return
    #
    translate = kwargs.get("t", True)
    rotate = kwargs.get("r", True)
    scale = kwargs.get("s", True)
    visibility = kwargs.get("v", True)
    #
    lock = kwargs.get("l", True)
    hide = kwargs.get("h", True)
    showCB = not hide
    #
    setList = [ [ translate, ["tx", "ty", "tz"]],
            [ rotate, ["rx", "ry", "rz"] ],
            [ scale, ["sx", "sy", "sz"] ],
            [ visibility, ["v"] ] ]
    #
    for set in setList:
        perform_set = set[0]
        attrList = set[1]
        if perform_set:
            for attr in attrList:
                if cmds.objExists(transform+"."+attr):
                    cmds.setAttr(transform+"."+attr, l=lock, k=showCB, cb=showCB)


#
def lockChannels(transform, **kwargs):
    if transform==None:
        return
    #
    translate = kwargs.get("t", False)
    rotate = kwargs.get("r", False)
    scale = kwargs.get("s", False)
    visibility = kwargs.get("v", False)
    everything = kwargs.get("all", False)
    hide = kwargs.get("h", False)
    #
    if everything:
        translate = True
        rotate = True
        scale= True
        visibility = True
    showCB = not hide
    #
    groupList = [ [ translate, ["tx", "ty", "tz"]],
            [ rotate, ["rx", "ry", "rz"] ],
            [ scale, ["sx", "sy", "sz"] ],
            [ visibility, ["v"] ] ]
    #
    for group in groupList:
        perforce = group[0]
        attrList = group[1]
        if perforce:
            for attr in attrList:
                if cmds.objExists(transform+"."+attr):
                    cmds.setAttr(transform+"."+attr, l=True, k=showCB, cb=showCB)



#
def findObjectInHierarchySelected(objName):
    sel = cmds.ls(sl=True, fl=True, l=True)
    if sel == None and len(sel) > 0:
        return None
    root = sel[0]
    return findObjectInHierarchy(objName, root)


#
def findObjectInHierarchy(objName, root):
    output = None
    hierarchy = []
    if root:
        hierarchy = cmds.listRelatives(root, ad=True, f=True)
        hierarchy.append(root)
    else:
        hierarchy = cmds.ls(dag=True, fl=True, l=True)
    #
    for hier in hierarchy:
        hier = str(hier)
        shortName = hier.split("|")[-1]
        shortName = shortName.split(':')[-1]
        if str(objName) == str(shortName):
            output = hier
            break
    return output

#
def calculatePlaneNormalTransform(objA, objB, objC):
    pointA = cmds.xform(objA, q=True, ws=True, t=True)
    pointB = cmds.xform(objB, q=True, ws=True, t=True)
    pointC = cmds.xform(objC, q=True, ws=True, t=True)
    return calculatePlaneNormal(pointA, pointB, pointC)


#
def calculatePlaneNormal(pointA, pointB, pointC):
    v1 = [
        pointA[0] - pointB[0],
        pointA[1] - pointB[1],
        pointA[2] - pointB[2] ]
    v2 = [
        pointC[0] - pointB[0],
        pointC[1] - pointB[1],
        pointC[2] - pointB[2] ]
    return crossProduct(v1, v2)

#
def crossProduct(vectorA, vectorB):
    vectorA = OpenMaya.MVector(vectorA[0], vectorA[1], vectorA[2])
    vectorB = OpenMaya.MVector(vectorB[0], vectorB[1], vectorB[2])
    output = (vectorA ^ vectorB).normal()
    return output

#
def parentByMatrix(obj, target_obj, **kwargs):
    maintainOffset = kwargs.get("mo", False)
    parent_obj= firstParentOf(obj)
    multMatrix = cmds.createNode("multMatrix")
    if maintainOffset:
        obj_wm = OpenMaya.MMatrix( cmds.xform(obj, q=True, ws=True, m=True) )
        target_wm = OpenMaya.MMatrix( cmds.xform(target_obj, q=True, ws=True, m=True) )
        obj_om = obj_wm * target_wm.inverse()
        opmNode = convertMatrixToNode(obj_om)
        cmds.connectAttr(opmNode+".matrixSum", multMatrix+".matrixIn[0]")
    #
    cmds.connectAttr(target_obj + ".worldMatrix[0]", multMatrix+".matrixIn[1]")
    if parent_obj != None:
        cmds.connectAttr(parent_obj + ".worldInverseMatrix[0]", multMatrix+".matrixIn[2]")
    cmds.connectAttr(multMatrix+".matrixSum", obj + ".offsetParentMatrix")
    return multMatrix


def setupFKStretch(controller, stretch_attr):
    opm = calculateOffsetParentMatrix(controller)
    #
    decomp = cmds.createNode("decomposeMatrix")
    cmds.setAttr("{}.inputMatrix".format(decomp), opm, type="matrix")
    #
    multiplyDivide = cmds.createNode("multiplyDivide")
    cmds.connectAttr("{}.outputTranslate".format(decomp), "{}.input1".format(multiplyDivide))
    cmds.connectAttr(stretch_attr, "{}.input2X".format(multiplyDivide))
    #
    comp = cmds.createNode("composeMatrix")
    cmds.connectAttr("{}.output".format(multiplyDivide), "{}.inputTranslate".format(comp))
    cmds.connectAttr("{}.outputRotate".format(decomp), "{}.inputRotate".format(comp))
    cmds.connectAttr("{}.outputScale".format(decomp), "{}.inputScale".format(comp))
    cmds.connectAttr("{}.outputShear".format(decomp), "{}.inputShear".format(comp))
    #
    cmds.connectAttr("{}.outputMatrix".format(comp), "{}.offsetParentMatrix".format(controller))


def listNearestTransforms(obj, transformList):
    objPoint = OpenMaya.MPoint(cmds.xform(obj, q=True, ws=True, t=True))
    output = [transformList[0]]
    while len(output ) < len(transformList):
        for i in range(1,len(transformList)):
            inserted= False
            for n in range (0, len(output)):
                transformPoint = OpenMaya.MPoint(cmds.xform(transformList[i], q=True, ws=True, t=True))
                outputPoint = OpenMaya.MPoint(cmds.xform(output[n], q=True, ws=True, t=True))
                if objPoint.distanceTo(transformPoint)< objPoint.distanceTo(outputPoint):
                    output.insert(n, transformList[i])
                    inserted = True
            if inserted == False:
                output.append(transformList[i])
    return output


def resetTransform(transform):
    cmds.xform(transform, os=True, m=[
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1])


def getBoneChain(startBone, endBone):
    chain = []
    startBone = cmds.ls(startBone, l=True)[-1]
    endBone = cmds.ls(endBone, l=True)[-1]
    if startBone == endBone[0:len(startBone)]:
        tail = endBone.replace(startBone, "")
        children = tail.split("|")
        bone = startBone
        chain.append(bone)
        for child in children:
            if child != "":
                bone = bone + "|" + child
                chain.append(bone)
    return chain


def convertListToString(list):
    output = "["
    for i in range(0,len(list)):
        if i == 0:
            output += "\""+list[i]+"\""
        else:
            output += ", \""+list[i]+"\""
    output += "]"
    return output


def convertStringToList(listString):
    list = []
    if listString:
        list = eval(listString)
    return list


def createCleanReference(fbx_file, **kwargs):
    '''
    Description: Creates a reference to a file removing all incoming namespaces.

    Optional Kwargs:
        'namespace' adds a custom namespace to the referenced content
        'group_name' (gn) adds a root transform over all imported transforms with the provided name.
    *note: if both flags are active the root transform will also receive the namespace
    '''
    namespace = kwargs.get("namespace", ":")
    group_name = kwargs.get("group_name",kwargs.get("gn", ""))
    group_reference = len(group_name) > 0
    # Get list of existing namespaces
    existing_nodes = cmds.ls(dag=True, l=True)
    existing_namespaces = cmds.namespaceInfo(listOnlyNamespaces=True, recurse=True)
    # Create the reference
    reference_node = cmds.file(fbx_file, r=True, namespace=namespace, gr=group_reference, gn=group_name)
    # HANDLE IMPORTED NAMESPACES
    all_namespaces = cmds.namespaceInfo(listOnlyNamespaces=True, recurse=True)
    new_namespaces = list(set(all_namespaces) - set(existing_namespaces))
    system_namespaces = ['UI', 'shared']
    user_namespaces = [ns for ns in new_namespaces if ns not in system_namespaces]
    # Remove the namespaces that were imported along with the new file and merge with the new namespace
    for ns in user_namespaces:
        if ns != namespace:
            cmds.namespace(removeNamespace=ns, mergeNamespaceWithOther=namespace)
    #HANDLE NEW NODES
    all_nodes = cmds.ls(dag=True, l=True)
    new_nodes = list(set(all_nodes)-set(existing_nodes))
    new_nodes = sorted(new_nodes, key=len, reverse=True)
    for node in new_nodes:
        short_name = node.split("|")[-1]
        if namespace not in short_name:
            path = node[:-len(short_name)]
            new_name = (path + namespace+":"+short_name)
            cmds.rename(node, new_name)
    #
    new_nodes = list(set(all_nodes)-set(existing_nodes))
    new_nodes = sorted(new_nodes, key=len, reverse=False)
    return reference_node


#
def getKeyframeRange(joint):
    # Get the joint's name
    joint_name = joint

    # Find the animation curves that are connected to the joint's transform attributes
    anim_curves = []
    for attr_name in ('translateX', 'translateY', 'translateZ', 'rotateX', 'rotateY', 'rotateZ', 'scaleX', 'scaleY', 'scaleZ'):
        attr = '%s.%s' % (joint, attr_name)
        sources = cmds.listConnections(attr, source=True, destination=False, type='animCurve')
        if sources:
            anim_curves.extend(sources)

    # Determine the range of keys for each animation curve
    keyframe_ranges = []
    for curve in anim_curves:
        start = cmds.findKeyframe(curve, time=(0,0), which='first')
        end = cmds.findKeyframe(curve, time=(0,0), which='last')
        if start == None or end == None:
            continue
        keyframe_ranges.append((start, end))

    # Determine the overall range of keys for the joint
    if len(keyframe_ranges) == 0:
        joint_range = None
    else:
        joint_range = (
            min([start_time for start_time, _ in keyframe_ranges]),
            max([end_time for _, end_time in keyframe_ranges])
        )

    return joint_range

#
def getKeyframeRangeHierarchy(root):
    min_time = float('inf')
    max_time = float('-inf')
    transforms = cmds.listRelatives(root, ad=True, f=True, type='transform')
    transforms.append(root)
    transforms = sorted(transforms, key=len)
    for transform in transforms:
        range = getKeyframeRange(transform)
        if range == None:
            continue
        if range[0] < min_time:
            min_time = range[0]
        if range[1] > max_time:
            max_time = range[1]
    return [min_time, max_time]