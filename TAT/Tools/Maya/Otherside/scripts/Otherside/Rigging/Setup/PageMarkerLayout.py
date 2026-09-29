from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
#
from Otherside.Rigging.Setup.PageBase import PageBase

class PageMarkerLayout(PageBase):
    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Marker Layout"
        self.description = "Setup orientation marker used to align controllers and retargeting."
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True

    def onShow(self):
        button_width = 200
        cmds.rowLayout(nc=2)
        cmds.button(l="Align to Parent", width=button_width, c=partial(self.markerAlignToParent))
        cmds.button(l="Align to World", width=button_width, c=partial(self.markerAlignToWorld))
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=2)
        cmds.button(l="X -90", width=button_width, c=partial(self.markerApplyRotation, 0))
        cmds.button(l="X +90", width=button_width, c=partial(self.markerApplyRotation, 1))
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=2)
        cmds.button(l="Y -90", width=button_width, c=partial(self.markerApplyRotation, 2))
        cmds.button(l="Y +90", width=button_width, c=partial(self.markerApplyRotation, 3))
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=2)
        cmds.button(l="Z -90", width=button_width, c=partial(self.markerApplyRotation, 4))
        cmds.button(l="Z +90", width=button_width, c=partial(self.markerApplyRotation, 5))
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=1)
        cmds.button(l="Select All Markers", width=button_width*2, c=partial(self.markerSelectAll))
        cmds.setParent("..")


    def onEnter(self, *args):
        self.character = RigNode.load(self.character.node)
        self.character.characterize()


    def markerAlignToParent(self, *args):
        selection = cmds.ls(sl=True, fl=True, type="transform")
        for item in selection:
            cmds.xform(item, os=True, ro=[0,0,0])


    def markerAlignToWorld(self, *args):
        selection = cmds.ls(sl=True, fl=True, type="transform")
        for item in selection:
            cmds.xform(item, ws=True, ro=[0,0,0])


    def markerApplyRotation(self, mode, *args):
        rotation = [0,0,0]
        if mode == 0:
            rotation = [-90,0,0]
        elif mode == 1:
            rotation = [90,0,0]
        elif mode == 2:
            rotation = [0,-90,0]
        elif mode == 3:
            rotation = [0,90,0]
        elif mode == 4:
            rotation = [0,0,-90]
        elif mode == 5:
            rotation = [0,0,90]
        selection = cmds.ls(sl=True, fl=True, type="transform")
        for item in selection:
            cmds.xform(item, os=True, r=True, ro=rotation)
    
    
    def markerSelectAll(self, *args):
        markerList = []
        for mod in self.character.fullModuleList:
            mod = RigNode.load(mod.node)
            markerList.extend(mod.getMarkerList())
        cmds.select(markerList)