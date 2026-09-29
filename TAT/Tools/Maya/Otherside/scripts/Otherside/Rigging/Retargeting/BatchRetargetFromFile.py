import os
import maya.cmds as cmds
import maya.OpenMayaUI as omui
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *
from shiboken2 import wrapInstance
import Otherside.Rigging.Retargeting.RetargetFromFile as RetargetFromFile
import Otherside.Rigging.ControlRig.Character as Character


# Job Definition
'''
{
    anim_base: "anim_base.ma"
    animation: "animation_source.fbx"
    output: "full/path/to/output.ma"
    passes : [
        {"rig_node":"node_name", "char_def":"char_def_file.char"},
        {"rig_node":"node_name", "char_def":"char_def_file.char"}
    ]
}
'''


'''
UTILITY FUNCTIONS
'''
def retarget(job):
    # load the anim base
    cmds.file(job["anim_base"], o=True, f=True)
    # handle passes
    # |- validate pass internal function
    def validate_pass(pass_data):
        if pass_data["rig_node"] == None or pass_data["rig_node"] == "":
            return False
        if pass_data["char_def"] == None or pass_data["char_def"] == "":
            return False
        return True
    # |- process passes
    for pass_data in job["passes"]:
        if validate_pass(pass_data):
            RetargetFromFile.retargetFromFile(pass_data["rig_node"], pass_data["char_def"], job["animation"])
    #save retargeted output
    # |- ensure the output directory exists
    output_dir = os.path.dirname(job["output"])
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    # |- rename the scene *required to perform 'save as'
    cmds.file(rename=job["output"])
    # |- save the newly named scene
    cmds.file(save=True, type='mayaAscii')

def maya_main_window():
    main_window_ptr = omui.MQtUtil.mainWindow()
    return wrapInstance(long(main_window_ptr), QWidget)

def show():
    ui = BatchRetargetUI()
    ui.show()


