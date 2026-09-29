import maya.cmds as cmds
import Otherside.Rigging.Marker as Marker
import Otherside.Rigging.RigNode as RigNode

class ControlRigBase(object):

    def __init__(self, name="controlRig", *args, **kwargs):
        self.instanceName = name

    def getBoneList(self, *args, **kwargs):
        return []

    def getBoneNames(self, *args, **kwargs):
        return []

    def setBoneList(self, boneList, *args, **kwargs):
        pass

    def getKeyable(self):
        return []

    def getKeyableNames(self):
        return []

    def getMarkerNames(self, *args, **kwargs):
        return []

    def getMarkerList(self, *args, **kwargs):
        return []

    def createNode(self, *args, **kwargs):
        pass

    def characterize(self, *args, **kwargs):
        pass

    def rig(self, *args, **kwargs):
        pass

    def gui(self, *args, **kwargs):
        pass

    def reset(self, *args, **kwargs):
        pass

    def keyAll(self, *args, **kwargs):
        pass

    def sync(self, *args, **kwargs):
        pass

    def setRestPosition(self, *args, **kwargs):
        markerNames = self.getMarkerNames()
        markerList = self.getMarkerList()
        for i in range(0,len(markerList)):
        #for marker in markerList:
            marker = markerList[i]
            markerName = markerNames[i]
            if marker:
                Marker.setRestPosition(marker)
            else:
                print ("[Warning] Missing Marker: " + self.instanceName + "."+ markerName)