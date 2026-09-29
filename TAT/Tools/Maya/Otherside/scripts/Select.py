import maya.cmds as cmds

def List (sel=cmds.ls(sl=1)) :
   #target = sel[-1]
   return sel

def Type (str) : 
   sel = cmds.ls(sl=1, type=str)
   return sel
