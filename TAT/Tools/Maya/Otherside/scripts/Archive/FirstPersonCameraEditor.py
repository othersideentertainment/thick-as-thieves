import maya.cmds as cmds 
from functools import partial

class EditorWindow():
    _windowName = "firstPersonCameraWindow"
    _rotatePivot = "|CameraRig|CameraSpace|RotatePivot"
    _translatePivot = "|CameraRig|CameraSpace|RotatePivot|TranslatePivot"

    @staticmethod
    def KeyCameraAll(*args):
        cmds.setKeyframe('%s.tx' % EditorWindow._translatePivot)
        cmds.setKeyframe('%s.ty' % EditorWindow._translatePivot)
        cmds.setKeyframe('%s.tz' % EditorWindow._translatePivot)
        cmds.setKeyframe('%s.rx' % EditorWindow._rotatePivot)
        cmds.setKeyframe('%s.ry' % EditorWindow._rotatePivot)
        cmds.setKeyframe('%s.rz' % EditorWindow._rotatePivot)
        #cmds.setKeyframe('%s.focalLength' % cameraObject)

    @staticmethod
    def KeyCameraTranslation(mode, *args):
        if mode == 0:
            cmds.setKeyframe(EditorWindow._translatePivot+".tx")
        elif mode == 1:
            cmds.setKeyframe(EditorWindow._translatePivot+".ty")
        elif mode == 2:
            cmds.setKeyframe(EditorWindow._translatePivot+".tz")
        elif mode == 3:
            cmds.setKeyframe(EditorWindow._translatePivot+".tx")
            cmds.setKeyframe(EditorWindow._translatePivot+".ty")
            cmds.setKeyframe(EditorWindow._translatePivot+".tz")

    @staticmethod
    def KeyCameraRotation(mode, *args):
        if mode == 0:
            cmds.setKeyframe(EditorWindow._rotatePivot+".rx")
        elif mode == 1:
            cmds.setKeyframe(EditorWindow._rotatePivot+".ry")
        elif mode == 2:
            cmds.setKeyframe(EditorWindow._rotatePivot+".rz")
        elif mode == 3:
            cmds.setKeyframe(EditorWindow._rotatePivot+".rx")
            cmds.setKeyframe(EditorWindow._rotatePivot+".ry")
            cmds.setKeyframe(EditorWindow._rotatePivot+".rz")

    @staticmethod
    def ResetCameraAll(*args):        
        cmds.setAttr(EditorWindow._translatePivot+".tx", 0)
        cmds.setAttr(EditorWindow._translatePivot+".ty", 0)
        cmds.setAttr(EditorWindow._translatePivot+".tz", 0)
        cmds.setAttr(EditorWindow._rotatePivot+".rx", 0)
        cmds.setAttr(EditorWindow._rotatePivot+".ry", 0)
        cmds.setAttr(EditorWindow._rotatePivot+".rz", 0)
        #cmds.setKeyframe('%s.focalLength' % cameraObject)

    @staticmethod
    def ResetCameraTranslation(mode, *args):
        if mode == 0:
            cmds.setAttr(EditorWindow._translatePivot+".tx", 0)
        elif mode == 1:
            cmds.setAttr(EditorWindow._translatePivot+".ty", 0)
        elif mode == 2:
            cmds.setAttr(EditorWindow._translatePivot+".tz", 0)
        elif mode == 3:
            cmds.setAttr(EditorWindow._translatePivot+".tx", 0)
            cmds.setAttr(EditorWindow._translatePivot+".ty", 0)
            cmds.setAttr(EditorWindow._translatePivot+".tz", 0)

    @staticmethod
    def ResetCameraRotation(mode, *args):
        if mode == 0:
            cmds.setAttr(EditorWindow._rotatePivot+".rx", 0)
        elif mode == 1:
            cmds.setAttr(EditorWindow._rotatePivot+".ry", 0)
        elif mode == 2:
            cmds.setAttr(EditorWindow._rotatePivot+".rz", 0)
        elif mode == 3:
            cmds.setAttr(EditorWindow._rotatePivot+".rx", 0)
            cmds.setAttr(EditorWindow._rotatePivot+".ry", 0)
            cmds.setAttr(EditorWindow._rotatePivot+".rz", 0)

    @staticmethod
    def SelectRotatePivot(*args):
        cmds.select(EditorWindow._rotatePivot, r=True)

    @staticmethod
    def SelectTranslatePivot(*args):
        cmds.select( EditorWindow._translatePivot, r=True)

    @staticmethod
    def SelectAll(*args):
        cmds.select( EditorWindow._translatePivot, EditorWindow._rotatePivot, r=True)        

    @staticmethod
    def Show(*args):
        #Camera Values
        rotatePivot = "|CameraRig|CameraSpace|RotatePivot"
        translatePivot = "|CameraRig|CameraSpace|RotatePivot|TranslatePivot"
        translateRange = 3
        rotateRange = 90

        windowName = "firstPersonCameraWindow"
        if (cmds.window(windowName, exists=True)):
            cmds.deleteUI( windowName, window=True )   
        if (cmds.windowPref(windowName, exists=True)):
            cmds.windowPref(windowName, r=True)

        #Window
        cmds.window(windowName, title='Camera Control' )
        labelWidth = 65
        sliderWidth = 350
        columnWidth = labelWidth + sliderWidth
        cmds.columnLayout(cal="left")

        #Translate Pivot        
        cmds.rowColumnLayout(numberOfColumns=2, cal=(1,"left"), columnWidth=[(1, columnWidth), (2, 102)])
        cmds.text(l="Translate", font="boldLabelFont", height=25)
        cmds.button("Select", width=102, c=EditorWindow.SelectTranslatePivot)
        cmds.setParent("..")
        #
        cmds.rowColumnLayout(numberOfColumns=4)        
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-translateRange, max=translateRange, at='%s.tx' % translatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraTranslation, 0))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraTranslation, 0))
        #
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-translateRange, max=translateRange, at='%s.ty' % translatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraTranslation, 1))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraTranslation, 1))
        #
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-translateRange, max=translateRange, at='%s.tz' % translatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraTranslation, 2))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraTranslation, 2))
        #
        cmds.text(l="", width=2)
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraTranslation, 3))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraTranslation, 3))

        cmds.setParent('..')
        
        #--
        cmds.text(l="", height=10)
        cmds.separator(width=(columnWidth+102))
        #--

        #Rotate Pivot
        cmds.rowColumnLayout(numberOfColumns=2, cal=(1,"left"), columnWidth=[(1, columnWidth), (2, 102)])
        cmds.text(l="Rotate", font="boldLabelFont", height=25)
        cmds.button("Select", width=102, c=EditorWindow.SelectRotatePivot)
        cmds.setParent("..")
        #
        cmds.rowColumnLayout(numberOfColumns=4)
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-rotateRange, max=rotateRange, at='%s.rx' % rotatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraRotation, 0))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraRotation, 0))
        #
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-rotateRange, max=rotateRange, at='%s.ry' % rotatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraRotation, 1))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraRotation, 1))
        #
        cmds.attrFieldSliderGrp( width=columnWidth, cw=[1,labelWidth], cal=[1, "left"], min=-rotateRange, max=rotateRange, at='%s.rz' % rotatePivot )
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraRotation, 2))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraRotation, 2))        
        #
        cmds.text(l="", width=2)
        cmds.button(l="key", width = 50, c=partial(EditorWindow.KeyCameraRotation, 3))
        cmds.text(l="", width=2)
        cmds.button(l="reset", width = 50, c=partial(EditorWindow.ResetCameraRotation, 3))
        cmds.setParent('..')
        #--
        cmds.text(l="", height=10)
        cmds.separator(width=(columnWidth+102))
        #--

        #Controls
        cmds.rowColumnLayout( numberOfColumns=10)    
        cmds.button(l="Select All", c=EditorWindow.SelectAll, width=100)
        cmds.text(l="", height=2, width=2)
        cmds.button(l="Key All", c=EditorWindow.KeyCameraAll, width=100)
        cmds.text(l="", height=2, width=2)
        cmds.button(l="Reset All", c=EditorWindow.ResetCameraAll, width=100)
        cmds.setParent('..')
        cmds.showWindow()


def ShowWindow(*args):
    EditorWindow.Show()