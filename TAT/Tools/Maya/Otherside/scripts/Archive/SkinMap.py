import maya.cmds as cmds
import maya.mel as mel
import os
import re
from string import *


#Vertex Weights
class VertexWeight(object):
    def __init__(self, vertexID, boneWeights=[]):
        self.vertexID = vertexID
        self.boneWeights = boneWeights
    
    def asString(self):
        output = ""
        output = (str(self.vertexID))
        for bw in self.boneWeights:            
            output += ("\t"+str(bw))
        return output


#SkinInfo
class SkinInfo(object):
    def __init__(self, skinnedMesh):
        self.skinnedMesh = skinnedMesh
        self.skinCluster = mel.eval('findRelatedSkinCluster '+skinnedMesh)        
        self.influences = []
        self.vertexWeights = []
    
    
    def getInfluences(self):        
        self.influences = []
        self.influences = cmds.skinCluster(self.skinCluster, q=True, inf=True)        


    def getVertexWeights(self):
        self.vertexWeights = []        
        vertexCount = cmds.polyEvaluate(self.skinnedMesh, v=True)
        for vertexID in xrange(0,vertexCount):
            meshVertex = (self.skinnedMesh+".vtx["+str(vertexID)+"]")
            bones = cmds.skinPercent(self.skinCluster, meshVertex, ib=.01, q=True, t=None)
            weights = cmds.skinPercent(self.skinCluster, meshVertex, ib=.01, q=True, value=True)
            boneWeights = []
            for i in range (0, len(bones)):
                w = (str(bones[i]), round(weights[i], 3) )
                boneWeights.append(w)
            vertexWeight = VertexWeight(vertexID, boneWeights)
            self.vertexWeights.append(vertexWeight) 


    def asString(self):
        output = ""
        output += (self.skinnedMesh)
        output += ("\n"+self.skinCluster)
        for influence in self.influences:
            output += influence
        for vertexWeight in self.vertexWeights:            
            output += vertexWeight.asString()
        return output


def isSkinnedMesh(obj):
    sc = mel.eval("findRelatedSkinCluster "+obj) 
    return sc != None


def getTagContents(text, startTag, endTag):
    output = ""
    try:
        start = text.find(startTag) + len(startTag)
        end = text.find(endTag)
        output = text[start:end]    
    except:
        output = ""
    return output


def getLineItems(text, startTag, endTag):
    output = []
    try:
        start = text.find(startTag) + len(startTag)
        end = text.find(endTag)
        subtext = text[start:end]
        buffer = subtext.split('\n')
        for b in buffer:
            if b != None and b != "":
                output.append(b)
    except:
        output = []
    return output


def exportMap(skinnedMesh, skinMapFile):    
    #collect influences
    skinInfo = SkinInfo(skinnedMesh)
    skinInfo.getInfluences()
    skinInfo.getVertexWeights()            
    skinmap = open(skinMapFile,"w")     
    #Write Mesh name
    skinmap.write("[MESH]")
    skinmap.write("\n"+skinnedMesh)
    skinmap.write("\n[/MESH]\n")
    #write influence list
    skinmap.write("\n[INFLUENCE]")  
    for influence in skinInfo.influences:                
        skinmap.write("\n"+influence)
    skinmap.write("\n[/INFLUENCE]\n")  
    #write weight list
    skinmap.write("\n[WEIGHT]")  
    for vertexWeight in skinInfo.vertexWeights:
        skinmap.write("\n"+vertexWeight.asString().replace("\'",""))      
    skinmap.write("\n[/WEIGHT]\n")  
    #send complete message
    print ("export complete: "+skinMapFile)


