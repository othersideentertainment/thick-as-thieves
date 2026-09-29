import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
from functools import partial

#Constants
COLOR_ACTIVE = (.9,.9,.9)
COLOR_INACTIVE = (.5,.5,.5)
COLOR_KEY = (.9,.6,.6)
SEPARATOR_SPACE = 3


'''
(Data Format)
    spaceList  = [
            ["SpaceA", "obj.spaceAAttributePath"]
            ["SpaceB", "obj.spaceBAttributePath"]
        ]
    **Kwargs
    changeCommand(spaceIndex) #spaceIndex (int)
    label (string)
    labelWidth (float)
    fieldWidth (float)
'''
class WSpaceSwitch():
    @staticmethod
    def Create(spaceList, **kwargs):
        label = kwargs.get("label", "Spaces")
        labelWidth = kwargs.get("labelWidth", 200)
        fieldWidth = kwargs.get("fieldWidth", 100)
        buttonWidth = fieldWidth-2
        self = WSpaceSwitch()
        self.changeCommand = kwargs.get("changeCommand", None)
        self.keyCommand = kwargs.get("keyCommand", None)
        self.spaceList = spaceList
        #
        self.widgetRoot = cmds.rowColumnLayout(nc=4)
        #Label
        cmds.text(l=label, width=labelWidth, align="left")
        #Drop Down
        self.optionMenu = cmds.optionMenu( label='', width=fieldWidth-30, changeCommand=(self.onSelectionChange))
        for i in range(0,len(spaceList)):
            cmds.menuItem( label=spaceList[i][0] )
        #Key
        cmds.text(l="  ")
        cmds.button(l="k", width=25, bgc=COLOR_KEY, c=self.onKey)
        #
        cmds.separator(style="none", height=SEPARATOR_SPACE)
        cmds.setParent("..")
        self.onUpdate()
        #Register Jobs/Callback
        cmds.scriptJob(uid=[self.widgetRoot, self.onClose], ro=True)
        for i in range(0, len(spaceList)):
            attributePath = spaceList[i][1]
            job = cmds.scriptJob(ac=[attributePath, self.onUpdate])
            self.scriptJobList.append(job)
        self.cb_timechange = OpenMaya.MEventMessage.addEventCallback("timeChanged", self.onUpdate)

    def __init__(self):
        self.spaceList = []
        self.widgetRoot = None
        self.changeCommand = None
        self.optionMenu = None
        self.cb_timechange = -1
        self.scriptJobList = []

    def onSelectionChange(self, selection, *args):
        if self.changeCommand != None:
            index = cmds.optionMenu(self.optionMenu, q=True, sl=True)
            runtimeCommand = partial(self.changeCommand, index)
            runtimeCommand()

    def onKey(self, *args):
        if self.keyCommand != None:
            runtimeCommand = partial(self.keyCommand)
            runtimeCommand()

    def onUpdate(self, *args):
        if self.validateScene() == False:
            print ("validateScene Failed")
            return
        selectedIndex = 1
        for i in range(0,len(self.spaceList)):
            value = cmds.getAttr(self.spaceList[i][1])
            if value == 1:
                selectedIndex = i+1
        cmds.optionMenu(self.optionMenu, e=True, sl=selectedIndex)

    def onClose(self, *args):
        for job in self.scriptJobList:
            cmds.scriptJob(kill=job, f=True)
        if self.cb_timechange != -1:
            OpenMaya.MMessage.removeCallback(self.cb_timechange)

    def validateScene(self, *args):
        #return self.optionMenu != None and cmds.objExists(self.optionMenu)
        return True



