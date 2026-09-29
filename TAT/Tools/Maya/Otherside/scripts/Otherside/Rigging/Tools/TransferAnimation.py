from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.ControlRig.Character as Character
import Otherside.Rigging.Retarget as Retarget

WINDOW_NAME = "TransferAnimationEditor"

class TransferAnimationEditor():
    def __init__(self):
        self.characterList = []
        self.characterSourceMenu = ""
        self.characterDestinationMenu = ""
        self.startFrameField = ""
        self.endFrameField = ""


    def showWindow(self):
        nodeList = RigNode.findNodes("Character")
        self.characterList = []
        for node in nodeList:
            name = cmds.getAttr("{}.instanceName".format(node))
            self.characterList.append( [name, node] )
        start = cmds.playbackOptions(q=True, min=True)
        end = cmds.playbackOptions(q=True, max=True)
        #Delete Existing Window/Pref
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        #Create new Window
        win = cmds.window(WINDOW_NAME, t="Transfer Animation Window", width=350)
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
        cmds.button(l="Copy", c=partial(self.copyAnimation), width=175, align="right")
        cmds.button(l="Retarget (Baked)", c=partial(self.retargetAnimation), width=175, align="right")
        cmds.setParent("..")
        cmds.showWindow(win)


    def copyAnimation(self, *args):
        start = cmds.intField(self.startFrameField, q=True, v=True)
        end = cmds.intField(self.endFrameField, q=True, v=True)
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        #
        Retarget.copyAnimation(sourceNode, destinationNode, startFrame=start, endFrame=end)


    def retargetAnimation(self, *args):
        start = cmds.intField(self.startFrameField, q=True, v=True)
        end = cmds.intField(self.endFrameField, q=True, v=True)
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        #
        Retarget.retargetAnimation(sourceNode, destinationNode, startFrame=start, endFrame=end)


def showWindow(*args, **kwargs):
    instance = TransferAnimationEditor()
    instance.showWindow()