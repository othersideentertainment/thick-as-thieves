from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
from Otherside.Rigging.Setup.PageBase import PageBase
from Otherside.Rigging.ControlRig.Character import Character
import Otherside.Rigging.CharacterDefinition as CharacterDefinition
import Otherside.Rigging.RigPose as RigPose
#
#
#
class PageRigPose(PageBase):

    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Rig Pose"
        self.description = "Check character is in T-pose before continuing."
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True
        self.widgetList = []


    def onShow(self):
        #cmds.text("Put Curve Editing Tools Here")
        button_width = 400
        cmds.button(l="Load Rig Pose", width=button_width, c=partial(self.loadRigPose))
        cmds.separator()
        cmds.button(l="Set Bone Rotations to Zero", width=button_width, c=partial(self.setBoneRotationsToZero))


    def loadRigPose(self, *args):
        #RigPose.assumeRigPose()
        RigPose.assumeRigPose_v2(self.character.node)


    def setBoneRotationsToZero(self, *args):
        RigPose.setBoneRotationsToZero()