class WSpaceSwitch2():
    @staticmethod
    def Create(switchAttr, **kwargs):
        label = kwargs.get("label", "Spaces")
        labelWidth = kwargs.get("labelWidth", 200)
        fieldWidth = kwargs.get("fieldWidth", 100)
        buttonWidth = fieldWidth-2
        self = WSpaceSwitch2()
        self.switchAttr = switchAttr
        self.changeCommand = kwargs.get("changeCommand", None)
        self.keyCommand = kwargs.get("keyCommand", None)
        self.spaceList = [str(space) for space in cmds.addAttr(switchAttr, q=True, en=True).split(":")]
        #
        self.widgetRoot = cmds.rowColumnLayout(nc=4)
        #Label
        cmds.text(l=label, width=labelWidth, align="left")
        #Drop Down
        self.optionMenu = cmds.optionMenu( label='', width=fieldWidth-30, changeCommand=(self.onSelectionChange))
        for i in range(0,len(self.spaceList)):
            cmds.menuItem( label=self.spaceList[i] )
        #Key
        cmds.text(l="  ")
        cmds.button(l="k", width=25, bgc=COLOR_KEY, c=self.onKey)
        #
        cmds.separator(style="none", height=SEPARATOR_SPACE)
        cmds.setParent("..")
        self.onUpdate()
        #Register Jobs/Callback
        cmds.scriptJob(uid=[self.widgetRoot, self.onClose], ro=True)
        job = cmds.scriptJob(ac=[self.switchAttr, self.onUpdate])
        self.scriptJobList.append(job)
        self.cb_timechange = OpenMaya.MEventMessage.addEventCallback("timeChanged", self.onUpdate)

    def __init__(self):
        self.switchAttr = None
        self.spaceList = []
        self.widgetRoot = None
        self.changeCommand = None
        self.optionMenu = None
        self.cb_timechange = -1
        self.scriptJobList = []

    def onSelectionChange(self, selection, *args):
        if self.changeCommand != None:
            index = cmds.optionMenu(self.optionMenu, q=True, sl=True)
            runtimeCommand = partial(self.changeCommand, index-1)
            runtimeCommand()

    def onKey(self, *args):
        if self.keyCommand != None:
            runtimeCommand = partial(self.keyCommand)
            runtimeCommand()

    def onUpdate(self, *args):
        if self.validateScene() == False:
            print ("validateScene Failed")
            return
        value = cmds.getAttr(self.switchAttr)
        selectedIndex = value+1
        cmds.optionMenu(self.optionMenu, e=True, sl=selectedIndex)

    def onClose(self, *args):
        for job in self.scriptJobList:
            cmds.scriptJob(kill=job, f=True)
        if self.cb_timechange != -1:
            OpenMaya.MMessage.removeCallback(self.cb_timechange)

    def validateScene(self, *args):
        #return self.optionMenu != None and cmds.objExists(self.optionMenu)
        return True

'''
(Data Format)
    attributePath  "obj.attributePath"

    changeCommand(value) #value (float)

    **Kwargs
    label (string)
    buttonLabel0 (string)
    buttonLabel1 (string)
    labelWidth (float)
    fieldWidth (float)
'''
class WAttributeToggle():
    @staticmethod
    def Create(attributePath, **kwargs):
        label = kwargs.get("label", "Mode")
        labelWidth = kwargs.get("labelWidth", 100)
        fieldWidth = kwargs.get("fieldWidth", 100)
        keyable = kwargs.get("keyable", True)
        changeCommand = kwargs.get("changeCommand", None)
        keyCommand = kwargs.get("keyCommand", None)
        buttonLabel0 = kwargs.get("buttonLabel0", "Off")
        buttonLabel1 = kwargs.get("buttonLabel1", "On")
        buttonWidth = (fieldWidth)*.5 - 14
        #
        self = WAttributeToggle()
        if changeCommand == None:
            changeCommand = self.setValue
        if keyCommand == None:
            keyCommand = self.keyHandler
        self.attributePath = attributePath
        #Build UI
        #Horizontal
        columnCount = 5
        columnAlign = [(1,"left"),(2,"center"), (3,"center"), (4, "center"), (5, "center")]
        columnWidth = [[1, labelWidth],[2,buttonWidth], [3, buttonWidth], [4,4], [5,25]]
        if keyable == False:
            columnCount = 3
            columnAlign = [(1,"left"),(2,"center"), (3,"center")]
            columnWidth = [[1, labelWidth], [2,buttonWidth], [3, buttonWidth]]
        self.widgetRoot = cmds.rowColumnLayout(nc=columnCount, cal=[(1,"left"),(2,"center"), (3,"center"), (4, "center"), (5, "center")], columnWidth=[[1, labelWidth],[2,buttonWidth], [3, buttonWidth], [4,4], [5,25]])
        cmds.text(l=label)
        self.button0 = cmds.button("button0", l=buttonLabel0, c=partial(changeCommand, 0), width=buttonWidth)
        self.button1 = cmds.button("button1", l=buttonLabel1, c=partial(changeCommand, 1), width=buttonWidth)
        ''' <NOT IMPLEMENTED> '''
        if keyable:
            cmds.text(l="")
            cmds.button(l="k", width=25, bgc=COLOR_KEY, c=partial(keyCommand))
        ''' </NOT IMPLEMENTED> '''
        cmds.separator(style="none", height=SEPARATOR_SPACE)
        cmds.setParent("..")
        self.onUpdate()
        #Register Jobs/Callback
        cmds.scriptJob(uid=[self.widgetRoot, self.onClose], ro=True)
        self.sj_attrchange = cmds.scriptJob(ac=[self.attributePath, self.onUpdate])
        self.cb_timechange = OpenMaya.MEventMessage.addEventCallback("timeChanged", self.onUpdate)


    def __init__(self):
        self.attributePath = ""
        self.widgetRoot = None
        self.button0 = None
        self.button1 = None
        self.cb_timechange = -1
        self.sj_attrchange = -1


    def onUpdate(self, *args):
        if self.validateScene() == False:
            return
        value = cmds.getAttr(self.attributePath)
        if value == 1:
            cmds.button(self.button1, e=True, bgc=COLOR_ACTIVE)
            cmds.button(self.button0, e=True, bgc=COLOR_INACTIVE)
        else:
            cmds.button(self.button1, e=True, bgc=COLOR_INACTIVE)
            cmds.button(self.button0, e=True, bgc=COLOR_ACTIVE)


    def onClose(self, *args):
        cmds.scriptJob(kill=self.sj_attrchange, f=True)
        if self.cb_timechange != -1:
            OpenMaya.MMessage.removeCallback(self.cb_timechange)

    def validateScene(self, *args):
        return cmds.objExists(self.attributePath)

    def setValue(self, value, *args):
        cmds.setAttr(self.attributePath, value)

    def keyHandler(self, *args):
        cmds.setKeyframe(self.attributePath)



