import maya.cmds as cmds
import maya.mel as mel
import MayaUtility as MayaUtility
import Workspace as Workspace
import AnimationPipeline as AnimationPipeline
import Validation as Validation
import SkinMap as SkinMap
import FirstPersonCameraEditor as FirstPersonCameraEditor
import SmearFrameEditor as SmearFrameEditor
import SmearGuideEditor as SmearGuideEditor
reload(MayaUtility)
reload(Workspace)
reload(AnimationPipeline)
reload(Validation)
reload(SkinMap)
reload(FirstPersonCameraEditor)
reload(SmearFrameEditor)
reload(SmearGuideEditor)
#import for usage in menu
import Otherside as Otherside



def initialize():
    #mayaConfig()
    showMenu()
    print "Otherside Maya Initialized"


def mayaConfig():
    #Options (Working Units)
    cmds.optionVar(sv=("workingUnitLinearDefault", "meter"))
    cmds.optionVar(sv=("workingUnitAngularDefault", "degree"))
    cmds.optionVar(sv=("workingUnitTimeDefault", "ntsc"))    
    #Grid
    currentGridSpacing = cmds.grid(q=True, spacing=True)
    currentGridSize = cmds.grid(q=True, size=True)
    cmds.currentUnit(l="meter")
    cmds.grid(spacing=currentGridSpacing, size=currentGridSize)


def showMenu():
    othersideMenuName = "Otherside"
     #close existing menu
    if cmds.menu(othersideMenuName, exists=True):
        cmds.deleteUI(othersideMenuName)
    #register the menu with the main window
    mel.eval('global string $gMainWindow; setParent $gMainWindow;')
    othersideMenuName = cmds.menu(othersideMenuName, label=othersideMenuName, tearOff=True)    
    
    #---------- Menu ----------
    cmds.menuItem(subMenu=True, label="Tools") 
    #cmds.menuItem("SkinMaps", stp="python", c=SkinMap.ShowWindow)
    cmds.menuItem(label="Skins", d=True)
    cmds.menuItem("Skin(s) Import", stp="python", c=SkinMap.importSkinSet)
    cmds.menuItem("Skin(s) Export", stp="python", c=SkinMap.exportSkinSet)
    cmds.setParent('..', menu=True)    


    '''
    #[File]
    cmds.menuItem(subMenu=True, label="File") 
    #-[Scene]
    #cmds.menuItem(label="Scene", d=True)
    #cmds.menuItem("SmartOpen", stp="python", c=Workspace.smartOpenDialog)
    #-[Animation]
    cmds.menuItem(label="Animation", d=True)
    cmds.menuItem("Animation Export", stp="python", c=AnimationPipeline.exportCurrentAnimation)
    cmds.menuItem("Animation Export (Batch)", stp="python", c=AnimationPipeline.exportMultipleAnimations)    
    cmds.menuItem(label="Skins", d=True)
    cmds.menuItem("Skin(s) Import", stp="python", c=SkinMap.importSkinSet)
    cmds.menuItem("Skin(s) Export", stp="python", c=SkinMap.exportSkinSet)    
    cmds.setParent('..', menu=True)
    #cmds.setParent('..', menu=True) #<- Close File Parent Menu

    #[Tools]
    cmds.menuItem(subMenu=True, label="Tools") 
    #cmds.menuItem("SkinMaps", stp="python", c=SkinMap.ShowWindow)
    cmds.menuItem("First Person Camera Editor", stp="python", c=FirstPersonCameraEditor.ShowWindow)
    cmds.menuItem("Smear Frame Editor", stp="python", c=SmearFrameEditor.ShowWindow)
    cmds.menuItem("Smear Guide Editor", stp="python", c=SmearGuideEditor.ShowWindow)
    cmds.setParent('..', menu=True)
    
    #[Commands]
    cmds.menuItem(subMenu=True, label="Commands")
    #-[References]
    cmds.menuItem(l="References", d=True)
    cmds.menuItem("Fix Reference Paths", stp="python", c=Workspace.updateReferencesInScene)
    cmds.menuItem("Fix Reference Paths (Batch)", stp="python", c=Workspace.batchUpdateReferencesInScene)
    #-[Utility]
    cmds.menuItem(l="Utility", d=True)
    cmds.menuItem("Update Camera Clip Planes", stp="python", c=MayaUtility.updateCameraClipPlanes)
    cmds.menuItem("Reset Transforms", stp="python", c=MayaUtility.ResetTransformSelected)
    cmds.menuItem("Match Transfroms", stp="python", c=MayaUtility.matchTransformsSelected)
    cmds.menuItem("Point + Orient Constraint", stp="python", c=MayaUtility.pointOrientConstraintSelected)
    cmds.menuItem("Convert Rotation to Orientation", stp="python", c=MayaUtility.convertRotationToOrientationSelected)    
    cmds.setParent('..', menu=True)
    '''    