'''
BATCH RETARGET MAIN UI
'''
class BatchRetargetUI(QDialog):
    def __init__(self, parent=None):
        parent = maya_main_window()
        super(BatchRetargetUI, self).__init__(parent)
        self.setWindowTitle("Batch Retarget Window")
        self.setWindowFlags(Qt.Window)
        self.text_fields = {}
        self.animation_items = []
        #
        self.build_ui()


    def build_ui(self):
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        #############
        # Add Asset References

        # |- Anim Base Field
        group = QGroupBox("Anim Base")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        button_use_current = QPushButton("Use Current Scene")
        button_use_current.clicked.connect(lambda: self.animbase_use_current())
        layout.addWidget(button_use_current)
        self.browser_dialog_group("Scene File", "anim_base", layout, filters="Maya Files (*.ma *.mb);;Maya ASCII (*.ma);;Maya Binary (*.mb);;All Files (*.*)")
        button_open_animbase = QPushButton("Open Anim Base Scene")
        button_open_animbase.clicked.connect(lambda: self.animbase_open())
        layout.addWidget(button_open_animbase)

        # |- 1st Person Rig/Char Fields
        group = QGroupBox("First Person Pass Data")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        self.character_select_group("Rig Node", "rig_node_1p", layout, message="Select First-Person Character Node")
        self.browser_dialog_group("Char Def", "char_def_1p", layout, filters="Character Definitions (*.char);;")

        # |- 3rd Person Rig/Char Fields
        group = QGroupBox("Third Person Pass Data")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        self.character_select_group("Rig Node", "rig_node_3p", layout,  message="Select Third-Person Character Node")
        self.browser_dialog_group("Char Def", "char_def_3p", layout, filters="Character Definitions (*.char);;")

        # |- Output Dir Field
        group = QGroupBox("Output Directory")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        self.browser_dialog_group("Folder Path", "output_dir", layout, folder=True)

        ##############
        # Rename / Replace output file names
        group = QGroupBox("Rename Output")
        self.layout.addWidget(group)
        layout = QHBoxLayout(group)
        self.layout.addLayout(layout)
        self.text_edit_group("Search", "search_string", layout, label_size=50)
        self.text_edit_group("Replace", "replace_string", layout, label_size=50)
        button = QPushButton("Update")
        button.clicked.connect(lambda: self.update_all_animation_output_names())
        button.setFixedWidth(75)
        layout.addWidget(button)

        #############
        # Animation List - Header
        row = QHBoxLayout()
        self.layout.addLayout(row)
        # |- Label
        label = QLabel("Animation")
        row.addWidget(label)
        # |- Add Button
        button_add = QPushButton('Add')
        button_add.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        button_add.setMinimumWidth(75)
        button_add.clicked.connect(lambda: self.select_animation_files())
        row.addWidget(button_add)
        # |- Clear BUtton
        button_clear = QPushButton('Clear')
        button_clear.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        button_clear.setMinimumWidth(75)
        button_clear.clicked.connect(lambda: self.clear_animation_items())
        row.addWidget(button_clear)
        # Animation List - Scrollable Area
        self.scroll_area = QScrollArea()
        self.scroll_content = QWidget(self.scroll_area)
        self.scroll_layout = QVBoxLayout(self.scroll_content)
        self.scroll_layout.setAlignment(Qt.AlignTop)
        self.scroll_content.setLayout(self.scroll_layout)
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setWidget(self.scroll_content)
        self.layout.addWidget(self.scroll_area)

        ##############
        # Execute button
        button_execute = QPushButton('Execute')
        button_execute.clicked.connect(lambda: self.execute())
        self.layout.addWidget(button_execute)
        #
        self.progress_bar = QProgressBar()
        self.layout.addWidget(self.progress_bar)


    def animbase_open(self):
        anim_base = self.text_fields["anim_base"].text()
        cmds.file(anim_base, o=True, f=True)

    def animbase_use_current(self):
        scene_name = cmds.file(query=True, sceneName=True)
        anim_base = self.text_fields["anim_base"].setText(scene_name)
        #cmds.file(anim_base, o=True, f=True)


    def text_edit_group(self, label, field_name, layout, **kwargs):
        label_size = kwargs.get("label_size", 100)
        row = QHBoxLayout()
        layout.addLayout(row)
        # label
        label = QLabel(label)
        label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        label.setMinimumWidth(label_size)
        row.addWidget(label)
        # line edit
        line_edit = QLineEdit()
        row.addWidget(line_edit)
        self.text_fields[field_name] = line_edit


    def character_select_group(self, label, field_name, layout, **kwargs):
        # kwargs
        message = kwargs.get("message", "Select Character Node")
        # horizontal layout
        row = QHBoxLayout()
        layout.addLayout(row)
        # label
        label = QLabel(label)
        label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        label.setMinimumWidth(100)
        row.addWidget(label)
        # line edit
        line_edit = QLineEdit()
        row.addWidget(line_edit)
        self.text_fields[field_name] = line_edit
        # browse button
        button = QPushButton("O")
        button.setFixedHeight(30)
        button.setFixedWidth(30)
        button.clicked.connect(lambda: self.select_character_dialog(line_edit, message))
        row.addWidget(button)


    def browser_dialog_group(self, label, field_name, layout, **kwargs):
        # kwargs
        folder_target = kwargs.get("folder", False)
        filters = kwargs.get("filters", "")
        # horizontal layout
        row = QHBoxLayout()
        layout.addLayout(row)
        # label
        label = QLabel(label)
        label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        label.setMinimumWidth(100)
        row.addWidget(label)
        # line edit
        line_edit = QLineEdit()
        row.addWidget(line_edit)
        self.text_fields[field_name] = line_edit
        # browse button
        button = QPushButton("O")
        button.setFixedHeight(30)
        button.setFixedWidth(30)
        if folder_target:
            button.clicked.connect(lambda: self.select_folder_dialog(line_edit))
        else:
            button.clicked.connect(lambda: self.select_file_dialog(line_edit, filters))
        row.addWidget(button)


    def select_character_dialog(self, line_edit, message):
        self.dialog = SelectCharacterNodeDialog(self)
        self.dialog.set_message_text(message)
        if self.dialog.exec_() == QDialog.Accepted:
            node = self.dialog.get_node()
            line_edit.setText(node)


    def select_file_dialog(self, line_edit, filters=""):
        result = cmds.fileDialog2(fileMode=1, dialogStyle=2, fileFilter=filters, okc="Select")  # fileMode=1 for directory mode
        if result:
            line_edit.setText(result[0])


    def select_folder_dialog(self, line_edit):
        result = cmds.fileDialog2(fileMode=2, dialogStyle=2, okc="Select")  # fileMode=2 for directory mode
        if result:
            line_edit.setText(result[0])


    def select_animation_files(self):
        result = cmds.fileDialog2(fileMode=4, dialogStyle=2, fileFilter="FBX Animations (*.fbx)")
        if result:
            for file in result:
                self.add_animation_item(file)


    def update_all_animation_output_names(self):
        for item in self.animation_items:
            update_animation_output_name(item)


    def update_animation_output_name(self, item):
        search_string = self.text_fields["search_string"].text()
        replace_string = self.text_fields["replace_string"].text()
        item["output_filename"] = os.path.basename(item["input_filename"]).replace(search_string, replace_string).replace(".fbx",".ma")
        item["output_label"].setText(item["output_filename"])


    def add_animation_item(self, text):
        # Validate
        for anim in self.animation_items:
            if anim["input_filename"] == text:
                print("Warning: Animation already in list. Ignored")
                return
        # Layout
        layout = QHBoxLayout()
        # Button
        button = QPushButton("X")
        button.setFixedHeight(30)
        button.setFixedWidth(30)
        button.clicked.connect(lambda: self.remove_animation_item(text))
        layout.addWidget(button)
        # Input Label
        display_text = os.path.basename(text)
        input_label = QLabel(display_text)
        input_label.setToolTip(text)
        layout.addWidget(input_label)
        # Output Label
        display_text = os.path.basename(text)
        output_label = QLabel(display_text)
        output_label.setToolTip(text)
        layout.addWidget(output_label)
        # Data
        item = {
            "input_filename":text,
            "output_filename":os.path.basename(text),
            "layout":layout,
            "input_label":input_label,
            "output_label":output_label
        }
        self.update_animation_output_name(item)
        self.animation_items.append(item)
        self.scroll_layout.addLayout(layout)
        self.scroll_content.adjustSize()


    def remove_animation_item(self, text):
        item = None
        for i in range(0, len(self.animation_items)):
            if self.animation_items[i]["input_filename"] == text:
                item = self.animation_items[i]
                self.animation_items.remove(item)
                break
        if item:
            layout = item["layout"]
            self.remove_layout(layout)
            # Removing the layout from the main layout
            index = self.scroll_layout.indexOf(layout)
            if index >= 0:
                item = self.scroll_layout.takeAt(index)
                del item
            # Update the scroll content size
            self.scroll_content.adjustSize()


    def remove_layout(self, layout):
        # Removing all widgets and sub-layouts from the layout
        while layout.count():
            child = layout.takeAt(0)
            if child.widget():
                # The item is a widget, delete it
                child.widget().deleteLater()
            elif child.layout():
                # The item is a layout, remove it recursively
                self.remove_layout(child.layout())


    def clear_animation_items(self):
        # While there are items in the layout
        while self.scroll_layout.count():
            # Get the first item in the layout
            child = self.scroll_layout.takeAt(0)
            # Check if the child is a widget
            if child.widget():
                # If the child is a widget, delete it
                child.widget().deleteLater()
            # Check if the child is a layout
            elif child.layout():
                # If the child is a layout, remove all its children
                self.remove_layout(child.layout())


    def execute(self):
        anim_base = self.text_fields["anim_base"].text()
        output_dir = self.text_fields["output_dir"].text()
        # Define Passes
        passes = []
        passes.append({
            "rig_node": self.text_fields["rig_node_1p"].text(),
            "char_def": self.text_fields["char_def_1p"].text()})
        # third person
        passes.append({
            "rig_node": self.text_fields["rig_node_3p"].text(),
            "char_def": self.text_fields["char_def_3p"].text()})
        # Build job list from animation data
        jobs = []
        for item in self.animation_items:
            job = {}
            job["anim_base"] = anim_base
            job["passes"] = passes
            job["animation"] = item["input_filename"]
            job["output"] = os.path.join(output_dir, item["output_filename"])
            jobs.append(job)
        # process jobs
        self.progress_bar.setMaximum(len(jobs))
        for i,job in enumerate(jobs):
            self.progress_bar.setValue(i)
            retarget(job)
            # print saved file to script editor
            filename = job["output"]
            print(filename)
        # set progress bar to complete
        self.progress_bar.setValue(len(jobs))



