import maya.cmds as cmds
import Otherside as Otherside
reload(Otherside)

cmds.evalDeferred("Otherside.initialize()")