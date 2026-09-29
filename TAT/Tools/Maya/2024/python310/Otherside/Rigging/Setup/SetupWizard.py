from functools import partial
import maya.cmds as cmds
#
from Otherside.Rigging.Setup.PageCharacterDefinition import PageCharacterDefinition
from Otherside.Rigging.Setup.PageSkeletonMap import PageSkeletonMap
from Otherside.Rigging.Setup.PageRigPose import PageRigPose
from Otherside.Rigging.Setup.PageMarkerLayout import PageMarkerLayout
from Otherside.Rigging.Setup.PageSpaceMap import PageSpaceMap
from Otherside.Rigging.Setup.PageBuildControlRig import PageBuildControlRig
from Otherside.Rigging.Setup.PageExportCharacterDefinition import PageExportCharacterDefinition
#
from Otherside.Rigging.ControlRig.Character import Character


class SetupWizard():
    @staticmethod
    def Show(*args):
        instance = SetupWizard()
        instance.showWindow()

    def __init__(self):
        self.character = Character()
        self.contentLayout = None
        self.pageIndex = 0
        self.pageList = [
            PageCharacterDefinition(self.character, self.next),
            PageSkeletonMap(self.character, self.next),
            PageRigPose(self.character, self.next),
            PageMarkerLayout(self.character, self.next),
            PageSpaceMap(self.character, self.next),
            PageBuildControlRig(self.character, self.next)
            ]
        self.pageList[-1].nextEnabled = False

    def showWindow(self):
        WINDOW_NAME = "CharacterEditorWindow"
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        #
        cmds.window(WINDOW_NAME, t="Character Editor", width=425)
        self.contentLayout = cmds.formLayout()
        self.showPage()
        cmds.setParent("..")
        cmds.showWindow(WINDOW_NAME)

    def showPage(self):
        # Clear Current Page
        children = cmds.layout(self.contentLayout, q=True, childArray=True)
        if children != None:
            for child in children:
                cmds.deleteUI(child, control=True)
        # Load Page at Index
        self.pageList[self.pageIndex].onEnter()
        if self.pageIndex < len(self.pageList):
            self.pageList[self.pageIndex].show(self.contentLayout)

    def next(self):
        #Increment Page
        self.pageIndex += 1
        self.showPage()

def Show(*args):
    SetupWizard.Show()