import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
from functools import partial
import Otherside.Rigging.RigNode as RigNode

WINDOW_NAME = "anim_tools_window"


def showWindow():
    instance = AnimTools()
    instance.showWindow()


class AnimTools():

    def __init__(self):
        self.cb_selectionChanged = -1
        self.cb_sceneChanged = -1


    def showWindow(self):
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        cmds.window(WINDOW_NAME, t="Anim Tools", cc=partial(self.removeEventListeners), width=350)
        cmds.columnLayout("main")
        #HEADER
        cmds.rowColumnLayout("header", nc=2, cal=[(1,"left"), (2,"right")], cw=[(1,348), (2,10)])
        cmds.text(l="lock window")
        cmds.checkBox("cbLocked", l="", v=False)
        cmds.setParent("..")
        #CONTENT
        cmds.rowColumnLayout("content_layout")
        cmds.text(l="(none)")
        cmds.showWindow(WINDOW_NAME)
        self.addEventListeners()
        self.onSelectionChanged()


    def addEventListeners(self):
        self.cb_selectionChanged = OpenMaya.MEventMessage.addEventCallback("SelectionChanged", self.onSelectionChanged)
        self.cb_sceneChanged = OpenMaya.MEventMessage.addEventCallback("PreFileNewOrOpened", self.onSceneChanged)


    def removeEventListeners(self):
        if self.cb_selectionChanged != -1:
            OpenMaya.MMessage.removeCallback(self.cb_selectionChanged)
        if self.cb_sceneChanged != -1:
            OpenMaya.MMessage.removeCallback(self.cb_sceneChanged)


    def onSceneChanged(self, *args):
        cmds.deleteUI(WINDOW_NAME)


    def onSelectionChanged(self, *args):
        locked = cmds.checkBox(WINDOW_NAME+"|main|header|cbLocked", q=True, v=True)
        if locked:
            return
        self.clearContent()
        cmds.setParent("content_layout")
        sel = cmds.ls(sl=True, fl=True)
        rigNodes = []
        for s in sel:
            node = RigNode.getRelated(s)
            if node != None:
                rigNodes.append(node)
        if len(rigNodes) == 0:
            cmds.text(l="(none)")
        if len(rigNodes) > 0:
            node = rigNodes[0]
            if RigNode.isClass(node, "Finger"):
                node = RigNode.getRelated(node)
            instanceName = cmds.getAttr("{}.instanceName".format(node))
            cmds.text(l="  "+instanceName, font="boldLabelFont", height=30, width=360, align="left", bgc=(.2,.2,.2))
            instance = RigNode.load(node)
            instance.gui()


    def clearContent(self):
        path = WINDOW_NAME+"|main|content_layout|"
        children = cmds.layout("content_layout", q=True, childArray=True)
        if children != None:
            for child in children:
                cmds.deleteUI(child, control=True)