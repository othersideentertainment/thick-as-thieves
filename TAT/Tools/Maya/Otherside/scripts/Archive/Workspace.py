import maya.cmds as cmds
import maya.mel as mel
import os 


def workspaceKey():
    return "TAT_Raw/SourceAssets/Content/Art/Animation/"


def getWorkspacePath():
    return cmds.workspace(q=True, rd=True)


def convertToProjectPath(filepath):
    filepath = filepath.replace("\\","/")
    buffer = filepath.split(workspaceKey())
    return buffer[1]


def existsInWorkspace(filepath):
    return workspaceKey() in filepath.replace("\\","/")


def smartOpen(filepath):    
    if filepath != None:
        try:        
            mel.eval("catchQuiet( `file -o -f -prompt false \"" + filepath + "\"`)") 
            updateReferencesInScene()        
        except:
            print ("Failed to SmartOpen:"+filepath)
    else:
        print "Error: Invalid file path"


def smartOpenDialog(*args):
    filepath = cmds.fileDialog2(fm=1, ff="Maya Files (*.ma *.mb)", okc="SmartOpen", cap="SmartOpen Scene File")
    smartOpen(filepath[0])


def updateReferencesInScene(*args):
    referenceList = cmds.ls(type='reference')
    for instance in referenceList:
        try:
            #get the resolved file path
            file = cmds.referenceQuery(instance, f=True)            
            file = convertToProjectPath(file)
            #load the project relative file path
            cmds.file( file, loadReference=instance, options='v=0;')
        except:
            print ("Failed to execute update process. Aborted")


def batchUpdateReferencesInScene(*args):
    #File List        
    paths = cmds.fileDialog2(fm=4, okc="Set Output", cap="Set File List")    
    if paths == None:
        return
    sceneList = paths
    sceneCount = len(sceneList)
    
    #abort empty list
    if sceneCount == 0:
        print ("No Files Selected")
        return     

    #progress bar
    window = cmds.window()
    cmds.columnLayout()
    batchTaskProgress = cmds.progressBar(maxValue=sceneCount, width=300)
    cmds.showWindow( window ) 

    #loop through files and do the work
    for i in range (0, sceneCount):
        scene = sceneList[i]
        cmds.progressBar(batchTaskProgress, edit=True, step=1) 
        print ("============================================================")
        print ( "Updating References ("+str(i+1) + " of "+ str(sceneCount)+ "): " + scene )
        print ("============================================================")
        cmds.refresh()  
        mel.eval("catchQuiet( `file -o -f -prompt false \"" + scene + "\"`)")
        cmds.refresh()        
        updateReferencesInScene()
        cmds.refresh()                
        cmds.file(save=True, f=True)  
    cmds.deleteUI(window)    