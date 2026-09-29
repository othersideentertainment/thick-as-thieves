import os
import sys
import maya.cmds as cmds
import maya.OpenMayaUI as omui
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *
from shiboken2 import wrapInstance


'''
UTILITY FUNCTIONS
'''

def maya_main_window():
    main_window_ptr = omui.MQtUtil.mainWindow()
    return wrapInstance(int(main_window_ptr), QWidget)

def show():
    ui = ExportSettingsUI()
    ui.show()

def removeExportOptionVars():
    cmds.optionVar(remove='OSE_ArtRoot')
    cmds.optionVar(remove='OSE_GameRoot')

'''
MAIN UI
'''
class ExportSettingsUI(QDialog):
    def __init__(self, parent=None):
        parent = maya_main_window()
        super(ExportSettingsUI, self).__init__(parent)
        self.setWindowTitle("OSE Export Settings")
        self.setWindowFlags(Qt.Window)
        self.text_fields = {}
        #
        self.build_ui()


    def build_ui(self):
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        #############

        # |- Directories
        if cmds.optionVar(exists='OSE_ArtRoot'):
            artRootText = cmds.optionVar(q='OSE_ArtRoot')
        else:
            artRootText = ""
        if cmds.optionVar(exists='OSE_GameRoot'):
            gameRootText = cmds.optionVar(q='OSE_GameRoot')
        else:
            gameRootText = ""   

        group = QGroupBox("Directories")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        self.browser_dialog_group("Art Root: ", "artRoot", layout, artRootText, folder=True)
        self.browser_dialog_group("Game Art Root: ", "gameRoot", layout, gameRootText, folder=True)

        ##############
        # Save button
        button_save = QPushButton('SAVE')
        button_save.clicked.connect(lambda: self.save())
        self.layout.addWidget(button_save)


    def browser_dialog_group(self, label, field_name, layout, text, **kwargs):
        # kwargs
        folder_target = kwargs.get("folder", False)
        filters = kwargs.get("filters", "")
       
        # horizontal layout
        row = QHBoxLayout()
        layout.addLayout(row)
        # label
        label = QLabel(label)
        label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        label.setMinimumWidth(125)
        row.addWidget(label)
        # line edit
        line_edit = QLineEdit()
        line_edit.setMinimumWidth(450)
        line_edit.insert(text)
        line_edit.textEdited.connect(lambda: self.update_text(line_edit))
        row.addWidget(line_edit)
        self.text_fields[field_name] = line_edit
        # browse button
        button = QPushButton("...")
        button.setFixedHeight(30)
        button.setFixedWidth(30)
        
        # if folder_target:
            # button.clicked.connect(lambda: self.select_folder_dialog(line_edit))
        # else:
            # button.clicked.connect(lambda: self.select_file_dialog(line_edit, filters))
        
        button.clicked.connect(lambda: self.select_folder_dialog(line_edit))
        row.addWidget(button)

    
    def update_text(self, line_edit):
        text = line_edit.text()
        if os.path.isdir(text):
            line_edit.setStyleSheet("")
            text = text.replace('\\', '/')
            if not text.endswith('/'):
                text+= '/'
        else:
            line_edit.setStyleSheet("color: rgb(255,0,0)")
        line_edit.setText(text)


    def select_folder_dialog(self, line_edit, startingDirectory=""):
        #get any existing paths from the target field
        pth = line_edit.text()
        #get workspace
        workspace = cmds.workspace(q=1, dir=1)
        if not pth:
            pth = workspace
        print("startingDirectory: " + pth)
        result = cmds.fileDialog2(fileMode=2, dialogStyle=2, okc="Select", startingDirectory=pth)  # fileMode=2 for directory mode
        if result:            
            line_edit.setText(result[0])
            self.update_text(line_edit)


    def save(self):          
        artRoot = self.text_fields["artRoot"].text()
        gameRoot = self.text_fields["gameRoot"].text()
        
        if os.path.isdir(artRoot):
            cmds.optionVar(sv=('OSE_ArtRoot', artRoot))
        if os.path.isdir(gameRoot):  
            cmds.optionVar(sv=('OSE_GameRoot', gameRoot))
        print("Saved successfully!")
        self.close()