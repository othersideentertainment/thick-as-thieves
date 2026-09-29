import maya.cmds as cmds

def scaleSelected(target, size):
    cmds.setAttr(target[0]+'.scaleX', size)
    cmds.setAttr(target[0]+'.scaleY', size)
    cmds.setAttr(target[0]+'.scaleZ', size)

def createLocators(selected):
    for each in selected:
        locatorName = each+'_loc'
        print (locatorName)
        locatorScale = 20
        if cmds.objExists(locatorName):
            cmds.delete(locatorName)
            print ('deleted the thing')
            loc = cmds.spaceLocator(n=locatorName)
            scaleSelected(loc, locatorScale)
        else : 
            loc = cmds.spaceLocator(n=locatorName)
            print ('made dumb thing')
            scaleSelected(loc, locatorScale)
        cmds.matchTransform(loc,each,pos=1,rot=1,scl=0)

createLocators(cmds.ls(sl=1))