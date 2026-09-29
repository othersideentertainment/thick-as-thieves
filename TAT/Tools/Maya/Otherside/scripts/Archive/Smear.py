import maya.cmds as cmds
import random


class PathData(object):
    keys = []
    
    def __init__(self):
        self.keys = []


def CloseWindow(windowName):
    if (cmds.window(windowName, exists=True)):
        cmds.deleteUI( windowName, window=True )   
    if (cmds.windowPref(windowName, exists=True)):
        cmds.windowPref(windowName, r=True)
            

def PlotPath(obj, startFrame, endFrame):
    path = PathData()
    for i in range(startFrame, endFrame+1):
        worldMatrix = cmds.getAttr(obj+".worldMatrix", t=i)
        path.keys.append(worldMatrix)
    return path    


def KeyTransform(obj):
    cmds.setKeyframe(obj, at="tx")
    cmds.setKeyframe(obj, at="ty")
    cmds.setKeyframe(obj, at="tz")
    cmds.setKeyframe(obj, at="rx")
    cmds.setKeyframe(obj, at="ry")
    cmds.setKeyframe(obj, at="rz")
    cmds.setKeyframe(obj, at="sx")
    cmds.setKeyframe(obj, at="sy")
    cmds.setKeyframe(obj, at="sz")   


def SmearPaths(targetList, pathList, smear):
    for i in range(0, len(pathList)):
        offset = smear * i
        for j in range(0, endFrame-startFrame+1+offset): 
            time = startFrame + j
            keyIndex = 0                     
            if j > offset:
                keyIndex = j - offset
            cmds.currentTime(time)
            cmds.xform(targetList[i], ws=True, m=pathList[i].keys[keyIndex])        
            KeyTransform(targetList[i])    


def CreatePathLocators(name, path):
    pathGroup = "PathLocators"
    if not cmds.objExists(pathGroup):
        pathGroup = cmds.createNode("transform", n=pathGroup)        
    markerGroup = name
    if cmds.objExists(markerGroup):
        cmds.delete(markerGroup)        
    markerGroup = cmds.createNode("transform", n=markerGroup)
    markerGroup = cmds.parent(markerGroup, pathGroup)
    for i in range(0, len(path.keys)):
        locName = name + "_Frame"+str(i)
        loc = cmds.createNode("locator", n=locName)
        transform = cmds.listRelatives(loc, parent=True)[0]
        cmds.setAttr(loc+".localScaleX", .001)
        cmds.setAttr(loc+".localScaleY", .001)
        cmds.setAttr(loc+".localScaleZ", .001)                       
        cmds.xform(transform, ws=True, m=path.keys[i])
        cmds.parent(transform, markerGroup)            


def CreatePathLocatorsFromGUI(*args):
    windowID = "pathLocatorWindow"
    startFrame = cmds.intField( windowID + "|layout|startFrame", q=True, v=True)  
    endFrame = cmds.intField( windowID + "|layout|endFrame", q=True, v=True)        
    selected = cmds.ls(sl=True,fl=True)
    pathList = []
    for transform in selected:
       path = PlotPath(transform, startFrame, endFrame)
       pathList.append(path) 
    for i in range(0, len(pathList)):
        CreatePathLocators("Path"+str(i), pathList[i])    
    CloseWindow(windowID)


def CreatePathCurves(name, path):
    pathGroup = "PathCurves"
    if not cmds.objExists(pathGroup):
        pathGroup = cmds.createNode("transform", n=pathGroup)        
    curvesGroup = name
    if cmds.objExists(curvesGroup):
        cmds.delete(curvesGroup)        
    curvesGroup = cmds.createNode("transform", n=curvesGroup)
    curvesGroup = cmds.parent(curvesGroup, pathGroup)    
    #worldspace locator
    loc = cmds.createNode("locator", n="pathLocator")
    pathLocator = cmds.listRelatives(loc, parent=True)[0]
    #Get Point List
    pointList = []
    for i in range(0, len(path.keys)):
        cmds.xform(pathLocator, ws=True, m=path.keys[i])
        point = cmds.xform(pathLocator, q=True, ws=True, t=True)
        pointList.append(point)
    curveObj = cmds.curve(n=name+"Curve", ws=True, p=pointList)    
    #randomize color
    colorList = [5,6,18,17,14,13,16,5]
    colorIndex = random.randrange(0, len(colorList))        
    cmds.setAttr(curveObj+".overrideEnabled", 1)
    cmds.setAttr(curveObj+".overrideColor", colorList[colorIndex])    
    #parent curve to group
    cmds.parent(curveObj, curvesGroup)
    #cleanup
    cmds.delete(pathLocator)


def CreatePathCurvesFromGUI(*args):
    windowID = "pathCurvesWindow"
    startFrame = cmds.intField( windowID + "|layout|startFrame", q=True, v=True)  
    endFrame = cmds.intField( windowID + "|layout|endFrame", q=True, v=True)  
    selected = cmds.ls(sl=True,fl=True)
    pathList = []
    for transform in selected:
       path = PlotPath(transform, startFrame, endFrame)
       pathList.append(path) 
    for i in range(0, len(pathList)):
        CreatePathCurves("Path"+str(i), pathList[i])    
    CloseWindow(windowID)

        
#Path Curves Window
def CreatePathCurvesWindow(*args):
    #Ensure single instance
    windowID = "pathCurvesWindow"
    CloseWindow(windowID)
    #UI Settings
    labelWidth = 60
    fieldWidth = 200     
    #Window Contents        
    cmds.window( windowID, title='Create Path Curves', widthHeight=(280, 85), s=False)
    cmds.rowColumnLayout("layout", numberOfColumns=2)
    #
    cmds.text(l=" Start", align="left", width=labelWidth)
    cmds.intField("startFrame", width=fieldWidth)
    #
    cmds.text(l=" End", align="left", width=labelWidth)
    cmds.intField("endFrame", width=fieldWidth)
    #
    cmds.separator(style="none")
    cmds.button(l="Create Curves", c=CreatePathCurvesFromGUI)
    cmds.setParent("..")
    cmds.showWindow()       

    
#Path Locators Window
def CreatePathLocatorsWindow(*args):
    #Ensure single instance
    windowID = "pathLocatorWindow"
    CloseWindow(windowID)
    #UI Settings
    labelWidth = 60
    fieldWidth = 200
    #Window Contents
    cmds.window( windowID, title='Create Path Locators', widthHeight=(280, 85), s=False)
    cmds.rowColumnLayout("layout", numberOfColumns=2)
    #
    cmds.text(l=" Start", align="left", width=labelWidth)
    cmds.intField("startFrame", width=fieldWidth)
    #
    cmds.text(l=" End", align="left", width=labelWidth)
    cmds.intField("endFrame", width=fieldWidth)
    #
    cmds.separator(style="none")
    cmds.button(l="Create Locators", c=CreatePathLocatorsFromGUI)
    cmds.setParent("..")
    cmds.showWindow()        