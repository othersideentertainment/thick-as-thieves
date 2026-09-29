import maya.cmds as cmds
import Workspace as Workspace

def validateScene():
    #Check References
    sceneName = cmds.file(q=True, sn=True)
    print "Validating Scene: " + sceneName    
    results = validateReferences()


def validateReferences():    
    referenceList = cmds.ls(type='reference')
    results = {}
    results["data"] = []
    results["invalidNodes"] = []
    results["requiredAction"] = "None"
    for instance in referenceList:        
        
        try:
            #Get Paths
            resolvedPath = cmds.referenceQuery(instance, f=True, un=False)
            unresolvedPath = cmds.referenceQuery(instance, f=True, un=True)
            #Check: Using Absolute Paths
            isAbsolutePath = len(resolvedPath) == len(unresolvedPath)            
            #Check: Ouside Workspace
            isOutsideWorkspace = not tat.existsInWorkspace(unresolvedPath)
            #Build Output
            data = {}
            data["resolvedPath"] = resolvedPath
            data["unresolvedPath"] = unresolvedPath
            data["isAbsolutePath"] = isAbsolutePath
            data["isOutsideWorkspace"] = isOutsideWorkspace
            results["data"].append(data)
        except:
            results["invalidNodes"].append(instance)
    return results


def validateAnimationExport():
    results = {}
    results["issues"] = []
    results["requiredAction"] = "None"

    if not cmds.objExists("gameExporterPreset2"):
        results["issues"].append("No GameExporter Nodes Found")
    else:
        clipCount = cmds.getAttr("gameExporterPreset2.animClips", s=True)
        if clipCount == 0:
            results["issues"].append("No Clips Found")
    return "OK"