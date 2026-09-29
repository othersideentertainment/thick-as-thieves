import maya.cmds as cmds

from importlib import reload
import Otherside as Otherside
reload(Otherside)

# cmds.evalDeferred("Otherside.initialize()")
cmds.evalDeferred('import Otherside.Pipe.Startup as Startup;', lowestPriority=True)
