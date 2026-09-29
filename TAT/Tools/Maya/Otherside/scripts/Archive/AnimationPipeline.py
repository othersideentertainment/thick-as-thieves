import maya.cmds as cmds
import maya.mel as mel
import Workspace as Workspace
import os

#
def getClipByIndex(clipIndex):
    attributePath = "gameExporterPreset2.animClips["+str(clipIndex)+"]"
    data = {}
    data["name"] = cmds.getAttr(attributePath+".animClipName")
    data["start"] = cmds.getAttr(attributePath+".animClipStart")
    data["end"] = cmds.getAttr(attributePath+".animClipEnd")
    data["export"] = cmds.getAttr(attributePath+".exportAnimClip")
    return data


#
def getActiveClips():
    clipCount = cmds.getAttr("gameExporterPreset2.animClips", s=True)
    clipList = []
    for i in range(0, clipCount):
        clip = getClipByIndex(i)
        if clip["export"]:
            clipList.append(clip)
    return clipList


#
def getOutputFilename():
    path = cmds.getAttr("gameExporterPreset2.exportPath")
    path = path.replace("\\","/");    
    if Workspace.existsInWorkspace(path):
        path = Workspace.convertToProjectPath(path)
        path = Workspace.getWorkspacePath() + path
    file = cmds.getAttr("gameExporterPreset2.exportFilename")
    return path +"/"+ file+".fbx"


#
def getFrameRange(clipList):        
    #get frame range
    frameRange = {"start":clipList[0]["start"], "end":clipList[0]["end"]}                
    for i in range(1, len(clipList)):
        if clipList[i]["start"] < frameRange["start"]:
            frameRange["start"] = clipList[i]["start"]
        if clipList[i]["end"] > frameRange["end"]:
            frameRange["end"] = clipList[i]["end"]   
    return frameRange


#
def getExportData():
    clipList = getActiveClips()    
    if len(clipList) == 0:
        print ("No Clips Ready for Export")
        return None    
    #get expor
    filename = getOutputFilename()        
    #build output
    output = {}
    output["filename"] = filename
    output["clipList"] = clipList
    return output


#
def buildExportSkeleton():
    #input
    rootBone ="|ControlRig|SHJntGrp|TrajectorySHJnt"
    #extract root space
    rootSpace = cmds.listRelatives(rootBone, p=True, f=True)[0]
    #read and sort heirarchy
    openList = [rootBone]
    boneList = []
    while len(openList) > 0:
        bone = openList.pop(0)
        boneList.append(bone)
        children = cmds.listRelatives(bone, c=1, f=1, type="joint")
        if children != None:                        
            for child in children:                
                openList.append(child)               
    boneList.sort()            
    #Build Clone Skeleton
    cloneBoneList = []
    for bone in boneList:
        cmds.select(cl=True)
        boneParent = cmds.listRelatives(bone, p=True, f=True)[0]
        cloneParent =  boneParent.replace(rootSpace,"")
        cloneName = cmds.ls(bone, sn=True)[0]
        clonePath = bone.replace(rootSpace,"")
        cloneBone = cmds.joint(n=cloneName)    
        if cloneParent != "":
            cloneBone = cmds.parent(cloneBone, cloneParent)[0]
        cloneBoneList.append(cloneBone)              
        #Set Translate/Rotate/Orient
        val = cmds.getAttr(bone+".jointOrient")    
        cmds.setAttr(cloneBone+".jointOrient", val[0][0], val[0][1], val[0][2])
        val = cmds.getAttr(bone+".rotate")    
        cmds.setAttr(cloneBone+".rotate", val[0][0], val[0][1], val[0][2])
        val = cmds.getAttr(bone+".translate")    
        cmds.setAttr(cloneBone+".translate", val[0][0], val[0][1], val[0][2])        
        #build constraints
        cmds.pointConstraint(bone, cloneBone)
        cmds.orientConstraint(bone, cloneBone)
    cloneBoneRoot = rootBone.replace(rootSpace, "")
    return cloneBoneRoot


