import maya.cmds as cmds            
from functools import partial

class FirstPersonCameraSpaceSwitch():
    windowName = "FirstPersonCameraSpaceSwitch"
    def __init__(self):
        self.widgets = []
        self.cameraList = []
        self.cameraRoot = None
        self.cameraController = None
    
    def show(self, *args):
        #close pre-existing window
        windowName = FirstPersonCameraSpaceSwitch.windowName
        if (cmds.window(windowName, exists=True)):
            cmds.deleteUI(windowName, window=True )   
        if (cmds.windowPref(windowName, exists=True)):
            cmds.windowPref(windowName, r=True) 
        self.cameraList = self.getCameraList()                       
        #create new window
        self.widgets = {}
        cmds.window( windowName, title="Camera - SpaceSwitch", width=300)    
        cmds.columnLayout("main") 
        #row1
        cmds.rowLayout(nc=2)
        cmds.text(l="Camera Root", align="left", w=80) 
        self.widgets["list_cameras"] = cmds.optionMenu(w=200, cc=self.update)
        for cam in self.cameraList:
            cmds.menuItem(l=cam)   
        cmds.setParent("..")             
        #
        cmds.rowLayout(nc=3)
        cmds.text(l="Parent Space", align="left", width = 80)             
        self.widgets["button_head"] = cmds.button(l="Head", width = 100, height = 25, c=partial(self.switchspace, 1))
        self.widgets["button_world"] = cmds.button(l="World", width = 100, height = 25, c=partial(self.switchspace, 0))   
        cmds.setParent("..")     
        cmds.showWindow()      
        self.update()
        
        
    def getCameraList(self):
        firstPersonCameraList = []
        transformList = cmds.ls(fl=True, type="transform", l=True)
        for transform in transformList:
            if cmds.objExists(transform+".firstPersonCameraRig"):
                firstPersonCameraList.append(transform)
        return firstPersonCameraList

    
    def findChildInHierarchy(self, root, childName):    
        output = None
        children = cmds.listRelatives(root, ad=True, f=True)
        for child in children:
            temp = child.split("|")
            if buffer != None:
                temp = temp[-1]
                if ":" in temp:
                    temp = temp.split(":")
                    if temp != None:
                        temp=temp[-1]
            if temp == childName:
                output = child
                break
        return output
        
        
    def switchspace(self, spaceID=0, *args):
        cam = self.cameraController
        pos = cmds.xform(cam, ws=True, q=True, t=True)
        rot = cmds.xform(cam, ws=True, q=True, ro=True)
        cmds.setAttr(cam+".followParent", spaceID)        
        cmds.xform(cam, ws=True, t=pos, ro=rot)
        self.update()


    def getColor(self, active, *args):
        color = (.2,.2,.2)
        if active == 1:
            color = (.4,.6,.9)
        return color
            
    
    def update(self, *args): 
        #Update Active Camera
        self.cameraRoot = cmds.optionMenu(self.widgets["list_cameras"], q=True, v=True)
        self.cameraController = None
        if self.cameraRoot != None:
            self.cameraController = self.findChildInHierarchy(self.cameraRoot, "CameraController")
        #Update Highlight Information
        if self.cameraController != None:
            headValue = cmds.getAttr(self.cameraController+".followParent")
            worldValue = 0
            if headValue == 0:
                worldValue = 1                            
            cmds.button(self.widgets["button_head"], e=True, backgroundColor = self.getColor(headValue))
            cmds.button(self.widgets["button_world"], e=True, backgroundColor = self.getColor(worldValue))        
        


def showWindow(*args):
    instance = FirstPersonCameraSpaceSwitch()
    instance.show()