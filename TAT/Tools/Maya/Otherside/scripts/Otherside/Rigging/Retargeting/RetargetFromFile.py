import sys
import os
from shiboken2 import wrapInstance
from PySide2 import QtCore, QtGui, QtWidgets
import maya.cmds as cmds
from maya import OpenMayaUI as omUI
import Otherside.Rigging.ControlRig.Character as Character
import Otherside.Rigging.CharacterDefinition as CharacterDefinition
import Otherside.Rigging.Retarget as Retarget
import Otherside.Rigging.RigUtility as RigUtility
reload(CharacterDefinition)


#- internal variables
ANIM_CHAR_NAME = "AnimSrc"
ANIM_NAMESPACE = 'AnimSrc'
ANIM_GROUP_NAME = "ANIM_ROOT"


def retargetFromFile(rig_node, char_definition_file, animation_src_file):
    '''
    Description: Retargets animation from an fbx file
    1) Creates a reference to an animation src file with a root-level skeleton
    2) Characterizes the animation src skeleton with the provided character definition file
    3) transfers animation from the newly created animation src character to the Control Rig associated with the provided rig_node
    4) Clean up all unloads all references, deletes all temporary data and namespaces
    '''
    #- get initial scene nodes/namespaces
    original_namespaces = cmds.namespaceInfo(listOnlyNamespaces=True, recurse=True)
    original_dag_nodes = cmds.ls(dag=True, l=True)

    #- import and characterize
    RigUtility.createCleanReference(animation_src_file, namespace=ANIM_NAMESPACE, group_name=ANIM_GROUP_NAME)
    current_dag_nodes = cmds.ls(dag=True, l=True)
    new_nodes = list(set(current_dag_nodes)-set(original_dag_nodes))
    new_nodes = sorted(new_nodes, key=len)
    root = new_nodes[0]
    anim_char_node = CharacterDefinition.import_definition(ANIM_CHAR_NAME, str(root)+"|"+str(ANIM_NAMESPACE)+":root", char_definition_file)

    #- get updated scene namespaces
    new_namespaces = cmds.namespaceInfo(listOnlyNamespaces=True, recurse=True)

    #- Apply the animation
    range = RigUtility.getKeyframeRangeHierarchy(root)
    if range[0] == "inf" or range[1] == "-inf":
        Retarget.retargetAnimation(anim_char_node, rig_node)
    else:
        Retarget.retargetAnimation(anim_char_node, rig_node, startFrame=range[0], endFrame=range[1])

    # - cleanup temp nodes and namespaces
    # |- Remove Referenced Content (including root transform and namespace)
    reference_node = cmds.referenceQuery(animation_src_file, referenceNode=True)
    cmds.file(removeReference=True, referenceNode=reference_node)
    # |- delete characterized markers
    anim_char = Character.load(anim_char_node)
    cmds.delete(anim_char.controlGroup)
    '''
    # Technically either should be fine - this would ever be more complete but Im opting to delete via the character root
    #   as its a more deliberate decision. and leaves room for things like contraints to be created on the fly in the future

    # |- identify other orphaned dag nodes
    #current_dag_nodes = cmds.ls(dag=True, l=True)
    #new_nodes = list(set(current_dag_nodes)-set(original_dag_nodes))
    #cmds.delete(new_nodes)
    '''
    print ("Retargeting {} complete!".format(animation_src_file))




