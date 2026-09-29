import os
import sys
import time
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
    range: False
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
            RetargetFromFile.retargetFromFile(pass_data["rig_node"], pass_data["char_def"], job["animation"], job["range"], job["placement_mode"])
    #force 60 fps        
    if job["save_60fps"]:
        cmds.currentUnit(time='ntscf')
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
    return wrapInstance(int(main_window_ptr), QWidget)

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
        self.sourcedir = ""
        self.label = ""
        #
        self.build_ui()


    def build_ui(self):
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        #############
        # Add Asset References

        # |- Retarget Base Field
        group = QGroupBox("Retarget Base")
        self.layout.addWidget(group)
        layout = QVBoxLayout(group)
        button_use_current = QPushButton("Use Current Scene")
        button_use_current.clicked.connect(lambda: self.animbase_use_current())
        layout.addWidget(button_use_current)
        self.browser_dialog_group("Scene File", "anim_base", layout, filters="Maya Files (*.ma *.mb);;Maya ASCII (*.ma);;Maya Binary (*.mb);;All Files (*.*)")
        button_open_animbase = QPushButton("Open Retarget Base Scene")
        button_open_animbase.clicked.connect(lambda: self.animbase_open())
        layout.addWidget(button_open_animbase)
        
        # |- Options
        group = QGroupBox("Options")
        self.layout.addWidget(group)
        column = QVBoxLayout(group)
        self.layout.addLayout(column)

        # |- Range Mode Radio
        layout = QHBoxLayout()
        column.addLayout(layout)
        # self.layout.addLayout(layout)
        label = QLabel("Range")
        layout.addWidget(label)
        
        self.range_all_radio = QRadioButton("All Curves")
        layout.addWidget(self.range_all_radio)
        self.range_timeline_radio = QRadioButton("Timeline Only")
        layout.addWidget(self.range_timeline_radio)
        self.range_all_radio.setChecked(True)
        r_group = QButtonGroup(layout)
        r_group.addButton(self.range_all_radio)
        r_group.addButton(self.range_timeline_radio)
        
        # |- Placement Pos
        layout = QHBoxLayout()
        column.addLayout(layout)
        label = QLabel("Placement Pos")
        layout.addWidget(label)
        self.placement_mode = QComboBox()
        self.placement_mode.addItems(["root (animates with root)", "origin (static at origin)", "rootFirstFrame (static at root's first frame position)"])
        layout.addWidget(self.placement_mode)
        
        # |- Force Save at 60 FPS
        layout = QHBoxLayout()
        column.addLayout(layout)
        self.save_60fps = QCheckBox("Save File at 60fps?")
        layout.addWidget(self.save_60fps)
        self.save_60fps.setChecked(True)
        
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
        
        # |- Folders or Files
        layout = QHBoxLayout()
        self.layout.addLayout(layout)        
        label = QLabel("Source")
        layout.addWidget(label)
        self.source_folders_radio = QRadioButton("Folders (Recursive)")
        layout.addWidget(self.source_folders_radio)
        self.source_files_radio = QRadioButton("Files")
        layout.addWidget(self.source_files_radio)
        self.source_folders_radio.setChecked(True)
        f_group = QButtonGroup(layout)
        f_group.addButton(self.source_folders_radio)
        f_group.addButton(self.source_files_radio)

        # |- Source Dir Field
        # group = QGroupBox("Source Directory")
        # self.layout.addWidget(group)
        # layout = QVBoxLayout(group)
        # self.browser_dialog_group("Folder Path", "source_dir", layout, folder=True)        

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
        label = QLabel("Animation Count: 0")
        row.addWidget(label)
        self.label = label
        # |- Add Button
        button_add = QPushButton('Add')
        button_add.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        button_add.setMinimumWidth(75)
        button_add.clicked.connect(lambda: self.add_animation_files())
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
        #get any existing paths from the target field
        pth = line_edit.text()
        #get workspace
        workspace = cmds.workspace(q=1, dir=1)
        if not pth:
            pth = workspace
        print("startingDirectory: " + pth)
        result = cmds.fileDialog2(fileMode=1, dialogStyle=2, fileFilter=filters, okc="Select", startingDirectory=pth)  # fileMode=1 for directory mode
        if result:
            line_edit.setText(result[0])


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


    def add_animation_files(self):
        if self.source_folders_radio.isChecked():
            self.select_animation_files_from_source_folder()
        else:
            self.select_animation_files()
        
        #update label with anim item count
        self.label.setText("Animation Count: {}".format(len(self.animation_items)))
            
    def select_animation_files(self):
        #get any existing source path
        pth = self.sourcedir
        #get workspace
        workspace = cmds.workspace(q=1, dir=1)
        if not pth:
            pth = workspace
        print("startingDirectory: " + pth)
        
        result = cmds.fileDialog2(fileMode=4, dialogStyle=2, fileFilter="FBX Animations (*.fbx)", startingDirectory=pth)
        if result:
            self.sourcedir = os.path.dirname(result[0])
            for file in result:
                self.add_animation_item(file)
                

    def update_all_animation_output_names(self):
        for item in self.animation_items:
            self.update_animation_output_name(item)


    def update_animation_output_name(self, item):
        search_string = self.text_fields["search_string"].text()
        replace_string = self.text_fields["replace_string"].text()
        item["output_filename"] = os.path.splitext(os.path.basename(item["input_filename"]).replace(search_string, replace_string))[0]+".ma"
        item["output_label"].setText(os.path.join(item["relative_dir"], item["output_filename"]).replace("\\","/"))


    def select_animation_files_from_source_folder(self):        
        #get any existing source path
        pth = self.sourcedir
        #get workspace
        workspace = cmds.workspace(q=1, dir=1)
        if not pth:
            pth = workspace
        print("startingDirectory: " + pth)
        
        result = cmds.fileDialog2(fileMode=2, dialogStyle=2, okc="Select", startingDirectory=pth)  # fileMode=2 for directory mode
        if result:
            self.sourcedir = result[0]
            for root, dirs, files in os.walk(self.sourcedir):
                for f in files:
                    if f.lower().endswith('.fbx'):
                        self.add_animation_item(os.path.join(root, f))
    

    def add_animation_item(self, text):
        text = text.replace("\\","/")
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
        
        output_name = os.path.basename(text)  
        relative_dir = ""        
        #find relative folder path if using a source folder
        if self.source_folders_radio.isChecked():
            relative_dir = os.path.dirname(text.replace(self.sourcedir+"/", "")) 
        
        # Output Label
        display_text = os.path.join(relative_dir, output_name)
        output_label = QLabel(display_text)
        output_label.setToolTip(text)
        layout.addWidget(output_label)        
        # Data
        item = {
            "input_filename":text,
            "output_filename":output_name,
            "relative_dir":relative_dir,
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
        
        #update label with anim item count
        self.label.setText("Animation Count: {}".format(len(self.animation_items)))


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
        
        #reset item list
        self.animation_items = []
        #update label with anim item count
        self.label.setText("Animation Count: 0")


    def execute(self):
        
        start = time.perf_counter()

        anim_base = self.text_fields["anim_base"].text()
        output_dir = self.text_fields["output_dir"].text()
        range_mode = self.range_all_radio.isChecked() == False
        placement_mode = self.placement_mode.currentText().split()[0]
        save_60fps = self.save_60fps.isChecked()
        # Define Passes
        passes = []
        # third person
        passes.append({
            "rig_node": self.text_fields["rig_node_3p"].text(),
            "char_def": self.text_fields["char_def_3p"].text()})
        passes.append({
            "rig_node": self.text_fields["rig_node_1p"].text(),
            "char_def": self.text_fields["char_def_1p"].text()})
        
        
        # Build job list from animation data
        jobs = []
        for item in self.animation_items:
            job = {}
            job["anim_base"] = anim_base
            job["passes"] = passes
            job["animation"] = item["input_filename"]
            job["output"] = os.path.join(output_dir, item["relative_dir"], item["output_filename"]).replace("\\","/")
            job["range"] = range_mode
            job["placement_mode"] = placement_mode
            job["save_60fps"] = save_60fps
            jobs.append(job)
            
        cmds.outputWindow(show=True)
        # process jobs
        numJobs = len(jobs)
        sys.__stdout__.write('\nStarted Batch Retargeting {} files at {}!\n'.format(numJobs, time.strftime('%H:%M', time.localtime())))        
        sys.__stdout__.flush()  #need to do this in 3.7
        print('\n\nBatch Retargeting {0} files!\n'.format(numJobs))
        self.progress_bar.setMaximum(numJobs)
        
        for i,job in enumerate(jobs):
            #how to abort qProgressBar?
            # if cmds.progressBar(bar, q=1, isCancelled=1):
                # print ('Esc pressed.  Batch Aborted!\n')
                # sys.__stdout__.write('Esc pressed.  Batch Aborted!\n')
                # sys.__stdout__.flush()  #need to do this in 3.7
                # cmds.progressBar(bar, e=1, endProgress=1)
                # break
            #update progress
            self.progress_bar.setValue(i)
            if i is not 0:
                now = time.perf_counter()
                elapsed = now - start
                total = (elapsed/i * numJobs)
                eta = total - elapsed
                sys.__stdout__.write('ETA: {}.  (Estimated Total: {} )\n'.format(self.prettyTime(eta), self.prettyTime(total)))
                # sys.__stdout__.write('ETA: {:.2f} seconds.  (Estimated Total: {:.2f} seconds)\n'.format(eta, total))
                sys.__stdout__.flush()  #need to do this in 3.7
            
            #inject now
            try:
                print('*--Processing "{0}"--*'.format(job["output"]))
                sys.__stdout__.write('\n{0} of {1}\nProcessing "{2}"\n'.format(i+1, numJobs, job["output"]))
                sys.__stdout__.flush()  #need to do this in 3.7
                #do it!
                retarget(job)
            except IOError as err:
                errno, strerror = err.args
                sys.__stdout__.write('\tI/O error on {0}({1}): {2}\n'.format(job["output"], errno, strerror))
                sys.__stdout__.flush()  #need to do this in 3.7
                print('I/O error on {0}({1}): {2}'.format(job["output"], errno, strerror))
            except RuntimeError as strerror:
                sys.__stdout__.write('\tRuntimeError on {0}: {1}\n'.format(job["output"], strerror))
                sys.__stdout__.flush()  #need to do this in 3.7
                print('RuntimeError on {0}: {1}'.format(job["output"], strerror))
            except KeyError as strerror:
                sys.__stdout__.write('\tKeyError on {0}: {1}\n'.format(job["output"], strerror))
                sys.__stdout__.flush()  #need to do this in 3.7
                print('KeyError on {0}: {1}'.format(job["output"], strerror))
            except:
                sys.__stdout__.write('\tUnexpected error on {0}: {1}\n'.format(job["output"], sys.exc_info()[0]))
                sys.__stdout__.flush()  #need to do this in 3.7
                print('Unexpected error on {0}:'.format(job["output"]), sys.exc_info()[0])
                raise
        
            # self.progress_bar.setValue(i)
            # retarget(job)
            # #print saved file to script editor
            # filename = job["output"]
            # print("Finished Processing: " + filename)
        # set progress bar to complete
        self.progress_bar.setValue(numJobs)

        end = time.perf_counter()

        sys.__stdout__.write('Batch Retarget Done at {}!  Total time: {}\n'.format(time.strftime('%H:%M', time.localtime()), self.prettyTime(end - start)))
        # sys.__stdout__.write('Batch Retarget Done at {}!  Total time: {:.2f} seconds\n'.format(time.strftime('%H:%M', time.localtime()),end - start))
        sys.__stdout__.flush()  #need to do this in 3.7
        
        print ('Batch Retarget Done!')
        print('Total Batch Retarget Time: {} seconds.'.format(self.prettyTime(end - start)))
        
    def prettyTime(self, seconds):
        m, s = divmod(seconds, 60)
        h, m = divmod(m, 60)
        d, h = divmod(h, 24)
        output = ''
        if d:
            output += f'{int(d)} day'
            if int(d) > 1:
                output += 's'
        if h:
            output += f' {int(h)} hour'
            if int(h) > 1:
                output += 's'
        if m:
            output += f' {int(m)} minute'
            if int(m) > 1:
                output += 's'
        if s:
            output += f' {int(s)} second'
            if int(s) > 1:
                output += 's'
        
        return output  
        

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
