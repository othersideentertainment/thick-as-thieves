import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya


def calculatePointInLocalSpace(worldPoint, transform):
    posA = cmds.xform(transform, q=True, ws=True, t=True)
    matrix = OpenMaya.MMatrix( cmds.xform(transform, q=True, ws=True, m=True) )
    posB =[ worldPoint[0]-posA[0], worldPoint[1]-posA[1], worldPoint[2]-posA[2]]
    return matrix * OpenMaya.MVector(posB) 
    
    
def getRelatedVertices(mesh, skin, influence):
    vertexCount = cmds.polyEvaluate(mesh, vertex = True)
    vertexList = []
    for i in range(0,vertexCount):
        vertexName = mesh+".vtx["+str(i)+"]"
        val = cmds.skinPercent(skin, vertexName, transform=influence, q=True)
        if val > 0:
            vertexList.append(i)
    return vertexList


def distributeSkin(mesh, skin, infSetList, axis=0):
    cmds.skinCluster(skin, e=True, nw=0)
    for infSet in infSetList:
        inf = infSet[0]
        inf_L = infSet[1]
        inf_R = infSet[2]
        min = float("inf")
        max = float("-inf")
        vertexList = getRelatedVertices(mesh, skin, inf)
        for v in vertexList:
            vertex = mesh+".vtx["+str(v)+"]"
            pos = cmds.pointPosition(vertex, w=True)
            pos = calculatePointInLocalSpace(pos, inf)
            if pos[axis] < min:
                min = pos[axis]
            if pos[axis] > max:
                max = pos[axis]

        for v in vertexList:
            vertex = mesh+".vtx["+str(v)+"]"
            pos = cmds.pointPosition(vertex, w=True)
            pos = calculatePointInLocalSpace(pos, inf)
            np = (pos[axis]-min) / (max-min)
            value = cmds.skinPercent(skin, vertex, transform=inf, q=True)
            value_R = np * value
            value_L = (1-np) * value
            cmds.skinPercent(skin, vertex, transformValue=[(inf,0), (inf_L, value_L), (inf_R, value_R)])
    cmds.skinCluster(skin, e=True, fnw=True)
    

#DEMO USAGE    
skin = "skinCluster1"
mesh = "mesh"
#BoneA - Mirror -X / +X
infSetList = [ ["boneA", "boneA_L", "boneA_R"] ]
distributeSkin(mesh, skin, infSetList, 2)
#BoneB - Mirror -Z / +Z
infSetList = [ ["boneB", "boneB_L", "boneB_R"] ]
distributeSkin(mesh, skin, infSetList, 0)