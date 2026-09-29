from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
#import Otherside.Rigging.ControlRig.Biped as Biped
import Otherside.Rigging.ControlRig.Character as Character


class CopyPoseEditor():
    def __init__(self):
        self.characterList = []
        self.characterSourceMenu = ""
        self.characterDestinationMenu = ""


    def showWindow(self):
        WINDOW_NAME = "CopyPoseEditor"
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
        win = cmds.window(WINDOW_NAME, t="Copy Pose Editor", width=350)
        cmds.columnLayout("main")
        #Character Source
        cmds.rowLayout(nc=2)
        cmds.text(label='Pose Source', width=150, align="left")
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
        #Execute Button
        cmds.rowLayout(nc=2)
        cmds.text(label='', width=150, align="left")
        cmds.button(l="Copy Pose", c=partial(self.execute), width=200, align="right")
        cmds.setParent("..")
        cmds.showWindow(win)


    def execute(self, *args):
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        #
        character = Character.load(destinationNode)
        #
        torso = character.getModuleByName("torso")
        if torso:
            cmds.setAttr(torso.controlGroup+".ikMode", 0)
        #
        character.sync(sourceNode)



def showWindow(*args, **kwargs):
    instance = CopyPoseEditor()
    instance.showWindow()

