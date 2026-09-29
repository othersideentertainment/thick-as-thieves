import maya.cmds as cmds
import maya.mel as mel
from functools import partial
import re

"""
a little utility to help make bind export sets using display layers.
layers names should end with an underscore version number.
a suffix will be created based on the uppercase letters in a layer name plus everything after the last underscore.
i.e. EarRing_01 will be ER01 while Earring_02b will be E02b
the topmost layer is considered always on.
"""

def get_visible_display_layers():
    """
    Returns a list of visible display layer names in the current Maya scene.
    """
    all_display_layers = cmds.ls(type='displayLayer')
    visible_layers = []

    if all_display_layers:
        for layer in all_display_layers:
            # Query the visibility attribute of the display layer
            if cmds.getAttr(layer + ".visibility"):
                if layer != "defaultLayer":
                    visible_layers.append(layer)
    return visible_layers
    
def get_layers_in_order():
    all_display_layers = cmds.ls(type='displayLayer')
    layer_order_dict = {}
    for layer in all_display_layers:
        order = cmds.getAttr(layer + ".displayOrder")
        layer_order_dict[layer] = order
    return sorted(layer_order_dict, key=layer_order_dict.get)
    
    
def get_topmost_layer():
    all_display_layers = cmds.ls(type='displayLayer')
    topmost = ''
    i = 0
    for layer in all_display_layers:
        if layer != "defaultLayer":
            order = cmds.getAttr(layer + ".displayOrder")
            print(layer, order)
            if order >= i:
                topmost = layer
                i = order
    return topmost
   
    
def get_meshes_in_display_layer(layer):
    # Query all members of the display layer
    layer_members = cmds.editDisplayLayerMembers(layer, query=True)

    if not layer_members:
        return []

    mesh_objects = []
    for obj in layer_members:
        # Check if the object is a mesh (or has a mesh shape node)
        shapes = cmds.listRelatives(obj, shapes=True, fullPath=True)
        if shapes:
            for shape in shapes:
                if cmds.nodeType(shape) == 'mesh':
                    mesh_objects.append(obj)
                    break # Add the transform node once and move to the next object

    return mesh_objects

def visibleDisplayLayersToStringold():
    visible_layers = get_visible_display_layers()
    topmost = get_topmost_layer()
    meshes = []
    suffix = ''
    output = 'exportSet: '
    for layer in visible_layers:
        meshes.extend(get_meshes_in_display_layer(layer))
        if layer != topmost:
            initials_list = re.findall(r'[A-Z]', layer)
            initials = "".join(initials_list)
            version = layer.rsplit('_', 1)[-1]
            suffix += initials+version
    if meshes:
        for mesh in meshes:
            output += mesh + ", "
        output += suffix
        
    return output
    
    
def visibleDisplayLayersToString():
    layers = get_layers_in_order()
    suffix = ''
    output = 'exportSet: '
    meshes = get_meshes_in_display_layer(layers[-1])
    for layer in layers:
        if layer != "defaultLayer":
            if cmds.getAttr(layer + ".visibility"):
                if layer != layers[-1]:
                    #get meshes in visible layers
                    meshes.extend(get_meshes_in_display_layer(layer))
                    #make suffix
                    initials_list = re.findall(r'[A-Z]', layer)
                    initials = "".join(initials_list)
                    version = layer.rsplit('_', 1)[-1]
                    if version == layer:
                        cmds.error(f'"{layer}" layer name does not end with an underscore and version number!')
                    suffix += initials+version
    if meshes:
        for mesh in meshes:
            output += mesh + ", "
        output += suffix
        
    return output
        
def addVisibleDisplayLayersToNotes():
    root = "root"
    if cmds.objExists(root):
        # if not cmds.attributeQuery("notes", node=root, exists=True):
        if not cmds.ls('{}.{}'.format(root, 'notes')):
            cmds.addAttr(root, longName="notes", dataType="string")
        notes = cmds.getAttr('{}.{}'.format(root, 'notes'))
        new = visibleDisplayLayersToString()
        suffix = new.rsplit(',', 1)[-1].strip()
        if suffix not in extractSuffixesFromNotes():
            notes += "\n\n"
            notes += new
            cmds.setAttr('{}.{}'.format(root, 'notes'), notes, type='string')
            mel.eval('print "'+suffix+' was added to notes!\\n";')
            return suffix
            #print(f'Added {suffix} to root notes!')
        else:
            cmds.error(f'{suffix} already in notes!')
            
