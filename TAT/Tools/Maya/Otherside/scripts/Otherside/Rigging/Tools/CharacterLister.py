from functools import partial
import maya.cmds as cmds
import Otherside.Rigging.ControlRig.Character as Character


WINDOW_NAME = "CharacterListerWindow"
WINDOW_TITLE = "Character Lister"
WINDOW_WIDTH = 605

COLUMN_WIDTH_NODE = 200
COLUMN_WIDTH_NAME = 200
COLUMN_WIDTH_VALID = 100
COLUMN_WIDTH_SELECT = 100


class CharacterInfo():
    def __init__(self):
        self.node = ""
        self.characterName = ""
        self.valid = False


class CharacterListerWindow():
    def __init__(self):
        self.contentLayout = None

    def showWindow(self):
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)
        #
        cmds.window(WINDOW_NAME, t=WINDOW_TITLE, width=WINDOW_WIDTH)
        cmds.columnLayout("layout_main", columnAttach=('both', 5), adj=True)
        #
        cmds.rowColumnLayout( "layout_header", numberOfColumns=4 )
        cmds.text(l="Name", align="left", width=COLUMN_WIDTH_NAME)
        cmds.text(l="Node", align="left", width=COLUMN_WIDTH_NODE)
        cmds.text(l="Valid", align="left", width=COLUMN_WIDTH_VALID)
        cmds.text(l="Select", align="left", width=COLUMN_WIDTH_SELECT)
        cmds.setParent("..")
        #
        scrollLayout = cmds.scrollLayout( "layout_scroll", cr=True, horizontalScrollBarThickness=16, verticalScrollBarThickness=16)
        self.contentLayout = cmds.rowColumnLayout( "layout_content", numberOfColumns=4 )
        #
        cmds.setParent("..")
        cmds.setParent("..")
        #
        cmds.button("button_refresh", l="Refresh", c=partial(self.refresh))
        cmds.showWindow(WINDOW_NAME)


    def clear_content(self):
        children = cmds.layout(self.contentLayout, q=True, childArray=True)
        if children != None:
            for child in children:
                cmds.deleteUI(child, control=True)


    def refresh(self, *args, **kwargs):
        # clear window content
        self.clear_content()
        # get nodes in scene
        nodes = get_character_nodes()
        # read nodes
        info_list = []
        for node in nodes:
            instance = Character.load(node)
            info = CharacterInfo()
            info.node = instance.node
            info.characterName = instance.instanceName
            info.valid = cmds.getAttr("{}.characterized".format(node))
            info_list.append(info)
        # update window
        cmds.setParent(self.contentLayout)
        for info in info_list:
            cmds.text(l=info.characterName, align="left", width=COLUMN_WIDTH_NAME)
            cmds.text(l=info.node, align="left", width=COLUMN_WIDTH_NODE)
            cmds.text(l=info.valid, align="left", width=COLUMN_WIDTH_VALID)
            cmds.button(l="Select", align="left", width=COLUMN_WIDTH_SELECT, c=partial(self.select_node, info.node))

    def select_node(self, node, *args):
        cmds.select(node, r=True)



def get_character_nodes(*args, **kwargs):
    nodes = Character.Character.findAll()
    return nodes


def showWindow():
    instance = CharacterListerWindow()
    instance.showWindow()