class WAttributeField():
    @staticmethod
    def Create(attributePath, **kwargs):
        self = WAttributeField()
        self.attributePath = attributePath
        label = kwargs.get("label", "Mode")
        labelWidth = kwargs.get("labelWidth", 125)
        fieldWidth = kwargs.get("fieldWidth", 200)
        buttonWidth = kwargs.get("fieldWidth", 25)
        self.widgetRoot =cmds.rowColumnLayout(nc=3, co=[(1,"left",0),(2,"both",5), (3,"right",0)], cw=[(1, labelWidth), (2,fieldWidth), (3,buttonWidth)])
        cmds.text(l=label)
        cmds.attrControlGrp(l=" ", attribute=self.attributePath)
        cmds.button(l="k", bgc=COLOR_KEY, width=25, c=partial(self.keyAttribute))
        cmds.separator(style="none", height=SEPARATOR_SPACE)
        cmds.setParent("..")
        #Register Jobs/Callback
        cmds.scriptJob(uid=[self.widgetRoot, self.onClose], ro=True)

    def __init__(self):
        self.attributePath = ""
        self.widgetRoot = None

    def onClose(self, *args):
        pass

    def keyAttribute(self, *args):
        cmds.setKeyframe(self.attributePath)



#
class SpaceSwitch():
    @staticmethod
    def Create(switchAttr, controllerList=[], **kwargs):
        label = kwargs.get("label", "Spaces")
        labelWidth = kwargs.get("labelWidth", 200)
        fieldWidth = kwargs.get("fieldWidth", 100)        
        self = SpaceSwitch()
        self.controllerList = controllerList
        self.switchAttr = switchAttr
        self.spaceList = [str(space) for space in cmds.addAttr(switchAttr, q=True, en=True).split(":")]        
        self.widgetRoot = cmds.rowColumnLayout(nc=4)
        #Label
        cmds.text(l=label, width=labelWidth, align="left")
        #Drop Down
        self.optionMenu = cmds.optionMenu( label='', width=fieldWidth-30, changeCommand=(self.onChange))
        for i in range(0,len(self.spaceList)):
            cmds.menuItem( label=self.spaceList[i] )
        #Key
        cmds.text(l="  ")
        cmds.button(l="k", width=25, bgc=COLOR_KEY, c=self.onKey)        
        cmds.separator(style="none", height=SEPARATOR_SPACE)
        cmds.setParent("..")
        self.onUpdate()
        #Register Jobs/Callback
        cmds.scriptJob(uid=[self.widgetRoot, self.onClose], ro=True)
        job = cmds.scriptJob(ac=[self.switchAttr, self.onUpdate])
        self.scriptJobList.append(job)
        self.cb_timechange = OpenMaya.MEventMessage.addEventCallback("timeChanged", self.onUpdate)

    def __init__(self):        
        self.switchAttr = None        
        self.controllerList = None
        self.widgetRoot = None
        self.optionMenu = None
        self.spaceList = []
        self.cb_timechange = -1
        self.scriptJobList = []

    def validateScene(self, *args):
        return True

    def onUpdate(self, *args):
        if self.validateScene() == False:
            print ("validateScene Failed")
            return
        value = cmds.getAttr(self.switchAttr)
        selectedIndex = value+1
        cmds.optionMenu(self.optionMenu, e=True, sl=selectedIndex)

    def onClose(self, *args):
        for job in self.scriptJobList:
            cmds.scriptJob(kill=job, f=True)
        if self.cb_timechange != -1:
            OpenMaya.MMessage.removeCallback(self.cb_timechange)

    def onChange(self, selection, *args):        
        # Cache WorldSpace Positions
        matrixList = []
        for i in range(0,len(self.controllerList)):
            matrixList.append( cmds.xform(self.controllerList[i], q=True, ws=True, m=True) )        
        # Perform SpaceSwitch
        index = cmds.optionMenu(self.optionMenu, q=True, sl=True)        
        cmds.setAttr(self.switchAttr, index-1)
        # Re-Apply WorldSpace Positions
        for i in range(0,len(self.controllerList)):
            cmds.xform(self.controllerList[i], ws=True, m=matrixList[i])

    def onKey(self, *args):
        cmds.setKeyframe(self.switchAttr)
        for controller in self.controllerList:
            cmds.setKeyframe(controller)