'''
CHARACTER SELECT POPUP DIALOG
'''
class SelectCharacterNodeDialog(QDialog):
    def __init__(self, parent=None):
        super(SelectCharacterNodeDialog, self).__init__(parent)
        self.selection = None
        self.character_list = self.load_character_list()
        main_layout = QVBoxLayout()
        self.messsage_label = QLabel("Select Character Node")
        main_layout.addWidget(self.messsage_label)
        if len(self.character_list) == 0:
            main_layout.addWidget(QLabel(""))
            main_layout.addWidget(QLabel(" (No Valid Characters Found in Scene)"))
            main_layout.addWidget(QLabel(""))
        else:
            # Header
            header_layout = QHBoxLayout()
            header_layout.addWidget(QLabel("Character"))
            header_layout.addWidget(QLabel("Node"))
            place_holder_label = QLabel("")
            place_holder_label.setFixedWidth(60)
            header_layout.addWidget(place_holder_label)
            main_layout.addLayout(header_layout)
            # rows
            scroll_area = QScrollArea()
            scroll_content = QWidget(scroll_area)
            scroll_layout = QVBoxLayout(scroll_content)
            scroll_layout.setAlignment(Qt.AlignTop)
            scroll_content.setLayout(scroll_layout)
            scroll_area.setWidgetResizable(True)
            scroll_area.setWidget(scroll_content)
            for char in self.character_list:
                self.character_field(char, scroll_layout)
            scroll_content.adjustSize()
            main_layout.addWidget(scroll_area)
        # Footer
        footer_layout = QHBoxLayout()
        footer_layout.addWidget(QLabel(""))
        cancel_button = QPushButton("Cancel")
        cancel_button.setFixedWidth(100)
        cancel_button.clicked.connect(self.reject)
        footer_layout.addWidget(cancel_button)
        footer_layout.addWidget(QLabel(""))
        main_layout.addLayout(footer_layout)
        #
        self.setLayout(main_layout)


    def set_message_text(self, text):
        self.messsage_label.setText(text)


    def character_field(self, data, parent):
        layout = QHBoxLayout()
        character_label = QLabel(data["character"])
        node_label = QLabel(data["node"])
        button = QPushButton("Select")
        button.setFixedHeight(30)
        button.setFixedWidth(60)
        button.clicked.connect(lambda: self.select(data))
        layout.addWidget(character_label)
        layout.addWidget(node_label)
        layout.addWidget(button)
        parent.addLayout(layout)


    def select(self, data):
        self.selection = data
        self.accept()


    def load_character_list(self):
        character_list = []
        nodes = Character.Character.findAll()
        for node in nodes:
            # ensure valid node
            valid = cmds.getAttr("{}.characterized".format(node))
            if not valid:
                continue
            #
            instance = Character.load(node)
            data = {}
            data["node"] = instance.node
            data["character"] = instance.instanceName
            character_list.append(data)
        return character_list


    def get_node(self):
        if self.selection == None:
            return None
        return self.selection["node"]
