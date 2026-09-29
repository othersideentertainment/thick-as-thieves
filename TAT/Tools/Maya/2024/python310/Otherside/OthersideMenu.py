import os
import imp
from functools import partial
import maya.cmds as cmds
import maya.mel as mel
import Otherside.Rigging.RigPose as RigPose
import Otherside.Pipe.PymelInstall as PymelInstall


#get root path with trailing slash
SITE_ROOT = os.path.abspath(__file__).split('python310')[0].replace('\\','/')
oseIcon = SITE_ROOT + 'share/icons/ose.bmp'

def showMenu():
    othersideMenuName = "Otherside"
     #close existing menu
    if cmds.menu(othersideMenuName, exists=True):
        cmds.deleteUI(othersideMenuName)
    #register the menu with the main window
    mel.eval('global string $gMainWindow; setParent $gMainWindow;')
    othersideMenuName = cmds.menu(othersideMenuName, label=othersideMenuName, tearOff=True, familyImage=oseIcon)

    #---------- Menu ----------
    cmds.menuItem(l="Export", divider=True)    
    cmds.menuItem(l="Open Explorer To Scene Path", stp="python", c=(partial(_exploreCurrentScenePath)))
    cmds.menuItem(l="Open Explorer To Export Path", stp="python", c=(partial(_exploreExportScenePath)))
    cmds.menuItem(l="Export Settings...", stp="python", c=(partial(_exportSettings)))
    cmds.menuItem(l="Export Animation", stp="python", c=partial(_exportFBXAnim))
    cmds.menuItem(l="Batch Export Animation", stp="python", c=partial(_batchExportFBXAnim))
    cmds.menuItem(l="Export Bind", stp="python", c=partial(_exportFBXBind))
    cmds.menuItem(l="Batch Export Bind", stp="python", c=partial(_batchExportFBXBind))
    
    cmds.menuItem(l="Utility", divider=True)    
    cmds.menuItem(l="Copy Paste Tool...", stp="python", c="Otherside.Tools.CopyPasteTool.createUI()")
    cmds.menuItem(l="Convert Up Axis (Skeletal Animation)", subMenu=True)
    cmds.menuItem(l="Active", stp="python", c=(partial(_convertSkeletalUpAxisActive)))
    cmds.menuItem(l="FBX", stp="python", c=(partial(_convertSkeletalUpAxisFBX)))
    cmds.menuItem(l="Batch", stp="python", c=(partial(_convertSkeletalUpAxisBatch)))
    cmds.setParent("..", menu=True)
    cmds.menuItem(l="Display Layers to Notes...", stp="python", c=(partial(_displayLayersToNotesUI)))

    #-ControlRig Animation
    cmds.menuItem(l="Animation (Control Rig)", divider=True)
    cmds.menuItem(l="Animator Tools...", stp="python", c="Otherside.Rigging.Tools.AnimTools.showWindow()")
    cmds.menuItem(l="First Person Camera Space Switch...", stp="python", c="Otherside.Tools.FirstPersonCameraSpaceSwitch.showWindow()")
    cmds.menuItem(l="Select Keyable (Character)", stp="python", c=(partial(_selectKeyableCharacter)))
    cmds.menuItem(l="Select Keyable (Current Module)", stp="python", c=(partial(_selectKeyableModule)))
    cmds.menuItem(l="Hide/Show on Playback", subMenu=True)
    cmds.menuItem(l="Hide on Playback", stp="python", c=(partial(_hideOnPlayback)))
    cmds.menuItem(l="Show on Playback", stp="python", c=(partial(_showOnPlayback)))
    cmds.setParent("..", menu=True)

    #-ControlRig Rigging
    cmds.menuItem(l="Rigging (Control Rig)", divider=True)
    cmds.menuItem(l="ControlRig Setup Wizard...", stp="python", c=(partial(_openSetupWizard)))
    cmds.menuItem(l="Characterize Skel. Wizard...", stp="python", c=(partial(_openCharacterizeWizard)))
    cmds.menuItem(l="Character Def. Importer...", stp="python", c=(partial(_openCharacterDefinitionImporter)))
    cmds.menuItem(l="Character Def. Exporter...", stp="python", c=(partial(_openCharacterDefinitionExporter)))
    cmds.menuItem(l="Character Lister...", stp="python", c=(partial(_openCharacterLister)))
    cmds.menuItem(l="Load Rig Pose", stp="python", c=(partial(_assumeRigPose)))
    cmds.menuItem(l="Link Root Space...", stp="python", c=(partial(_linkRootSpace)))
    cmds.menuItem(l="Transfer Single Control Shape", stp="python", c=(partial(_transferSingleCtrlShape)))
    cmds.menuItem(l="Transfer Control Shapes", stp="python", c=(partial(_transferCtrlShapes)))

    #-ControlRig Retargeting
    cmds.menuItem(l="Retargeting (Control Rig)", divider=True)
    cmds.menuItem(l="Transfer Pose...", stp="python", c=(partial(_openTransferPoseEditor)))
    cmds.menuItem(l="Transfer Animation...", stp="python", c=(partial(_openTransferAnimationEditor)))
    cmds.menuItem(l="Retarget From File...", stp="python", c=(partial(_openRetargetFromFile)))
    cmds.menuItem(l="Batch Retarget From File", stp="python", c=(partial(_openBatchRetargetFromFile)))
    
    #-Scripts
    cmds.menuItem(l="Scripts", divider=True)
    cmds.menuItem(l="Rebuild Rigging", stp="python", c=partial(_rebuildRigging))
    cmds.setParent('..', menu=True)
    
    #-Pymel
    if not PymelInstall.PymelInstall().hasTargetPymel():
        cmds.menuItem(l="Pymel", divider=True)
        cmds.menuItem(l="Install Pymel...", stp="python", c=partial(_installPymel))
        cmds.setParent('..', menu=True)

