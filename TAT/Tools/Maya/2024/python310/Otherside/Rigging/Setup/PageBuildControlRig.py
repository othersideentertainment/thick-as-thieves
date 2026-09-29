from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
from Otherside.Rigging.Setup.PageBase import PageBase
from Otherside.Rigging.ControlRig.Character import Character
import Otherside.Rigging.CharacterDefinition as CharacterDefinition
#
#
#
class PageBuildControlRig(PageBase):

    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Control Rig"
        self.description = "Build Control Rig and Export Character Data."
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True
        self.widgetList = []


    def onShow(self):
        #cmds.text("Put Curve Editing Tools Here")
        button_width = 400
        cmds.button(l="Export Char. Definition", width=button_width, c=partial(self.export_character_definition))


    def onEnter(self, *args):
        self.character = RigNode.load(self.character.node)
        self.character.rig()

    def export_character_definition(self, *args):
        buffer = cmds.fileDialog2(fm=0, ff="*.char", ds=2)
        if not buffer:
            return
        filename = buffer[0]
        output = CharacterDefinition.export_definition(self.character.node, filename)
        print ("Character Definition Exported To: " + output)