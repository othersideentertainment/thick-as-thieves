import os
import imp
from functools import partial
import maya.cmds as cmds
import maya.mel as mel
import Otherside.Rigging.RigPose as RigPose
import Otherside.exportFBX as exportFBX


def showMenu():
    othersideMenuName = "Otherside"
     #close existing menu
    if cmds.menu(othersideMenuName, exists=True):
        cmds.deleteUI(othersideMenuName)
    #register the menu with the main window
    mel.eval('global string $gMainWindow; setParent $gMainWindow;')
    othersideMenuName = cmds.menu(othersideMenuName, label=othersideMenuName, tearOff=True)

    #---------- Menu ----------
    cmds.menuItem(l="Utility", divider=True)
    cmds.menuItem(l="Copy Paste Tool", stp="python", c="Otherside.Tools.CopyPasteTool.createUI()")
    cmds.menuItem(l="Convert Up Axis (Skeletal Animation)", subMenu=True)
    cmds.menuItem(l="Active", stp="python", c=(partial(_convertSkeletalUpAxisActive)))
    cmds.menuItem(l="FBX", stp="python", c=(partial(_convertSkeletalUpAxisFBX)))
    cmds.menuItem(l="Batch", stp="python", c=(partial(_convertSkeletalUpAxisBatch)))
    cmds.setParent("..", menu=True)

    #-ControlRig Animation
    cmds.menuItem(l="Animation (Control Rig)", divider=True)
    cmds.menuItem(l="Animator Tools", stp="python", c="Otherside.Rigging.Tools.AnimTools.showWindow()")
    cmds.menuItem(l="First Person Camera Space Switch", stp="python", c="Otherside.Tools.FirstPersonCameraSpaceSwitch.showWindow()")
    cmds.menuItem(l="Select Keyable (Character)", stp="python", c=(partial(_selectKeyableCharacter)))
    cmds.menuItem(l="Select Keyable (Current Module)", stp="python", c=(partial(_selectKeyableModule)))

    #-ControlRig Rigging
    cmds.menuItem(l="Rigging (Control Rig)", divider=True)
    cmds.menuItem(l="ControlRig Setup Wizard", stp="python", c=(partial(_openSetupWizard)))
    cmds.menuItem(l="Characterize Skel. Wizard", stp="python", c=(partial(_openCharacterizeWizard)))
    cmds.menuItem(l="Character Def. Importer", stp="python", c=(partial(_openCharacterDefinitionImporter)))
    cmds.menuItem(l="Character Def. Exporter", stp="python", c=(partial(_openCharacterDefinitionExporter)))
    cmds.menuItem(l="Character Lister", stp="python", c=(partial(_openCharacterLister)))
    cmds.menuItem(l="Load Rig Pose", stp="python", c=(partial(_assumeRigPose)))
    cmds.menuItem(l="Link Root Space", stp="python", c=(partial(_linkRootSpace)))

    #-ControlRig Retargeting
    cmds.menuItem(l="Retargeting (Control Rig)", divider=True)
    cmds.menuItem(l="Transfer Pose", stp="python", c=(partial(_openTransferPoseEditor)))
    cmds.menuItem(l="Transfer Animation", stp="python", c=(partial(_openTransferAnimationEditor)))
    cmds.menuItem(l="Retarget From File", stp="python", c=(partial(_openRetargetFromFile)))
    cmds.menuItem(l="Batch Retarget From File", stp="python", c=(partial(_openBatchRetargetFromFile)))

    #-Export
    cmds.menuItem(l="Export FBX", divider=True)
    cmds.menuItem(l="Export Animation", stp="python", c=partial(_exportFBX))
    cmds.menuItem(l="Batch Export Animation", stp="python", c=partial(_batchExportFBX))
    
    
    #-Scripts
    cmds.menuItem(l="Scripts", divider=True)
    cmds.menuItem(l="Rebuild Rigging", stp="python", c=partial(_rebuildRigging))
    cmds.setParent('..', menu=True)

#----------- HELPER MEHTODS --------------#

def _openCharacterDefinitionImporter(*args):
    import Otherside.Rigging.Tools.CharacterDefinitionImporter as CharacterDefinitionImporter
    CharacterDefinitionImporter.Show()


