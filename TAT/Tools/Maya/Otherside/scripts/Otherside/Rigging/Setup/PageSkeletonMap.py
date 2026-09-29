from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
#
from Otherside.Rigging.Setup.PageBase import PageBase


class PageSkeletonMap(PageBase):
    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Skeleton Map"
        self.description = "Map the required bones to build the control rig."
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True
        self.widgetList = []

    def onShow(self):
        # Header
        cmds.rowLayout(nc=3)
        cmds.button("Reset", c=partial(self.resetBoneList), width=133)
        cmds.button("Import", c=partial(self.importBoneList), width=133)
        cmds.button("Export", c=partial(self.exportBoneList), width=133)
        cmds.setParent("..")
        for mod in self.character.fullModuleList:
            self.widgetList.append(WidgetBoneMap(mod))

    def resetBoneList(self, *args):
        # Reset the individual modules
        for mod in self.character.fullModuleList:
            boneNames = mod.getBoneNames()
            boneList = []
            for bone in boneNames:
                RigNode.clearPlug(mod.node, bone)
                boneList.append(None)
            mod.setBoneList(boneList)
        # Reload the Character
        self.character = RigNode.load(self.character.node)
        # Reload UI
        for widget in self.widgetList:
            widget.update()

    def importBoneList(self, *args):
        #Get Root
        root_object = cmds.ls(sl=True, fl=True, type="transform")
        if root_object:
            root_object = root_object[0]
        else:
            root_object = None
        #Import The File
        buffer = cmds.fileDialog2(fm=1, ff="*.bmap", ds=2)
        if not buffer:
            return
        filename = buffer[0]
        file = open(filename, "r")
        text = file.read()
        file.close()
        template = eval(text)
        # Map To Skeleton
        for line in template:
            name = line[0]
            boneList = line[1]
            #Find the Bone in Heirarchy
            for i in range(0,len(boneList)):
                if boneList[i] != None:
                    boneList[i] = RigUtility.findObjectInHierarchy(boneList[i], root_object)
            #Set BoneList for module
            mod = self.character.getModuleByName(name)
            if mod:
                # Set the bonelist
                mod.setBoneList(boneList)
                # Connect Bones to Node
                boneNames = mod.getBoneNames()
                for i in range(0,len(boneNames)):
                    RigNode.setPlug(mod.node, boneNames[i], boneList[i])
        #Reload the Character
        self.character = RigNode.load(self.character.node)
        # Update the UI
        for widget in self.widgetList:
            widget.update()
        print("Imported BoneMap: "+str(filename))

    def exportBoneList(self, *args):
        self.character = RigNode.load(self.character.node)
        buffer = cmds.fileDialog2(fm=0, ff="*.bmap", ds=2)
        if buffer:
            output = []
            for mod in self.character.fullModuleList:
                name = mod.instanceName
                print("Exporting: "+name)
                boneList = mod.getBoneList()
                for i in range(0,len(boneList)):
                    if boneList[i] != None:
                        boneList[i] = RigUtility.shortNameOf(boneList[i])
                data = (name, boneList)
                output.append(data)

            #
            filename = buffer[0]
            file = open(filename, "w")
            #file.write(str(output))
            file.write("[\n")
            for i in range(0, len(output)):
                text = str(output[i])
                if i < len(output)-1:
                    text = text + ","
                text = text + "\n"
                file.write(text)
            file.write("]")
            file.close()
            print("Export Complete: "+filename)



class WidgetBoneMap():
    def __init__(self, module):
        self.module = module
        self.fields = []
        header_width = 405
        label_width = 130
        field_width = 228
        button_width = 40
        #
        cmds.columnLayout()
        cmds.text(l=self.module.instanceName, bgc=(.4,.4,.4), width=header_width, align="left")
        boneList = module.getBoneList()
        boneNames = module.getBoneNames()
        for i in range(0, len(boneNames)):
            cmds.rowLayout(nc=3)
            cmds.text(boneNames[i], width=label_width, align="left")
            self.fields.append(cmds.textField(text="(none)", width=field_width))
            cmds.button(l="set", width=button_width, c=partial(self.setBone, i))
            cmds.setParent("..")
        cmds.setParent("..")

    def setBone(self, index, *args):
        selected = cmds.ls(sl=True, fl=True, type="joint", l=True)
        if selected:
            selected = selected[-1]
        boneList = self.module.getBoneList()
        boneList[index] = selected
        self.module.setBoneList(boneList)
        boneNames = self.module.getBoneNames()
        for i in range(0,len(boneNames)):
            RigNode.setPlug(self.module.node, boneNames[i], boneList[i])
        self.update()

    def update(self, *args):
        self.module = RigNode.load(self.module.node)
        boneList = self.module.getBoneList()
        for i in range(0,len(self.fields)):
            field = self.fields[i]
            bone = boneList[i]
            if bone:
                buffer = bone.split("|")
                bone = buffer[-1]
            else:
                bone = "(none)"
            cmds.textField(field, e=True, text=bone)