def deleteSuffixFromNotes(suffix):
    root = "root"
    newnotes= ''
    if cmds.objExists(root):
        if cmds.ls('{}.{}'.format(root, 'notes')):
            notes = cmds.getAttr('{}.{}'.format(root, 'notes'))
            for line in notes.splitlines():
                if line.startswith('exportSet:'): #custom mesh export line
                    #get all comma separated mesh names from the line
                    meshList = line.split(':')[1].split(',')
                    testsuffix = meshList.pop().strip()
                    if testsuffix != suffix:
                        newnotes += line + "\n\n"
            if newnotes != notes:
                cmds.setAttr('{}.{}'.format(root, 'notes'), newnotes, type='string')
                mel.eval('print "'+suffix+' was deleted from notes!\\n";')
                return True
            else:
                cmds.error(f'{suffix} not found in notes!')
    return False
    
    
def setLayerVisibilityBasedOnSuffix(suffix):
    if not suffix:
        cmds.error('No suffix selected!')
    # split_suffix = re.findall(r'[A-Z][^A-Z]*', suffix)
    # split_suffix = re.split(r'(?<=\d)(?=[A-Z])', suffix) #split at every uppercase letter that follows a number
    split_suffix = re.split(r'(?<=[a-z0-9])(?=[A-Z])', suffix) #split at every uppercase letter that follows a number or lowercase letter
    layers = get_layers_in_order()
    #hide all layers
    for layer in layers:
        if layer != "defaultLayer":
            cmds.setAttr(layer + ".visibility", False)
            cmds.layerButton(layer, edit=True, layerVisible=False)
            
    for s in split_suffix:
        # print(f'Updating layer {s}')
        for layer in layers:
            if layer != "defaultLayer" and layer != layers[-1]:
                initials_list = re.findall(r'[A-Z]', layer)
                initials = "".join(initials_list)
                version = layer.rsplit('_', 1)[-1]
                testsuffix = initials+version
                if testsuffix == s:
                    #found a match. show layer.
                    cmds.setAttr(layer + ".visibility", True)
                    cmds.layerButton(layer, edit=True, layerVisible=True)
                    break
    
    cmds.setAttr(layers[-1] + ".visibility", True)
    cmds.layerButton(layers[-1], edit=True, layerVisible=True)
    
    
def extractSuffixesFromNotes():
    root = "root"
    suffixes = []
    if cmds.objExists(root):
        if cmds.ls('{}.{}'.format(root, 'notes')):
            notes = cmds.getAttr('{}.{}'.format(root, 'notes'))
            for line in notes.splitlines():
                if line.startswith('exportSet:'): #custom mesh export line
                    #get all comma separated mesh names from the line
                    meshList = line.split(':')[1].split(',')
                    suffixes.append(meshList.pop().strip())
    return suffixes
    
    
def listSuffixesFromNotes():    
    suffixes = ", ".join(extractSuffixesFromNotes())
    mel.eval('print "'+suffixes+'";')
    
    
def createLayerPerMesh(meshes):
    for mesh in meshes:
        new_layer = cmds.createDisplayLayer(name=mesh, empty=True)
        cmds.editDisplayLayerMembers(new_layer, mesh)
        cmds.setAttr(f"{new_layer}.color", 4)
        
    
    
WINDOW_NAME = "DisplayLayersToNotesUI"

