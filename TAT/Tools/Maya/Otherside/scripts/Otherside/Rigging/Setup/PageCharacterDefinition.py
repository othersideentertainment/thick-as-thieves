from functools import partial

import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
#
from Otherside.Rigging.Setup.PageBase import PageBase
#
from Otherside.Rigging.ControlRig.Head import Head
from Otherside.Rigging.ControlRig.Torso import Torso
from Otherside.Rigging.ControlRig.Shoulder import Shoulder
from Otherside.Rigging.ControlRig.Arm import Arm
from Otherside.Rigging.ControlRig.Hand import Hand
from Otherside.Rigging.ControlRig.Leg import Leg
from Otherside.Rigging.ControlRig.HindLeg import HindLeg
from Otherside.Rigging.ControlRig.ToeSet import ToeSet
from Otherside.Rigging.ControlRig.Prop import Prop
from Otherside.Rigging.ControlRig.PropChain import PropChain
from Otherside.Rigging.ControlRig.Placement import Placement


class PageCharacterDefinition(PageBase):
    def __init__(self, character, nextHandler):
        self.character = character
        self.title = "Character Definition"
        self.description = "Select a name and the individual modules used in Control Rig"
        self.nextLabel = "Next"
        self.nextHandler = nextHandler
        self.nextEnabled = True
        self.widgets = {}

    def onShow(self):
        #
        cmds.rowLayout(nc=2)
        cmds.text(l="Name", width=100, align="left")
        self.widgets["nameField"] = cmds.textField(text=self.character.instanceName, width=300)
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=3)
        cmds.text(l="Select Modules", width=100, align="left")
        cmds.button(l="Third Person", width=149, c=partial(self.__thirdPersonOptions))
        cmds.button(l="First Person", width=149, c=partial(self.__firstPersonOptions))
        cmds.setParent("..")
        #
        cmds.rowLayout(nc=2)
        cmds.text(l="", width=100)
        cmds.columnLayout()
        self.widgets["placementField"]= self.__checkbox("Placement")
        self.widgets["headField"]= self.__checkbox("Head")
        self.widgets["torsoField"] = self.__checkbox("Torso")
        self.widgets["shoulderField"]= self.__checkbox("Shoulder")
        self.widgets["armField"] = self.__checkbox("Arms")
        self.widgets["handField"] = self.__checkbox("Hands")
        self.widgets["legField"] = self.__checkbox("Legs")
        self.widgets["hindLegField"] = self.__checkbox("HindLegs", False)
        self.widgets["toeField"] = self.__checkbox("Toes", False)
        self.widgets["tailField"] = self.__checkbox("Tail", False)
        self.widgets["weaponField"] = self.__checkbox("Weapons")
        self.widgets["cameraField"] = self.__checkbox("Camera", False)
        cmds.setParent("..")
        cmds.setParent("..")

    def __thirdPersonOptions(self, *args):
        cmds.checkBox(self.widgets["placementField"], e=True, value=True)
        cmds.checkBox(self.widgets["headField"], e=True, value=True)
        cmds.checkBox(self.widgets["torsoField"], e=True, value=True)
        cmds.checkBox(self.widgets["shoulderField"], e=True, value=True)
        cmds.checkBox(self.widgets["armField"], e=True, value=True)
        cmds.checkBox(self.widgets["handField"], e=True, value=True)
        cmds.checkBox(self.widgets["legField"], e=True, value=True)
        cmds.checkBox(self.widgets["hindLegField"], e=True, value=False)
        cmds.checkBox(self.widgets["toeField"], e=True, value=False)
        cmds.checkBox(self.widgets["tailField"], e=True, value=False)
        cmds.checkBox(self.widgets["weaponField"], e=True, value=True)
        cmds.checkBox(self.widgets["cameraField"], e=True, value=False)

    def __firstPersonOptions(self, *args):
        cmds.checkBox(self.widgets["placementField"], e=True, value=False)
        cmds.checkBox(self.widgets["headField"], e=True, value=False)
        cmds.checkBox(self.widgets["torsoField"], e=True, value=True)
        cmds.checkBox(self.widgets["shoulderField"], e=True, value=True)
        cmds.checkBox(self.widgets["armField"], e=True, value=True)
        cmds.checkBox(self.widgets["handField"], e=True, value=True)
        cmds.checkBox(self.widgets["legField"], e=True, value=False)
        cmds.checkBox(self.widgets["hindLegField"], e=True, value=False)
        cmds.checkBox(self.widgets["toeField"], e=True, value=False)
        cmds.checkBox(self.widgets["tailField"], e=True, value=False)
        cmds.checkBox(self.widgets["weaponField"], e=True, value=True)
        cmds.checkBox(self.widgets["cameraField"], e=True, value=True)

    def __checkbox(self, label, defaultValue=True):
        cmds.rowLayout(nc=2)
        cmds.text(l=label, width=100, align="left")
        field = cmds.checkBox(l="", value=defaultValue)
        cmds.setParent("..")
        return field

    def onClick(self, *args):
        name = cmds.textField(self.widgets["nameField"], q=True, text=True)
        self.character.instanceName = name
        #
        if cmds.checkBox(self.widgets["placementField"], q=True, v=True):
            self.character.addModule(Placement("placement"))
        if cmds.checkBox(self.widgets["headField"], q=True, v=True):
            self.character.addModule(Head("head"))
        if cmds.checkBox(self.widgets["torsoField"], q=True, v=True):
            self.character.addModule(Torso("torso"))
        if cmds.checkBox(self.widgets["shoulderField"], q=True, v=True):
            self.character.addModule(Shoulder("shoulder_L", side=1))
            self.character.addModule(Shoulder("shoulder_R", side=2))
        if cmds.checkBox(self.widgets["armField"], q=True, v=True):
            self.character.addModule(Arm("arm_L", side=1))
            self.character.addModule(Arm("arm_R", side=2))
        if cmds.checkBox(self.widgets["handField"], q=True, v=True):
            self.character.addModule(Hand("hand_L", side=1))
            self.character.addModule(Hand("hand_R", side=2))
        if cmds.checkBox(self.widgets["legField"], q=True, v=True):
            self.character.addModule(Leg("leg_L", side=1))
            self.character.addModule(Leg("leg_R", side=2))
        if cmds.checkBox(self.widgets["hindLegField"], q=True, v=True):
            self.character.addModule(HindLeg("hind_leg_L", side=1))
            self.character.addModule(HindLeg("hind_leg_R", side=2))
        if cmds.checkBox(self.widgets["toeField"], q=True, v=True):
            self.character.addModule(ToeSet("toes_L", side=1))
            self.character.addModule(ToeSet("toes_R", side=2))
        if cmds.checkBox(self.widgets["weaponField"], q=True, v=True):
            self.character.addModule(Prop("weapon_L", side=1))
            self.character.addModule(Prop("weapon_R", side=2))
        if cmds.checkBox(self.widgets["tailField"], q=True, v=True):
            self.character.addModule(PropChain("tail"))
        if cmds.checkBox(self.widgets["cameraField"], q=True, v=True):
            self.character.addModule(Prop("camera"))
        #
        self.character.createNode()