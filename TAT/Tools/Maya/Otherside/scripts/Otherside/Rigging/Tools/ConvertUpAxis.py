import os
import maya.cmds as cmds
import maya.mel as mel


### Optional Parameters
#- root
#- startFrame
#- endFrame
###
def convert_up_axis(*args, **kwargs):
    #prep scene
    # |- remove all namespaces
    remove_all_namespaces()
    #Get Inputs
    mel.eval("setPlaybackRangeToMinMax")
    rootNode= kwargs.get("root", "root")
    startFrame  = kwargs.get("startFrame", int(cmds.playbackOptions(q=True, min=True)))
    endFrame    = kwargs.get("endFrame", int(cmds.playbackOptions(q=True, max=True)))
    # Check for Y-up animation
    jo = cmds.getAttr(rootNode+".jointOrient")[0]
    if jo[0] == 0 and jo[1] == 0 and jo[2] == 0:
        print("Y-Up animation found - aborting")
        return
    # Get Lists of Children
    children = cmds.listRelatives(rootNode, children=True)
    animated_children = []
    static_children = []
    for child in children:
        if cmds.keyframe(child, q=True):
            animated_children.append(child)
        else:
            static_children.append(child)
    #Zero out the root orientation
    cmds.setAttr(rootNode+".jointOrient",0,0,0)
    cmds.currentTime(startFrame)
    #
    marker = cmds.createNode("transform")
    #static children
    for child in static_children:
        pivot= cmds.xform(rootNode, q=True, ws=True, t=True)
        cmds.matchTransform(marker, child)
        cmds.rotate(-90, 0, 0, marker, r=True, ws=True, fo=True, t=True, p=pivot)
        cmds.matchTransform(child, marker)

    #animated children
    for frame in range(startFrame, endFrame+1):
        cmds.currentTime(frame)
        pivot= cmds.xform(rootNode, q=True, ws=True, t=True)
        for child in animated_children:
            cmds.matchTransform(marker, child)
            cmds.rotate(-90, 0, 0, marker, r=True, ws=True, fo=True, t=True, p=pivot)
            cmds.matchTransform(child, marker)
            cmds.setKeyframe(child)
    #cleanup
    cmds.delete(marker)


def convert_fbx(filename,*args, **kwargs):
    root = kwargs.get("root", "root")
    cmds.file(new=True, f=True)
    # Using FBXImport prevents FBX Import Settings popups
    # that get triggered when using Maya dialogs in Native OS mode
    mel.eval("FBXImport -f \""+ str(filename)+"\";")
    convert_up_axis()
    cmds.select(root, r=True)
    mel.eval("FBXResetExport")
    mel.eval("FBXProperty Export|IncludeGrp|Animation -v true;")
    mel.eval("FBXExport -f \""+filename+"\" -s")


def convert_dir(path, *args, **kwargs):
    root = kwargs.get("root", "root")
    fbx_list = cmds.getFileList(folder=path)
    for fbx in fbx_list:
        filepath = path + "/" + fbx
        convert_fbx(filepath, root=root)


def remove_all_namespaces():
    ns_list = cmds.namespaceInfo(":", lon=True)
    ns_list.sort(key=len)
    ns_list.reverse()

    for ns in ns_list:
        namespace = str(ns)
        # UI and Shared are hidden system namespaces that can/should not be deleted.
        # They are however/unfortunately, returned as part of the namespace list...
        if namespace == "UI" or namespace == "shared":
            continue

        #Remove the namespace and merge all of its content into the root.
        cmds.namespace( removeNamespace = namespace, mergeNamespaceWithRoot = True)
        # this is not a global solution because name collisions could be problematic
        # however, this should work fine in this instance because we're operating in
        # a fresh scene and with animations that only contain a single skeleton.
