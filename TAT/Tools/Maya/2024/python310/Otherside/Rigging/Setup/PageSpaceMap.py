from functools import partial
from collections import OrderedDict
import json
import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.RigUtility as RigUtility
from Otherside.Rigging.Setup.PageBase import PageBase

#
#
#
class SpaceField(object):
    def __init__(self, removeSpaceHandler, **kwargs):
        # Value
        self.labelText = kwargs.get("label", "")
        self.targetPath = kwargs.get("target", "")
        self.targetText = RigUtility.shortNameOf(self.targetPath)
        self.widgets = {}
        self.removeSpaceHandler = removeSpaceHandler
        # UI
        self.widgets["labelField"] = cmds.textField(text=self.labelText, cc=(partial(self.setLabel)))
        self.widgets["targetField"] = cmds.text(l=self.targetText)
        self.widgets["buttonSet"] = cmds.button(l="set", width=20, c=(partial(self.setTarget)))
        self.widgets["buttonDelete"] = cmds.button(l="x", width=20, c=(partial(self.remove)))

    def setLabel(self,*args):
        self.labelText = cmds.textField(self.widgets["labelField"], q=True, text=True)

    def setTarget(self,*args):
        self.targetPath = ""
        self.targetText = ""
        selected = cmds.ls(sl=True, fl=True, l=True)
        if selected:
            self.targetPath = selected[0]
            self.targetText = RigUtility.shortNameOf(self.targetPath)
        cmds.text(self.widgets["targetField"], e=True, l=self.targetText)

    def remove(self, *args):
        cmds.deleteUI(self.widgets["buttonDelete"], control=True)
        cmds.deleteUI(self.widgets["buttonSet"], control=True)
        cmds.deleteUI(self.widgets["targetField"], control=True)
        cmds.deleteUI(self.widgets["labelField"], control=True)
        if self.removeSpaceHandler:
            self.removeSpaceHandler(self)


#
#
#
class PageSpaceMap(PageBase):
    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Space Map (Character)"
        self.description = "Define needed space anchors used in SpaceSwitching."
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True
        #
        self.tableLayout = None
        self.spaceFields = []

    def onShow(self):
        button_width = 98
        # Draw the UI
        cmds.rowColumnLayout(nc=4, cs=[(1,2), (2,2), (3,2), (4,2)])
        # Controls
        cmds.button(l="Reset", c=(partial(self.removeAllSpaces)), width=button_width)
        cmds.button(l="Template", c=(partial(self.loadTemplate)), width=button_width)
        cmds.button(l="+Space", c=partial(self.addSpace), width=button_width)
        cmds.button(l="+Space (Selection)", c=partial(self.addSpaceSelected), width=button_width)
        cmds.setParent("..")
        # Table Header
        self.tableLayout = cmds.rowColumnLayout("tableLayout", nc=4)
        cmds.text("label", width=100)
        cmds.text("target", width=200)
        cmds.text("set", width=50)
        cmds.text(l="delete", width=50)
        # [ TABLE DATA WILL END UP HERE ]
        self.loadTemplate()
        cmds.setParent("..")


    def onEnter(self, *args):
        self.character = RigNode.load(self.character.node)

    def onClick(self, *args):
        spacemap = self.getSpacemap()
        self.character.spacemap = spacemap
        cmds.setAttr(self.character.node+".spacemap", json.dumps(spacemap), type="string")
        #
        print("\nAssigning Spaces")
        for mod in self.character.moduleList:
            relatedSpaces = []
            if mod.instanceName == "arm_L":
                relatedSpaces = ["shoulder_L", "propTwoHand", "chest", "hips", "world"]
            elif mod.instanceName == "arm_R":
                relatedSpaces = ["shoulder_R", "propTwoHand", "chest", "hips", "world"]
            elif mod.instanceName == "shoulder_L":
                relatedSpaces = ["chest", "hips", "world"]
            elif mod.instanceName == "shoulder_R":
                relatedSpaces = ["chest", "hips", "world"]
            elif mod.instanceName == "propTwoHand":
                relatedSpaces = ["chest", "hips", "world"]
            elif mod.instanceName == "head":
                relatedSpaces = ["chest", "hips", "world"]
            elif mod.instanceName == "weapon_L":
                relatedSpaces = ["hand_L", "hips", "world"]
            elif mod.instanceName == "weapon_R":
                relatedSpaces = ["hand_R", "hips", "world"]
            elif mod.instanceName == "camera":
                relatedSpaces = ["chest", "hips", "world"]
            #
            spaces = OrderedDict()
            for item in relatedSpaces:
                path = spacemap.get(item, None)
                if path:
                    #spaces.append((str(item), str(path)))
                    spaces[item] = path
            spacesJson = json.dumps(spaces)
            mod.spaces = spaces
            cmds.setAttr(mod.node+".spaces", spacesJson, type="string")
            print(f"|- {mod.instanceName} -> {spacesJson}")

    def getSpacemap(self, *args):
        output = {}
        for field in self.spaceFields:
            label = field.labelText
            path = field.targetPath
            output[label] = path
        return output

    def loadTemplate(self, *args):
        # Module Space Template
        moduleTemplate = {
            "arm_L":["shoulder_L", "propTwoHand", "chest", "hips"],
            "arm_R":["shoulder_R", "propTwoHand", "chest", "hips"],
            "shoulder_L":["chest", "hips"],
            "shoulder_R":["chest", "hips"],
            "weapon_L":["hand_L", "hips"],
            "weapon_R":["hand_R", "hips"],
            "camera":["chest", "hips"]
        }
        # Bone Map
        boneTemplate = {
            "hips":["torso", "bonePelvis"],
            "chest":["torso", "boneSpine3"],
            "propTwoHand": ["propTwoHand", "boneProp"],
            "head":["head", "boneHead"],
            "shoulder_L":["shoulder_L", "boneShoulder"],
            "shoulder_R":["shoulder_R", "boneShoulder"],
            "hand_L":["hand_L", "boneHand"],
            "hand_R":["hand_R", "boneHand"],
            "gameCamera": ["camera", "boneGameCamera"],
            "playerCamera": ["camera", "bonePlayerCamera"]
        }
        # Build Label List
        labelList = []
        for mod in self.character.moduleList:
            newLabels = moduleTemplate.get(mod.instanceName, [])
            labelList = list(set().union(labelList, newLabels))
        labelList = sorted(labelList)
        #
        cmds.setParent(self.tableLayout)
        for label in labelList:
            data = boneTemplate.get(label, None)
            if data:
                mod = self.character.getModuleByName(data[0])
                if mod:
                    target = getattr(mod, data[1], None)
                    if not target:
                        target = ""
                    self.addSpace(label=label, target=target)

    def addSpaceSelected(self, *args):
        targetList = cmds.ls(sl=True, fl=True, l=True)
        self.addSpace(targetList)

    def addSpace(self, *args, **kwargs):
        cmds.setParent(self.tableLayout)
        label = kwargs.get("label", "")
        target = kwargs.get("target", "")
        self.spaceFields.append(SpaceField(self.removeSpace, label=label, target=target))

    def removeAllSpaces(self, *args):
        while len(self.spaceFields) > 0:
            self.spaceFields[0].remove()

    def removeSpace(self, spaceField, *args):
        self.spaceFields.remove(spaceField)


