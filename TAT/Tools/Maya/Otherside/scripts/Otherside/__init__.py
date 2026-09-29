import os
import maya.cmds as cmds
import maya.mel as mel
import OthersideMenu
#
import Otherside.Tools.CameraPositionTool
import Otherside.Tools.FirstPersonCameraSpaceSwitch
import Otherside.Tools.CameraVisibility
import Otherside.Tools.JointMoverTemplate
import Otherside.Tools.CopyPasteTool
# rigging
import Otherside.Rigging
import Otherside.Rigging.Rebuild



def initialize(*args):
    update()
    import OthersideMenu
    OthersideMenu.showMenu()


def update(*args):
    reload(Otherside.Tools.FirstPersonCameraSpaceSwitch)
    reload(Otherside.Tools.CameraPositionTool)
    reload(Otherside.Tools.CameraVisibility)
    reload(Otherside.Tools.JointMoverTemplate)
    reload(Otherside.Tools.CopyPasteTool)
    reload(OthersideMenu)
    '''update rigging'''
    reload(Otherside.Rigging.Rebuild)
    ''' REPORT STATUS '''
    print("Otherside Update Complete")
