from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.ControlRig.Character as Character
import Otherside.Rigging.Retarget as Retarget

WINDOW_NAME = "LinkRootSpaceEditor"

class LinkRootSpace():
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
        win = cmds.window(WINDOW_NAME, t="Link Root Space Editor", width=350)
        cmds.columnLayout("main")
        #Character Source
        cmds.rowLayout(nc=2)
        cmds.text(label='3P Rig', width=150, align="left")
        self.characterSourceMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Character Destination
        cmds.rowLayout(nc=2)
        cmds.text(label='1P Rig', width=150, align="left")
        self.characterDestinationMenu = cmds.optionMenu(width=200)
        for i in range(0,len(self.characterList)):
            cmds.menuItem( label=self.characterList[i][0] )
        cmds.setParent("..")
        #Copy Button
        cmds.rowLayout(nc=2)
        cmds.text(l="", width=150)
        cmds.button(l="Attach", c=partial(self.attach), width=175, align="right")
        cmds.setParent("..")
        #
        cmds.showWindow(win)


    #Copy Pose Based on Rig Values
    def attach(self, *args):
        sourceIndex = cmds.optionMenu(self.characterSourceMenu, q=True, sl=True) - 1
        thirdPersonNode = self.characterList[sourceIndex][1]
        destinationIndex = cmds.optionMenu(self.characterDestinationMenu, q=True, sl=True) - 1
        firstPersonNode = self.characterList[destinationIndex][1]
        #
        thirdPerson = Character.load(thirdPersonNode)
        firstPerson = Character.load(firstPersonNode)
        #
        placement = thirdPerson.getModuleByName("placement")
        if placement:
            for instance in firstPerson.moduleList:
                if instance.rootSpace:
                    RigUtility.parentByMatrix(instance.rootSpace, placement.controllerPlacement, mo=True)


def showWindow(*args, **kwargs):
    instance = LinkRootSpace()
    instance.showWindow()