class DisplayLayersToNotesUI():
    def __init__(self):
        self.suffixMenu = ''

    def showWindow(self):
        #Destroy existing window/prefs
        if (cmds.windowPref( WINDOW_NAME, exists=True)):
            cmds.windowPref( WINDOW_NAME, r=True)        
        if (cmds.window( WINDOW_NAME, exists=True)):
            cmds.deleteUI( WINDOW_NAME, window=True )

        #Create new window
        win = cmds.window(WINDOW_NAME, t="Display Layers To  Notes", rtf=1)
        c = cmds.columnLayout("main", margins=10, generalSpacing=10, adjustableColumn=1)

        r3 = cmds.rowLayout(nc=3, adjustableColumn=2)  
        cmds.button(l="CREATE DISPLAY LAYER PER MESH", c=partial(self.createLayerPerMesh), align="center", width=200, p=r3)
        cmds.button(l="TOGGLE LAYERS", c=partial(self.toggle_layers), align="center", p=r3)
        cmds.iconTextButton(image='help.png', style='iconOnly', label='?', c=partial(self.open_help), ann="Open Help", p=r3)
        cmds.setParent("..")
        
        r2 = cmds.rowLayout(nc=3, adjustableColumn=2)  
        cmds.button(l="SAVE VISIBLE LAYERS TO ROOT NOTES", c=partial(self.save), align="center", width=200, p=r2)
        cmds.button(l="TOGGLE NOTES", c=partial(self.toggle_notes), align="center", p=r2)
        cmds.iconTextButton(image='refresh.png', style='iconOnly', label='REFRESH', c=partial(self.refresh), ann="Refresh UI", p=r2)
        cmds.setParent("..")
        
        r = cmds.rowLayout(nc=3, adjustableColumn=2)      
        self.suffixMenu = cmds.optionMenu(width=200, cc=partial(self.update), p=r)
        for suffix in extractSuffixesFromNotes():
            cmds.menuItem( label=suffix )
        cmds.setParent("..")
        cmds.button(label='HIDE ALL DISPLAY LAYERS', width=150, align="right", c=partial(self.hide), p=r)
        cmds.iconTextButton(image='QR_delete.png', style='iconOnly', label='x', c=partial(self.delete), ann="Delete Current Suffix from Notes", p=r)
        #
        cmds.showWindow(win)
        
        cmds.scriptJob(event=('PostSceneRead', partial(self.showWindow)), parent=win)

    def update(self, *args):
        suffix = cmds.optionMenu(self.suffixMenu, query=True, value=True)
        setLayerVisibilityBasedOnSuffix(suffix)
        
    def refresh(self, *args):
        suffix = cmds.optionMenu(self.suffixMenu, query=True, select=True)
        self.showWindow()
        if suffix:
            cmds.optionMenu(self.suffixMenu, edit=True, select=suffix)
        
    def delete(self, *args):
        suffix = cmds.optionMenu(self.suffixMenu, query=True, value=True)
        if suffix:
            message = f'Delete "{suffix}" from Root Notes?'
            cd = cmds.confirmDialog(title='Delete Entry?',
                          message=message,
                          messageAlign='left',
                          icon='question',
                          button=['Delete', 'Cancel'],
                          defaultButton='Delete',
                          dismissString='Cancel')
            if cd == 'Cancel':
                return
            if deleteSuffixFromNotes(suffix):
                self.showWindow()
        
    def save(self, *args):
        suffix = addVisibleDisplayLayersToNotes()
        self.refresh()
        if suffix:
            num = cmds.optionMenu(self.suffixMenu, query=True, numberOfItems=True)
            cmds.optionMenu(self.suffixMenu, edit=True, select=num)
        
    def createLayerPerMesh(self, *args):
        sel = cmds.ls(selection=True, type='transform')
        createLayerPerMesh(sel)
        
    def hide(self, *args):
        layers = get_layers_in_order()
        #hide all layers
        for layer in layers:
            if layer != "defaultLayer" and layer != layers[-1]:
                cmds.setAttr(layer + ".visibility", False)
                cmds.layerButton(layer, edit=True, layerVisible=False)
        
    def toggle_notes(self, *args):
        if cmds.objExists('root'):
            cmds.select('root')
        cmds.ToggleAttributeEditor()
 
    def toggle_layers(self, *args):
            cmds.ToggleChannelsLayers()
            
    def open_help(self, *args):
        if (cmds.window( WINDOW_NAME+'Help', exists=True)):
            cmds.deleteUI( WINDOW_NAME+'Help', window=True )
        #Create new window
        win = cmds.window(WINDOW_NAME+'Help', t="Display Layers To Notes Help", width=350, height=400)
        f = cmds.formLayout("main")
        long_text = (
        'This little utility helps create, visualize and manage various mesh permutations for variety.\n'
        'It does this by creating shortcodes that pair to Display Layer names.  These are saved to the notes attribute of the root bone.\n'
        'To create a shortcode, all the uppercase letters of a Display Layer name are extracted, then everything after the last underscore is added.\n'
        'Don\'t use any uppercase letter after the last undescore.\n\n'
        'For example:\n'
        '\tHair_01 becomes H01\n'
        '\tEarRing_02 becomes ER02\n'
        '\tEyebrow_03a becomes E03a\n'
        'All these shortcodes are added together to form the suffix "H01ER02E03a".\n\n'
        'The topmost layer will always be included in the export.  It should contain all the base meshes, like the Head.')
        t = cmds.scrollField(text=long_text, editable=False, wordWrap=True)
        cmds.formLayout(f, edit=True, attachForm=((t, "top", 10), (t, "bottom", 10), (t, "left", 10), (t, "right", 10)))
        cmds.showWindow(WINDOW_NAME+'Help')
       
       
def showWindow(*args, **kwargs):
    instance = DisplayLayersToNotesUI()
    instance.showWindow()