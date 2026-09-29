from functools import partial
import json
import maya.cmds as cmds
import Otherside.Rigging.CharacterDefinition as CharacterDefinition
import Otherside.Rigging.ControlRig.Character as Character


def Show():
    instance = CharacterDefinitionImporter()
    instance.showWindow()


class CharacterDefinitionImporter():
    def __init__(self, *args):
        self.nameField = ""
        self.rootField = ""
        self.definitionField = ""
        self.rigCheckbox = ""
        self.characterList = []


    def showWindow(self, *args):
        window = cmds.window(width=450, t="Character Definition Importer")
        cmds.columnLayout("main")
        cmds.columnLayout("content")
        # Set Name
        cmds.rowLayout(nc=2)
        cmds.text(l="Name", align="left", width=70)
        self.nameField = cmds.textField("name", tx="", width=390)
        cmds.setParent("..")
        # Set Root
        cmds.rowLayout(nc=3)
        cmds.text(l="Root", align="left", width=70)
        self.rootField = cmds.textField("root", tx="", width=330)
        cmds.button(l="set", width=60, c=partial(self.setRoot))
        cmds.setParent("..")
        # Browse for Definition
        cmds.rowLayout(nc=3)
        cmds.text(l="Definition", align="left", width=70)
        self.definitionField = cmds.textField("definition", tx="", width=330)
        cmds.button(l="browse", width=60, c=partial(self.browseForDefinition))
        cmds.setParent("..")
        # Browse for Definition
        cmds.rowLayout(nc=3)
        cmds.text(l="Rig", align="left", width=70)
        self.rigCheckbox = cmds.checkBox("rig", width=330)
        cmds.text(l="", width=60)
        cmds.setParent("..")
        # Import
        cmds.button(l="Import", c=partial(self.importData))
        cmds.showWindow( window )


    def setRoot(self, *args):
        transforms = cmds.ls(sl=True, fl=True, type="transform", l=True)
        if transforms == None:
            return
        cmds.textField(self.rootField, e=True, tx=transforms[0])


    def browseForDefinition(self, *args):
        #prompt for filename
        buffer = cmds.fileDialog2(fm=1, ff="*.char", ds=2)
        if not buffer:
            return None
        filename = buffer[0]
        cmds.textField(self.definitionField, e=True, tx=filename)


    def importData(self, *args):
        root = cmds.textField(self.rootField, q=True, tx=True)
        rig = cmds.checkBox(self.rigCheckbox, q=True, v=True)
        if len(root) == 0:
            root = None
        name = cmds.textField(self.nameField, q=True, tx=True)
        filename = cmds.textField(self.definitionField, q=True, tx=True)
        CharacterDefinition.import_definition(name, root, filename, rig=rig)

