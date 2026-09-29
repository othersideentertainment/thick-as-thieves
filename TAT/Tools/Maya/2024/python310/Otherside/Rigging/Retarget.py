import maya.cmds as cmds
import Otherside.Rigging.RigNode as RigNode
import Otherside.Rigging.ControlRig.Character as Character

#
def copyModuleAttributes(source, destination, *args, **kwargs):
    #Get Keyable Objects
    source_controller_list = source.getKeyable()
    destination_controller_list = destination.getKeyable()
    #Loop through object lists and copy attributes
    for i in range(0,len(source_controller_list)):
        #current source/destination
        source_control = source_controller_list[i]
        destination_control = destination_controller_list[i]
        if source_control != None and destination_control != None:
            #current attribute list
            attrList = cmds.listAttr(source_control, k=True)
            #perform the copy
            cmds.copyAttr(source_control, destination_control, values=True, at=attrList)


#
def copyModuleKeyframes(source, destination, startFrame, endFrame, *args, **kwargs):
    #Get Keyable Objects
    source_controller_list = source.getKeyable()
    destination_controller_list = destination.getKeyable()
    #Loop through object lists and copy attributes
    for i in range(0,len(source_controller_list)):
        #current source/destination
        source_control = source_controller_list[i]
        destination_control = destination_controller_list[i]
        if source_control != None and destination_control != None:
            keys = cmds.keyframe(source_control, q=True)
            if keys != None:
                #perform the copy
                cmds.copyKey(source_control, option="curve", t=(startFrame, endFrame))
                cmds.pasteKey(destination_control, option="replace")


#
def copyPose(sourceCharacterNode, destinationCharacterNode, *args, **kwargs):
    #Validate Input
    if (
        RigNode.isType(sourceCharacterNode, "Character") == False or
        RigNode.isType(destinationCharacterNode, "Character") == False
        ):
        print ("Invalid Input - requires two character nodes")
        return
    # load the nodes and get character instances
    source_character = RigNode.load(sourceCharacterNode)
    destination_character = RigNode.load(destinationCharacterNode)
    # get a list of module names from source character
    mod_list = source_character.fullModuleList
    for mod in mod_list:
        #Find our matching destination module
        destination_mod = destination_character.getModuleByName(mod.instanceName)
        # if the destination is value run the command
        if destination_mod != None:
            copyModuleAttributes(mod, destination_mod)


#
def copyAnimation(sourceCharacterNode, destinationCharacterNode, *args, **kwargs):
    #Validate Input
    if (
        RigNode.isType(sourceCharacterNode, "Character") == False or
        RigNode.isType(destinationCharacterNode, "Character") == False
        ):
        print ("Invalid Input - requires two character nodes")
        return
    #
    default_start = cmds.playbackOptions(q=True, min=True)
    default_end = cmds.playbackOptions(q=True, max=True)
    start_frame = kwargs.get("startFrame", default_start)
    end_frame = kwargs.get("endFrame", default_end)
    # load the nodes and get character instances
    source_character = RigNode.load(sourceCharacterNode)
    destination_character = RigNode.load(destinationCharacterNode)
    # get a list of module names from source character
    mod_list = source_character.fullModuleList
    for mod in mod_list:
        #Find our matching destination module
        destination_mod = destination_character.getModuleByName(mod.instanceName)
        #if the destination is value run the command
        if destination_mod != None:
            copyModuleKeyframes(mod, destination_mod, start_frame, end_frame)


#
def retargetPose(sourceNode, destinationNode, *args, **kwargs):
    character = Character.load(destinationNode)
    #
    torso = character.getModuleByName("torso")
    if torso:
        cmds.setAttr(torso.controlGroup+".ikMode", 0)
    #
    character.sync(sourceNode)


#
def retargetAnimation(sourceNode, destinationNode, *args, **kwargs):
    default_start = cmds.playbackOptions(q=True, min=True)
    default_end = cmds.playbackOptions(q=True, max=True)
    start_frame = kwargs.get("startFrame", default_start)
    end_frame = kwargs.get("endFrame", default_end)
    character = Character.load(destinationNode)
    placementMode = kwargs.get("placementMode", "root")
    #print(placementMode + " from retarget")
    character.transferAnimation(sourceNode, startFrame=start_frame, endFrame=end_frame, placementMode=placementMode)