def _openCharacterDefinitionExporter(*args):
    import Otherside.Rigging.Tools.CharacterDefinitionExporter as CharacterDefinitionExporter
    CharacterDefinitionExporter.Show()


def _openRetargetFromFile(*args):
    import Otherside.Rigging.Retargeting.RetargetFromFile as RetargetFromFile
    RetargetFromFile.show()


def _openBatchRetargetFromFile(*args):
    import Otherside.Rigging.Retargeting.BatchRetargetFromFile as BatchRetargetFromFile
    BatchRetargetFromFile.show()


def _openSetupWizard(*args):
    from Otherside.Rigging.Setup.SetupWizard import SetupWizard
    SetupWizard.Show()


def _openCharacterizeWizard(*args):
    from Otherside.Rigging.Setup.CharacterizeWizard import CharacterizeWizard
    CharacterizeWizard.Show()


def _openCharacterLister(*args):
    import Otherside.Rigging.Tools.CharacterLister as CharacterLister
    CharacterLister.showWindow()


def _openTransferAnimationEditor(*args):
    import Otherside.Rigging.Tools.TransferAnimation as TransferAnimation
    TransferAnimation.showWindow()


def _openTransferPoseEditor(*args):
    import Otherside.Rigging.Tools.TransferPose as TransferPose
    TransferPose.showWindow()


def _assumeRigPose(*args, **kwargs):
    root = cmds.ls(sl=True, fl=True, l=True, type="transform")
    if root:
        root = root[0]
    RigPose.assumeRigPose(root=root)


def _linkRootSpace(*args, **kwargs):
    import Otherside.Rigging.Tools.LinkRootSpace as LinkRootSpace
    LinkRootSpace.showWindow()


def _convertSkeletalUpAxisActive(*args, **kwargs):
    import Otherside.Rigging.Tools.ConvertUpAxis as ConvertUpAxis
    ConvertUpAxis.convert_up_axis()


def _convertSkeletalUpAxisFBX(*args, **kwargs):
    import Otherside.Rigging.Tools.ConvertUpAxis as ConvertUpAxis
    paths = cmds.fileDialog2(fm=1, okc="Select", cap="Convert FBX", ff="*.fbx")
    ConvertUpAxis.convert_fbx(paths[0])


def _convertSkeletalUpAxisBatch(*args, **kwargs):
    import Otherside.Rigging.Tools.ConvertUpAxis as ConvertUpAxis
    paths = cmds.fileDialog2(fm=2, okc="Select", cap="Batch Convert Directory", ff="*.fbx")
    ConvertUpAxis.convert_dir(paths[0])


def _selectKeyableCharacter(*args, **kwargs):
    import Otherside.Rigging.RigNode as RigNode
    selectionList = []
    sel = cmds.ls(sl=True, fl=True)
    if sel:
        rootNode = RigNode.getRoot(sel[0])
        instance = RigNode.load(rootNode)
        selectionList = instance.getKeyable()
    cmds.select(selectionList, r=True)


def _selectKeyableModule(*args, **kwargs):
    import Otherside.Rigging.RigNode as RigNode
    selectionList = []
    sel = cmds.ls(sl=True, fl=True)
    if sel:
        node = None
        #
        if RigNode.isRigNode(sel[0]):
            node = sel[0]
        else:
            node = RigNode.getRelated(sel[0])
        #
        if node:
            instance = RigNode.load(node)
            selectionList = instance.getKeyable()
    cmds.select(selectionList, r=True)


def _exportFBX(*args):
    import Otherside.exportFBX as exportFBX
    exportFBX.exportSceneAnim()
    

def _batchExportFBX(*args):
    import Otherside.exportFBX as exportFBX
    
    current = cmds.workspace(q=1, dir=1)
    topdir = ''
    if os.path.isdir(current):
        topdir = cmds.fileDialog2(cap='Select Top Folder', fileMode=2, dir=current)[0]
    else:
        topdir = cmds.fileDialog2(cap='Select Top Folder', fileMode=2)[0]
    
    if os.path.isdir(topdir):
        exportFBX.batch('anim', topdir)
    else:
        raise IOError('{} is not a valid directory!'.format(topdir))
    
    
def _rebuildRigging(*args):
    import Otherside.Rigging.Rebuild
    imp.reload(Otherside.Rigging.Rebuild)


