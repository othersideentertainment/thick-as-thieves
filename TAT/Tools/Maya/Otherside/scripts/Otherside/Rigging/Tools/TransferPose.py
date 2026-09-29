from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.ControlRig.Character as Character
import Otherside.Rigging.Retarget as Retarget

WINDOW_NAME = "TransferPoseEditor"

class TransferPoseEditor():
    def __init__(self):
        self.characterList = []
        self.characterSourceMenu = ""
        self.characterDestinationMenu = ""


    def showWindow(self):
        nodeList = RigNode.findNodes("Character")
        self.characterList = []
        for node in nodeList:
            name = cmds.getAttr("{}.instanceName".format(node))
            self.characterList.append( [name, node] )
        #Destroy existing window/prefs
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        #Create new window
        win = cmds.window(WINDOW_NAME, t="Transfer Pose Window", width=350)
        cmds.columnLayout("main")
        #Character Source
        cmds.rowLayout(nc=2)
        cmds.text(label='Source', width=150, align="left")
        self.characterSourceMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Character Destination
        cmds.rowLayout(nc=2)
        cmds.text(label='Destination', width=150, align="left")
        self.characterDestinationMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Copy Button
        cmds.rowLayout(nc=2)
        cmds.button(l="Copy", c=partial(self.copyPose), width=175, align="right")
        cmds.button(l="Retarget", c=partial(self.retargetPose), width=175, align="right")
        cmds.setParent("..")
        #
        cmds.showWindow(win)


    #Copy Pose Based on Rig Values
    def copyPose(self, *args):
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        #
        Retarget.copyPose(sourceNode, destinationNode)


    #Retarget based on Character Markers
    def retargetPose(self, *args):
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        sourceNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        destinationNode = self.characterList[destinationIndex][1]
        #
        Retarget.retargetPose(sourceNode, destinationNode)



def showWindow(*args, **kwargs):
    instance = TransferPoseEditor()
    instance.showWindow()