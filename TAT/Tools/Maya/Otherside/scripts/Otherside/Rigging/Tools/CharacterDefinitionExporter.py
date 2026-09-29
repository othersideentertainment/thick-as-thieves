from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.CharacterDefinition as CharacterDefinition
import Otherside.Rigging.ControlRig.Character as Character


def Show():
    instance = CharacterDefinitionExporter()
    instance.showWindow()


class CharacterDefinitionExporter():
    def __init__(self, *args):
        self.characterMenu = ""
        self.characterList = []

    def showWindow(self, *args):
        window = cmds.window(width=300, t="Character Definition Exporter")
        cmds.columnLayout("main")
        # Character List
        cmds.columnLayout("content")
        cmds.setParent("..")
        # Output Field
        cmds.rowLayout(nc = 3)
        cmds.text(l="Output", width=65, align="left")
        cmds.textField("filename", text="", width=220)
        cmds.button(l="Set", width=30, c=partial(self.setOutput))
        cmds.setParent("..")
        # Export Button
        cmds.rowLayout(nc=2)
        cmds.text(l="", width=65)
        cmds.button(l="Export", width=250, c=partial(self.exportData))
        cmds.setParent("..")
        #
        cmds.showWindow( window )
        self.update()


    def setOutput(self, *args):
        filename = ""
        buffer = None
        if not filename:
            buffer = cmds.fileDialog2(fm=0, ff="*.char", ds=2)
        if buffer:
            filename = buffer[0]
        cmds.textField("filename", e=True, text=filename)


    def clearContent(self, *args):
        children = cmds.layout("content", q=True, childArray=True)
        if children != None:
            for child in children:
                cmds.deleteUI(child, control=True)


    def update(self, *args):
        self.characterList = Character.Character.findAll()
        #draw the update
        self.clearContent()
        cmds.setParent("content")
        # Source
        cmds.rowLayout(nc=2)
        cmds.text(l="Character", width=65, align="left")
        self.characterMenu = cmds.optionMenu("characterList", width=250)
        for chr in self.characterList:
            cmds.menuItem(l=chr)
        cmds.setParent("..")


    def exportData(self, *args):
        index = cmds.optionMenu(self.characterMenu, q=True, sl=True) - 1
        character_node = self.characterList[index]
        filename = cmds.textField("filename", q=True, text=True)
        output = CharacterDefinition.export_definition(character_node, filename)
        print ("Character Definition Exported To: " + output)