#----------- HELPER MEHTODS --------------#

def _exportSettings(*args):
    import Otherside.Pipe.exportSettings as exportSettings
    exportSettings.show()
    
    
def _exploreCurrentScenePath(*args):
    import os
    os.startfile(os.path.dirname(cmds.file(q=True, sceneName=True)))
    
    
def _exploreExportScenePath(*args):
    import os
    
    sceneName = cmds.file(q=True, sceneName=True)
    if not sceneName:
        return
    sceneDir = os.path.dirname(sceneName)
    
    #use export settings?
    if cmds.optionVar(exists='OSE_ArtRoot'):
        artRoot = cmds.optionVar(q='OSE_ArtRoot')
    else:
        artRoot = ""
    if cmds.optionVar(exists='OSE_GameRoot'):
        gameRoot = cmds.optionVar(q='OSE_GameRoot')
    else:
        gameRoot = "" 
    
    if os.path.isdir(artRoot) and os.path.isdir(gameRoot):
        pth = sceneDir.replace(artRoot, gameRoot)+'/'
    else:
        pth = sceneDir + '/Export/' 
    orgpath = pth
        
    #find the longest path to our export path that exists on disk
    while not os.path.isdir(pth):
        pth = os.path.dirname(pth)
        if pth == "":
            break
    
    if os.path.isdir(pth):
        os.startfile(pth)
        if pth != orgpath:
            cmds.warning('Export path "{}" does not exist!'.format(orgpath))
    else:
        cmds.warning('Export path "{}" does not exist!'.format(orgpath))
        
    
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


def _transferSingleCtrlShape(*args):
    import Otherside.Rigging.Controller as Controller
    Controller.transferSingleControlShape()


def _transferCtrlShapes(*args):
    import Otherside.Rigging.Controller as Controller
    Controller.transferControlShapesByName()
    
    
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


def _hideOnPlayback(*args):
    sel = cmds.ls(sl=1)
    for s in sel:
        cmds.setAttr(s+".hideOnPlayback", 1)
        
        
def _showOnPlayback(*args):
    sel = cmds.ls(sl=1)
    for s in sel:
        cmds.setAttr(s+".hideOnPlayback", 0)


def _exportFBXAnim(*args):
    import Otherside.Pipe.exportFBX as exportFBX
    exportFBX.exportSceneAnim()
    
    
def _batchExportFBXAnim(*args):
    import Otherside.Pipe.exportFBX as exportFBX
    
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


def _exportFBXBind(*args):
    import Otherside.Pipe.exportFBX as exportFBX
    exportFBX.exportSceneBind()


def _batchExportFBXBind(*args):
    import Otherside.Pipe.exportFBX as exportFBX
    
    current = cmds.workspace(q=1, dir=1)
    topdir = ''
    if os.path.isdir(current):
        topdir = cmds.fileDialog2(cap='Select Top Folder', fileMode=2, dir=current)[0]
    else:
        topdir = cmds.fileDialog2(cap='Select Top Folder', fileMode=2)[0]
    
    if os.path.isdir(topdir):
        exportFBX.batch('bind', topdir)
    else:
        raise IOError('{} is not a valid directory!'.format(topdir))


def _displayLayersToNotesUI(*args):
    import Otherside.Rigging.Tools.DisplayLayersToNotes as dltn
    dltn.showWindow()


def _rebuildRigging(*args):
    import Otherside.Rigging.Rebuild
    imp.reload(Otherside.Rigging.Rebuild)


def _installPymel(*args):
    import Otherside.Pipe.PymelInstall as PymelInstall
    PymelInstall.PymelInstall().installPymel()