def importMap(skinMapFile):
    #get skin map contents    
    file = open(skinMapFile, 'r')
    skinmap = file.read()                             
    #parse contents into sorted lists
    skinnedMesh = getLineItems(skinmap, "[MESH]","[/MESH]")
    if skinnedMesh == None:
        print "Error: No Mesh Defined in SkinMap"
        return
    skinnedMesh = skinnedMesh[0]

    #Check if mesh exists
    if not cmds.objExists(skinnedMesh):
        print "Error: Skinned mesh not found"
        return

    #check for existing skin cluster
    skinCluster = mel.eval('findRelatedSkinCluster '+skinnedMesh)
    if skinCluster == None or skinCluster == "":
        print "Error: No Skin Cluster Found"
        return    

    influenceList = getLineItems(skinmap, "[INFLUENCE]", "[/INFLUENCE]")            
    weightList = getLineItems(skinmap, "[WEIGHT]", "[/WEIGHT]")    
    #check for existing skin cluster
    skinCluster = mel.eval('findRelatedSkinCluster '+skinnedMesh)      
    if skinCluster != None and skinCluster != "":
        cmds.skinCluster(skinnedMesh, e=True, unbind=True)    
    skinCluster = cmds.skinCluster(influenceList, skinnedMesh, tsb=True)[0]        
    #access main progress bar
    gMainProgressBar = mel.eval('$tmp = $gMainProgressBar')
    cmds.progressBar( gMainProgressBar,
                    edit=True,
                    beginProgress=True,
                    isInterruptable=False,
                    status='Updating Weights: ' + skinnedMesh,
                    maxValue=len(weightList))        
    #update skin weights
    cmds.skinCluster(skinCluster, e=True, nw=0)
    cmds.skinPercent(skinCluster, skinnedMesh, pruneWeights=2)        
    for vertexWeight in weightList:
        buffer = vertexWeight.split("\t")
        if len(buffer) > 1:
            vertexID = (skinnedMesh + ".vtx["+buffer[0]+"]")             
            for i in  range(1,len(buffer)):
                rawContents = getTagContents(buffer[i], "(", ")")
                elements = rawContents.split(", ")
                inf = elements[0].replace("\'","")
                value = float(elements[1])
                cmds.skinPercent(skinCluster, vertexID, tv=(inf, value))        
        cmds.progressBar(gMainProgressBar, edit=True, step=1)
    cmds.skinCluster(skinCluster, e=True, fnw=True)
    cmds.skinCluster(skinCluster, e=True, nw=1)    
    cmds.progressBar(gMainProgressBar, edit=True, endProgress=True)  


def exportMapSelected():
    objList = cmds.ls(sl=True,fl=True)    
    if len(objList) > 0:
        skinnedMesh = objList[0]
        skinCluster = mel.eval('findRelatedSkinCluster '+skinnedMesh)   
        if skinCluster != None and skinCluster != "":
            #export file dialog
            skinMapFile = cmds.fileDialog2(ff="skin (*.skn)",fm=0,ds=2,okc="Export Skin File")
            if len(skinMapFile) > 0:
                exportMap(skinnedMesh, skinMapFile[0])            
        else:
            print("Error: skinCluster not found")
    else:
        print("Error: no selection found")


def importMapSelected():
    objList = cmds.ls(sl=True,fl=True)    
    if len(objList) > 0:
        skinnedMesh = objList[0]        
        skinMapFile = cmds.fileDialog2(ff="skin (*.skn)",fm=1,ds=2,okc="Import")
        if len(skinMapFile) > 0:        
            importMap(skinnedMesh, skinMapFile[0])
    else:
        print("Error: no selection found")    


def exportSkinSet(*args):
    skinnedMesh = []
    objList = cmds.ls(sl=True, fl=True)
    for obj in objList:
        if isSkinnedMesh(obj):
            skinnedMesh.append(obj)
    if len(skinnedMesh) == 0:
        print "Error: No skinned meshes found..."
        return
    #main 
    folder = cmds.fileDialog2(ff="skin set (*.skn)",fm=3, ds=2, okc="Export")
    if folder == None:
        print "Error: Aborted by user..."
        return
    folder = folder[0]    
    count = 0
    maxCount = len(skinnedMesh)
    cmds.progressWindow(title='Skin Set Export', progress=0, min=0, max = maxCount, status='Exporting Maps...', isInterruptable=False)	
    for mesh in skinnedMesh:
        meshPath = mesh.split('|')[-1]
        path = folder + "/" + meshPath + ".skn"        
        count= count + 1        
        cmds.progressWindow( edit=True, progress=count, status=('Exporting: ' + path ) )
        exportMap(mesh, path)
    cmds.progressWindow(endProgress=1)


def importSkinSet(*args):
    skinMaps = cmds.fileDialog2(ff="skin set (*.skn)",fm=4, ds=2, okc="Import Skin Set")
    if skinMaps == None or len(skinMaps) == 0:
        print "Error: Aborted by user..."
        return    
    count = 0
    maxCount = len(skinMaps)
    cmds.progressWindow(title='Skin Set Export', progress=0, min=0, max = maxCount, status='Exporting Maps...', isInterruptable=False)	
    for skinMap in skinMaps:        
        count = count + 1
        cmds.progressWindow( edit=True, progress=count, status=('Importing: ' + skinMap ) )
        importMap(skinMap)
    cmds.progressWindow(endProgress=1)


#simple UI 
def ShowWindow(*args):
    windowName = "skinMapWindow"
    width = 225
    if (cmds.window(windowName, exists=True)):
        cmds.deleteUI( windowName, window=True )   
    if (cmds.windowPref(windowName, exists=True)):
        cmds.windowPref(windowName, r=True)
    skinMapWindow = cmds.window(windowName, title="Skin Maps", widthHeight=(width,55))
    layout = cmds.columnLayout(rs=2)
    cmds.text(l="", height=2)
    cmds.button(l="Import Skins", c="Otherside.SkinMap.importSkinSet()", w=width)
    cmds.button(l="Export Skins", c="Otherside.SkinMap.exportSkinSet()", w=width)
    cmds.showWindow(skinMapWindow)