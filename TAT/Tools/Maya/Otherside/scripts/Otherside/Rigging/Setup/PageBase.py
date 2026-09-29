from functools import partial
import maya.cmds as cmds
#
#
#
class PageBase(object):
    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "PageBase"
        self.description = "(description text here)"
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True

    def show(self, parentLayout):
        cmds.setParent(parentLayout)
        main = cmds.scrollLayout( width=425, vsb=True, verticalScrollBarThickness=16)
        # Header
        if self.nextEnabled:
            cmds.rowColumnLayout(nc=2, cs=[(1,2), (2,2)], bgc=(.4,.4,.4))
            cmds.text(l=self.title, width=300, align="left", font="boldLabelFont")
            cmds.button(l=self.nextLabel, c=partial(self.click), width=100, bgc=(.5,.5,.5))
            cmds.setParent("..")
        else:
            cmds.rowColumnLayout(nc=1,bgc=(.4,.4,.4))
            cmds.text(l=self.title, width=400, align="left", font="boldLabelFont")
            cmds.setParent("..")
        # Description
        cmds.separator(height=5)
        cmds.text(l=self.description, width=420)
        cmds.separator(height=10)
        #
        self.onShow()
        #Close 'Main'
        cmds.setParent("..")
        cmds.formLayout(
            parentLayout,
            edit=True,
            attachForm=((main, 'top', 0), (main, 'left', 0), (main, 'bottom', 0), (main, 'right', 0)) )

    def enter(self, *args):
        self.onEnter()

    def exit(self, *args):
        self.onExit()

    def click(self, *args):
        self.onClick()
        if self.nextHandler:
            self.nextHandler()

    #
    def onShow(self):
        pass

    def onEnter(self, *args):
        pass

    def onExit(self, *args):
        pass

    def onClick(self, *args):
        pass