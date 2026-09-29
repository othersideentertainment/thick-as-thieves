'''
# -- TODO --
-Batch the retarget process using the above
-Use MayaStandalone/Subprocess to retrieve a list of character nodes from an unopened character rig scene
 this will allow the selection of a target rig without needing to write out the name.
'''

import json
import re
from collections import OrderedDict
import maya.cmds as cmds
import Otherside.Rigging.ControlRig.Character as Character
import Otherside.Rigging.RigUtility as RigUtility
import Otherside.Rigging.RigNode as RigNode
#
import Otherside.Rigging.ControlRig.Head
import Otherside.Rigging.ControlRig.Torso
import Otherside.Rigging.ControlRig.Shoulder
import Otherside.Rigging.ControlRig.Arm
import Otherside.Rigging.ControlRig.Hand
import Otherside.Rigging.ControlRig.Finger
import Otherside.Rigging.ControlRig.Leg
import Otherside.Rigging.ControlRig.HindLeg
import Otherside.Rigging.ControlRig.ToeSet
import Otherside.Rigging.ControlRig.Prop
import Otherside.Rigging.ControlRig.PropChain
import Otherside.Rigging.ControlRig.Placement
'''
Dict of control rig types
# used for lazy initialization of modules when loaded from a text file
'''
RIGS = {}
RIGS["Head"] = Otherside.Rigging.ControlRig.Head.Head
RIGS["Torso"] = Otherside.Rigging.ControlRig.Torso.Torso
RIGS["Shoulder"] = Otherside.Rigging.ControlRig.Shoulder.Shoulder
RIGS["Arm"] = Otherside.Rigging.ControlRig.Arm.Arm
RIGS["Hand"] = Otherside.Rigging.ControlRig.Hand.Hand
RIGS["Finer"] = Otherside.Rigging.ControlRig.Finger.Finger
RIGS["Leg"] = Otherside.Rigging.ControlRig.Leg.Leg
RIGS["HindLeg"] = Otherside.Rigging.ControlRig.HindLeg.HindLeg
RIGS["ToeSet"] = Otherside.Rigging.ControlRig.ToeSet.ToeSet
RIGS["Prop"] = Otherside.Rigging.ControlRig.Prop.Prop
RIGS["PropChain"] = Otherside.Rigging.ControlRig.PropChain.PropChain
RIGS["Placement"] = Otherside.Rigging.ControlRig.Placement.Placement
#


''' READ CHARACTER DATA '''
def read_character_data(character_node):
    # load the character instance
    instance = Character.load(character_node)
    # initialize output data structure
    output = OrderedDict()
    output["name"] = instance.instanceName
    output["module_data"] = []
    output["bone_data"] = []
    output["marker_data"] = []
    # modules
    for mod in instance.moduleList:
        data = {}
        data["name"] = mod.instanceName
        data["type"] = cmds.getAttr("{}.className".format(mod.node))
        data["side"] = mod.side
        #
        output["module_data"].append(data)
    # bone list
    for mod in instance.fullModuleList:
        data = {}
        data["name"] = mod.instanceName
        # strip namespaces
        bones = mod.getBoneList()
        for i in range(0,len(bones)):
            bones[i] = strip_namespace(bones[i])
        data["bones"] = bones

        #
        output["bone_data"].append(data)
    # marker list
    for mod in instance.fullModuleList:
        #
        rotation_offsets = []
        rest_positions = []
        for marker in mod.getMarkerList():
            if marker:
                #
                rot = cmds.getAttr("{}.rotate".format(marker))[0]
                rotation_offsets.append(rot)
                #
                rest = cmds.getAttr("{}.restPosition".format(marker))[0]
                rest_positions.append(rest)
        #
        data = {}
        data["name"] = mod.instanceName
        data["rotation_offsets"] = rotation_offsets
        data["rest_positions"] = rest_positions
        #
        output["marker_data"].append(data)
    #return result
    return json.dumps(output, indent=4, sort_keys=False)



