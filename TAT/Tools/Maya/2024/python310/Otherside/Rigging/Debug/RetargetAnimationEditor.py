from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
#import Otherside.Rigging.ControlRig.Biped as Biped
import Otherside.Rigging.ControlRig.Character as Character


class RetargetAnimationEditor():
    def __init__(self):
        self.characterList = []
        self.characterSourceMenu = ""
        self.characterDestinationMenu = ""
        self.startFrameField = ""
        self.endFrameField = ""


    def showWindow(self):
        WINDOW_NAME = "RetargetAnimationEditor"
        nodeList = RigNode.findNodes("Character")
        self.characterList = []
        for node in nodeList:
            name = cmds.getAttr("{}.instanceName".format(node))
            self.characterList.append( [name, node] )
        start = cmds.playbackOptions(q=True, min=True)
        end = cmds.playbackOptions(q=True, max=True)
        #
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        #
        win = cmds.window(WINDOW_NAME, t="Retarget Animation Editor", width=350)
        cmds.columnLayout("main")
        #Character Source
        cmds.rowLayout(nc=2)
        cmds.text(label='Animation Source', width=150, align="left")
        self.characterSourceMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Character Destination
        cmds.rowLayout(nc=2)
        cmds.text(label='Control Rig', width=150, align="left")
        self.characterDestinationMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Start Frame
        cmds.rowLayout(nc=2)
        cmds.text(l="Start Frame", width=150, align="left")
        self.startFrameField = cmds.intField(v=start, width=200)
        cmds.setParent("..")
        #End Frame
        cmds.rowLayout(nc=2)
        cmds.text(l="End Frame", width=150, align="left")
        self.endFrameField = cmds.intField(v=end, width=200)
        cmds.setParent("..")
        #Execute Button
        cmds.rowLayout(nc=2)
        cmds.text(label='', width=150, align="left")
        cmds.button(l="Retarget Animation", c=partial(self.execute), width=200, align="right")
        cmds.setParent("..")
        cmds.showWindow(win)


    def execute(self, *args):
        start = cmds.intField(self.startFrameField, q=True, v=True)
        end = cmds.intField(self.endFrameField, q=True, v=True)
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        character = Character.load(destinationNode)
        character.transferAnimation(sourceNode, startFrame=start, endFrame=end)


def showWindow(*args, **kwargs):
    instance = RetargetAnimationEditor()
    instance.showWindow()

