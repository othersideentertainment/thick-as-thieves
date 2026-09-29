from functools import partial
import maya.cmds as cmds


def createNode():
    node = cmds.createNode("network", name="CameraVisibilitySet")
    cmds.addAttr(node, ln="CameraVisibilitySet", at="compound", nc=2)
    cmds.addAttr(node, ln="camera", at="message", parent="CameraVisibilitySet")
    cmds.addAttr(node, ln="object", at="message", parent="CameraVisibilitySet")           
    return node            


def isNode(node):
    return cmds.objExists(node+".CameraVisibilitySet")


def getNodeValues(node):
    values = {}
    values["camera"] = None
    values["object"] = None
    connectedCamera = cmds.listConnections(node+".camera")
    if connectedCamera != None:
        values["camera"] = connectedCamera[0]            
    connectedObject = cmds.listConnections(node+".object")
    if connectedObject != None:
        values["object"] = connectedObject[0]
    return values


def listRelatedNodes(obj):
    nodeList = []
    connected = cmds.listConnections(obj+".message")
    if connected != None:
        for c in connected:
            if isNode(c):
                nodeList.append(c)
    return nodeList


def findSharedNode(obj, camera):
    objNodes = listRelatedNodes(obj)
    camNodes = listRelatedNodes(camera)
    sharedNode = None
    for nodeA in objNodes:
        for nodeB in camNodes:
            if nodeA == nodeB:
                sharedNode = nodeA
                break
    return sharedNode


def hideObjectInCamera(obj, camera):
    objList = listHiddenObjects(camera)
    if objList == None or obj not in objList:
        cmds.perCameraVisibility(obj, c=camera, hide=True)
        node = createNode()
        cmds.connectAttr(obj+".message", node+".object", f=True)
        cmds.connectAttr(camera+".message", node+".camera", f=True)


def unhideObjectInCamera(obj, camera):
    node = findSharedNode(obj, camera)        
    if node != None:    
        cmds.perCameraVisibility(obj, c=camera, hide=True, remove=True)        
        cmds.delete(node)
        

def listHiddenObjects(camera):
    nodeList = cmds.listConnections(camera+".message")
    objList = []
    if nodeList != None:
        for node in nodeList:
            if isNode(node):
                values = getNodeValues(node)
                if values["camera"] == camera and values["object"] != None:
                    objList.append(values["object"])
    return objList
    
        
class CameraVisibilityEditor():
    @staticmethod
    def showWindow(*args):
        instance = CameraVisibilityEditor()
        instance.draw()
    
    def draw(self):
        self.widgets = {}
        windowName = "CameraVisibilitySet"
        if (cmds.window(windowName, exists=True)):
            cmds.deleteUI( windowName, window=True)
        if (cmds.windowPref(windowName, exists=True)):
            cmds.windowPref(windowName, r=True)
        self.widgets["window"] = cmds.window(windowName, t="Camera Visibility Manager")
        cmds.columnLayout()
        #camera menu    
        cmds.rowLayout(nc=3)
        cmds.text(l="Camera", align="left", width=50,  height=30)
        cameraList = cmds.ls(type="camera", fl=True)
        self.widgets["cameraList"] = cmds.optionMenu(cc=self.update, width=150, height=30)
        for i in range (0,len(cameraList)):
            cameraList[i] = cmds.listRelatives(cameraList[i], p=True)[0]
            cmds.menuItem(l=cameraList[i])
        cmds.button(l="Reload", width=45, height=28, c=partial(CameraVisibilityEditor.showWindow))
        cmds.setParent("..")
        #Related List
        cmds.text(l="", height=10)
        cmds.button("Add Selected", width = 250, c=partial(self.unhideSelected))
        cmds.button("Remove Selected", width = 250, c=partial(self.hideSelected))
        cmds.text(l="Hidden Items", w=250, align="center")
        self.widgets["hiddenItems"] = cmds.textScrollList( allowMultiSelection=True, width=250)   
        cmds.showWindow(self.widgets["window"])
        self.update()
            
    def update(self, *args):
        activeCamera = cmds.optionMenu(self.widgets["cameraList"], q=True, v=True)
        hiddenItems = listHiddenObjects(activeCamera)        
        cmds.textScrollList(self.widgets["hiddenItems"], e=True, ra=True)    
        cmds.textScrollList(self.widgets["hiddenItems"], e=True, append=hiddenItems) 
        
    def unhideSelected(self, *args):
        activeCamera = cmds.optionMenu(self.widgets["cameraList"], q=True, v=True)
        objList = cmds.ls(sl=True, fl=True, type="transform")
        for obj in objList:
            hideObjectInCamera(obj, activeCamera)
        self.update()
        
    def hideSelected(self, *args):
        activeCamera = cmds.optionMenu(self.widgets["cameraList"], q=True, v=True)
        objList = cmds.ls(sl=True, fl=True, type="transform")
        for obj in objList:
            unhideObjectInCamera(obj, activeCamera)
        self.update()


def showEditor(*args):
    CameraVisibilityEditor.showWindow()
#CameraVisibilityEditor.showWindow()    