def processExport():
    log = ""
    try:
        #Get export data
        exportData = getExportData()
        frameRange = getFrameRange(exportData["clipList"])
        #Build export skeleton
        cloneBoneRoot = buildExportSkeleton()
        #Maya 2019 Bug Fix - baking the frame before ensures accurate first frame pose of export
        adjustedStartFrame = frameRange["start"] - 1
        cmds.bakeResults(cloneBoneRoot, t=(adjustedStartFrame, frameRange["end"]), sb=1, hi="below", simulation=True)
        constraintList = cmds.listRelatives(cloneBoneRoot, type="constraint", f=True, ad=True)
        cmds.delete(constraintList)
        #FBXExport
        mel.eval("FBXResetExport")
        mel.eval("FBXProperty Export|IncludeGrp|Animation -v true;")
        mel.eval("FBXExportBakeComplexAnimation  -v 0")
        mel.eval("FBXExportSplitAnimationIntoTakes -c")
        mel.eval("FBXExportDeleteOriginalTakeOnSplitAnimation -v true;")
        #Define Clips/Takes
        for clip in exportData["clipList"]:
            name = str(clip["name"])
            start = str(clip["start"])
            end = str(clip["end"])
            mel.eval("FBXExportSplitAnimationIntoTakes -v \"" + name + "\" "+start + " " + end)
        #Write the FBX
        cmds.select(cloneBoneRoot, r=True, hi=True)
        filename = exportData["filename"]
        mel.eval("FBXExport -f \""+filename+"\" -s")    
        #Cleanup Temporary Assets
        cmds.delete(cloneBoneRoot)
        log = "Exported Animation To: "+filename
    except:
        log = "Error: Unhandled Exception"
    return log

#
def exportCurrentAnimation(*args):
    valid = True
    log = ""
    #Check for valid GameExporter Nodes
    if not cmds.objExists("gameExporterPreset2"):
        log = "Error: No GameExporter Nodes"
        valid = False
    #Check for Active Clips    
    clipList = getActiveClips()
    if len(clipList) == 0:
        log = "Error: No Active Clips Found"
        valid = False    
    #Check for valid output filepath inside the workspace
    #* should this be overriden based off the filename?    
    outputPath = getOutputFilename()
    if not Workspace.existsInWorkspace(outputPath):
        log = "Error: No GameExporter Nodes"
        valid = False
    #Check file permissions
    if os.path.exists(outputPath):
        if os.path.isfile:
            if not os.access(outputPath, os.W_OK):
                log = "Error: Output file exists and is marked Read-Only.  "
                valid = False
        else:
            log = "Error: Invalid Filename (Folder found)"
            valid = False    
    #Export is valid continue
    if valid:
        processExport()
    #Report the results
    print log


def exportMultipleAnimations(*args):
    #File List
    paths = cmds.fileDialog2(fm=4, okc="Select", cap="Set File List", ff="*.mb")
    sceneList = []
    if paths != None:
        sceneList = paths
    sceneCount = len(sceneList)        
    #abort empty list
    if sceneCount == 0:
        print ("No Files Selected")
        return
    #progress bar
        
    batchTaskProgress = cmds.progressWindow(title="Batch Animation Export", status="Starting...", minValue=0, maxValue=sceneCount, progress=0)
    #loop through files and do the work
    for i in range (0, sceneCount):
        scene = sceneList[i]
        cmds.progressWindow(batchTaskProgress, edit=True, step=1, status="Exporting: "+scene)
        print ( "Exporting Animation ("+str(i+1) + " of "+ str(sceneCount)+ "): " + scene )
        cmds.refresh()
        Workspace.smartOpen(scene)        
        exportCurrentAnimation()
        cmds.refresh()    
    cmds.progressWindow(ep=True)
    print "Export Process Complete!"