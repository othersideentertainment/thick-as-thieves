import maya.cmds as cmds
import random
from functools import partial

class PathData(object):
    keys = []
    def __init__(self):
        self.keys = []


def PlotPath(obj, startFrame, endFrame):
    path = PathData()
    for i in range(startFrame, endFrame+1):
        worldMatrix = cmds.getAttr(obj+".worldMatrix", t=i)
        path.keys.append(worldMatrix)
    return path


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


class EditorWindow():
    _windowID = "smearGuideWindow"
    _startFrame = "smearGuideWindow|layout|startFrame"
    _endFrame = "smearGuideWindow|layout|endFrame"

    @staticmethod
    def Show():
        EditorWindow.CloseWindow()
        frameMin = cmds.playbackOptions(q=True, min=True)
        frameMax = cmds.playbackOptions(q=True, max=True)
        #Window Contents
        cmds.window( EditorWindow._windowID, title='Smear Guide Editor', widthHeight=(300, 60), s=False)
        cmds.rowColumnLayout("layout", numberOfColumns=4)
        # -spacer
        cmds.text(l="", width=5, height=5)
        cmds.text(l="", width=5, height=5)
        cmds.text(l="", width=5, height=5)
        cmds.text(l="", width=5, height=5)
        #
        cmds.text(l=" Start", align="left", width=50)
        cmds.intField("startFrame", width=150, v=frameMin)
        cmds.text(l="", width=5)
        cmds.button(l="Create Locators", c=partial(EditorWindow.CreateGuidesSelected, 0), width = 90)
        #
        cmds.text(l=" End", align="left", width=50)
        cmds.intField("endFrame", width=150, v=frameMax)
        cmds.text(l="", width=5)
        cmds.button(l="Create Curves", c=partial(EditorWindow.CreateGuidesSelected, 1), width = 90)
        cmds.setParent("..")
        cmds.showWindow()
    
    @staticmethod
    def CloseWindow():        
        if (cmds.window(EditorWindow._windowID, exists=True)):
            cmds.deleteUI( EditorWindow._windowID, window=True )   
        if (cmds.windowPref( EditorWindow._windowID, exists=True)):
            cmds.windowPref( EditorWindow._windowID, r=True)

    @staticmethod
    def CreateGuidesSelected(mode, *args):        
        startFrame = cmds.intField( EditorWindow._startFrame, q=True, v=True)  
        endFrame = cmds.intField( EditorWindow._endFrame, q=True, v=True)  
        selected = cmds.ls(sl=True,fl=True)
        pathList = []
        for transform in selected:
            path = PlotPath(transform, startFrame, endFrame)
            pathList.append(path) 
            for i in range(0, len(pathList)):
                if mode == 0:
                    CreatePathLocators("Path"+str(i), pathList[i])
                else:
                    CreatePathCurves("Path"+str(i), pathList[i])
        cmds.select(selected, r=True)


def ShowWindow(*args):
    EditorWindow.Show()