WINDOW_NAME = "retarget_from_file_window"
WINDOW_TITLE = "Retarget From File"
class RetargeterFBXWindow(QtWidgets.QDialog):
    '''
    Simple UI for importing and retargeting animation from an FBX file
    Requires:
    1) Rigged Characte to receive the animation
    2) Character Definition file for the skeleton names and orientation
    3) FBX file matching the provided skeleton as animation source
    '''
    def __init__(self, parent=None):
        super(RetargeterFBXWindow, self).__init__(parent)
        self.setWindowTitle(WINDOW_TITLE)
        self.setObjectName(WINDOW_NAME)
        #
        # Remove the "?" button from the window and enable the minimize button
        # Remove the "?" button from the window, enable the minimize button, and disable the maximize button
        flags = self.windowFlags()
        self.setWindowFlags(flags & ~QtCore.Qt.WindowContextHelpButtonHint | QtCore.Qt.WindowMinimizeButtonHint & ~QtCore.Qt.WindowMaximizeButtonHint | QtCore.Qt.CustomizeWindowHint | QtCore.Qt.WindowTitleHint)
        #
        self.setFixedSize(600, 180)

        # Create the layout with 2 columns and 4 rows
        layout = QtWidgets.QGridLayout(self)
        layout.setColumnMinimumWidth(0, 100)  # set the stretch factor for column 0
        layout.setColumnStretch(1, 3)  # set the stretch factor for column 0
        layout.setColumnMinimumWidth(2,50)  # set the stretch factor for column 1
        layout.setHorizontalSpacing(10) # set the horizontal spacing between columns
        layout.setVerticalSpacing(10)   # set the vertical spacing between rows
        layout.setContentsMargins(10, 10, 10, 10) # set the margin around the layout
        layout.setAlignment(QtCore.Qt.AlignTop)

        # Create the dropdown field in row 1
        label = QtWidgets.QLabel("Nodes")
        layout.addWidget(label, 0, 0, 1, 1) # set the row span to 1 and column span to 1
        self.character_nodes = QtWidgets.QComboBox()
        self.character_nodes.addItems(self.getNodeList())
        layout.addWidget(self.character_nodes, 0, 1, 1, 2) # set the row span to 1 and column span to 2

        # Create the text field and button in row 2
        label = QtWidgets.QLabel("Char Def")
        layout.addWidget(label, 1, 0, 1, 1) # set the row span to 1 and column span to 1
        self.char_file_field = QtWidgets.QLineEdit()
        layout.addWidget(self.char_file_field, 1, 1, 1, 1) # set the row span to 1 and column span to 1
        self.char_file_button = QtWidgets.QPushButton('Browse')
        self.char_file_button.clicked.connect(self.selectCharFile)
        layout.addWidget(self.char_file_button, 1, 2, 1, 1) # set the row span to 1 and column span to 1

        # Create the text field and button in row 3
        label = QtWidgets.QLabel("Anim File")
        layout.addWidget(label, 2, 0, 1, 1) # set the row span to 1 and column span to 1
        self.fbx_file_field = QtWidgets.QLineEdit()
        layout.addWidget(self.fbx_file_field, 2, 1, 1, 1) # set the row span to 1 and column span to 1
        self.fbx_file_button = QtWidgets.QPushButton('Browse')
        self.fbx_file_button.clicked.connect(self.selectFBXFile)
        layout.addWidget(self.fbx_file_button, 2, 2, 1, 1) # set the row span to 1 and column span to 1

        # Create the execute button in row 4
        self.execute_button = QtWidgets.QPushButton('Execute')
        self.execute_button.clicked.connect(self.execute)
        layout.addWidget(self.execute_button, 3, 0, 1, 3) # set the row span to 1 and column span to 2

    def getNodeList(self):
        # Implement the function that gets the list of nodes and returns them as a list of strings
        #node_list = ["Node1", "Node2", "Node3"]
        node_list = Character.Character.findAll()
        return node_list

    def selectCharFile(self):
        # Implement the function that opens the file dialog and sets the char_file_field text to the selected file path
        maya_window = getMainMayaWindow()
        #char_file_path, _ = QtWidgets.QFileDialog.getOpenFileName(maya_window, 'Select .char file', os.path.expanduser("~"), '*.char')
        char_file_path = cmds.fileDialog2(fileMode=1, caption="Select .char file", ff="Character Definition (*.char)")
        if char_file_path:
            char_file_path = char_file_path[0]
            self.char_file_field.setText(char_file_path)

    def selectFBXFile(self):
        # Implement the function that opens the file dialog and sets the fbx_file_field text to the selected file path
        maya_window = getMainMayaWindow()
        #fbx_file_path, _ = QtWidgets.QFileDialog.getOpenFileName(maya_window, 'Select .fbx file', os.path.expanduser("~"), '*.fbx')
        fbx_file_path = cmds.fileDialog2(fileMode=1, caption="Select .fbx file", ff="*.fbx")
        if fbx_file_path:
            fbx_file_path = fbx_file_path[0]
            self.fbx_file_field.setText(fbx_file_path)

    def execute(self):
        # Implement the function that gets the current value of the dropdown, char file field, and fbx file field and executes the desired functionality
        selected_node = self.character_nodes.currentText()
        char_file_path = self.char_file_field.text()
        fbx_file_path = self.fbx_file_field.text()

        # Call your function with the selected node, char file path, and fbx file path as arguments
        retargetFromFile(selected_node, char_file_path, fbx_file_path)


def getMainMayaWindow():
    maya_ptr = omUI.MQtUtil.mainWindow()
    maya_window = wrapInstance(long(maya_ptr), QtWidgets.QWidget)
    return maya_window


def show(*args, **kwargs):
    # Check if the window already exists
    if cmds.window(WINDOW_NAME, exists=True):
        cmds.deleteUI(WINDOW_NAME)
    #
    # Create the PySide2 window
    maya_window = getMainMayaWindow()
    window = RetargeterFBXWindow(maya_window)

    # Add the PySide2 window to the Maya interface
    qt_layout = maya_window.layout()
    qt_layout.addWidget(window)

    # Show the window
    window.show()