''' APPLY CHARACTER DATA '''
def apply_character_data(character_data, **kwargs):
    overrideName = kwargs.get("overrideName", "")
    root = kwargs.get("root", None)
    rig = kwargs.get("rig", False)
    boneMap = None
    if root:
        print(root)
        boneMap = get_bone_map(root)
        print(boneMap)
    # character instance
    character = Character.Character()
    if len(overrideName) > 0:
        character.instanceName = overrideName
    else:
        character.instanceName = character_data["name"]
    # add modules
    for data in character_data["module_data"]:
        type = data["type"]
        name = data["name"]
        side = data["side"]
        instance = RIGS[type](name, side=side)
        character.addModule(instance)
        #
        print("Add Module -> {}".format(name))
    # charcter | create node network
    character.createNode()
    # modules -> set bone lists
    for data in character_data["bone_data"]:
        #
        name = data["name"]
        bone_list = data["bones"]
        if boneMap:
            for i in range(0,len(bone_list)):
                bone_list[i] = boneMap[bone_list[i]]
        #
        mod = character.getModuleByName(name)
        mod.setBoneList(bone_list)
        #
        print("Set Bone Data -> {}".format(name))
        # attach bones to node
        bone_names = mod.getBoneNames()
        for i in range(0,len(bone_names)):
            RigNode.setPlug(mod.node, bone_names[i], bone_list[i])
    # reload character instance
    '''
    this triggers a forced sync of subnodes
    without this subnodes can have their own ophaned instances which create problems during the characterization phase.
    not relavant after this step as re-loading the character in the future will do so based off connectiosn -
    '''
    character = Character.load(character.node)
    # character | characterize
    character.characterize()
    # modules -> orient markers
    for data in character_data["marker_data"]:
        name = data["name"]
        rotation_offsets = data["rotation_offsets"]
        rest_positions = data["rest_positions"]
        # |- get module / markers
        mod = character.getModuleByName(name)
        marker_list = mod.getMarkerList()
        # |- apply marker data
        if len(rotation_offsets) == len(marker_list):
            for i in range(0, len(rotation_offsets)):
                if marker_list[i]:
                    # |- set rotation_offsets
                    cmds.xform(marker_list[i], os=True, ro=rotation_offsets[i])
                    # |- set rest position
                    cmds.setAttr("{}.restPosition".format(marker_list[i]),
                        rest_positions[i][0], rest_positions[i][1], rest_positions[i][2] )
        else:
            print("error: mismatch marker_list - skip")
        #
        print("Set Marker Data -> {}".format(name))
    #Rig the Character
    if rig:
        character.rig()
    return character.node


'''Strip Namespace'''
def strip_namespace(path):
    return re.sub('\|[\w]+\:+', '|', path, flags=re.DOTALL)


'''Get Bone Map'''
def get_bone_map(root):
    bone_list = cmds.listRelatives(root, ad=True, type="transform", f=True)
    bone_list.append(root)
    bone_map = {}
    root_short = root.split("|")[-1]
    root_short = "|"+root_short
    truncate_prefix = root.replace(root_short, "")
    truncate_prefix = strip_namespace(truncate_prefix)
    for bone in bone_list:
        key = strip_namespace(bone)
        key = key.replace(truncate_prefix, "")
        bone_map[key] = bone
    print("Bone Map Complete")
    return bone_map


''' WRITE TEXT FILE '''
def write_text_file(filename, text):
    file = open(filename, "w")
    file.write(text)
    file.close()
    return filename


''' READ TEXT FILE '''
def read_text_file(filename):
    file = open(filename, "r")
    text = file.read()
    file.close()
    return text


''' EXPORT CHARACTER DATA '''
def export_definition(character_node, filename):
    json = read_character_data(character_node)
    write_text_file(filename, json)
    return filename


''' Import Character Data'''
def import_definition(name, root, filename, **kwargs):
    rig = kwargs.get("rig", False)
    raw_data = read_text_file(filename)
    data = json.loads(raw_data)
    node = apply_character_data(data, overrideName=name, root=root, rig=rig)
    return node