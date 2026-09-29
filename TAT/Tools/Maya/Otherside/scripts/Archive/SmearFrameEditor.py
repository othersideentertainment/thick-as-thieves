import maya.cmds as cmds
from functools import partial

class SmearData(object):   
    #CREATE
    @staticmethod
    def Create(source, target, scale):
        #Build Node
        node = cmds.createNode("network", n=(source+"_smearNode"))
        cmds.addAttr(node, ln="isSmearNode", at="message")
        cmds.addAttr(node, ln="smearChain", at="message")
        cmds.addAttr(node, ln="source", at="message")
        cmds.addAttr(node, ln="target", at="message")
        cmds.addAttr(node, ln="scale", at="float", dv=1, min=0, k=True)        
        cmds.connectAttr(source+".message", node+".source", f=True)
        cmds.connectAttr(target+".message", node+".target", f=True)        
        cmds.setAttr(node+".scale", scale)
        #Create Instance
        instance = SmearData()
        instance.node = node
        instance.source = source
        instance.target = target
        instance.scale = scale     
        #validate
        instance.valid = (instance.node != None) and (instance.source != None) and (instance.target != None)   
        return instance

    #READ        
    @staticmethod
    def Read(node):
        if node == None:
            return None
            
        if not cmds.objExists(node+".isSmearNode"):
            return None
            
        instance = SmearData()
        instance.node = node
        #source
        con = cmds.listConnections(node+".source")
        if con != None:
            con = con[0]
            instance.source = con
        #target
        con = cmds.listConnections(node+".target")
        if con != None:
            con = con[0]
            instance.target = con            
        #scale
        instance.scale = cmds.getAttr(node+".scale")
        #validate
        instance.valid = (instance.node != None) and (instance.source != None) and (instance.target != None)   
        return instance

    #FIND RELATED SMEAR DATA
    @staticmethod
    def FindRelatedSmearData(obj):
        conList = cmds.listConnections(obj+".message")
        for con in conList:
            if cmds.objExists(con+".isSmearNode"):
                data = SmearData.Read(con)
                return data
        return None

    #INIT
    def __init__(self):
        self.node = None
        self.source = None
        self.target = None
        self.scale = None
        self.valid = False            

    #APPLY        
    def apply(self, frameLength):
        if self.valid:
            sampleFrame = cmds.currentTime(q=True)
            sampleFrame = sampleFrame - (frameLength * self.scale)
            worldMatrix = cmds.getAttr(self.source+".worldMatrix", t=sampleFrame)
            cmds.xform(self.target, ws=True, m=worldMatrix)        
        
    def key(self):
        if self.valid:
            cmds.setKeyframe(self.target)

#SmearData.Create("bone1", "smear1", 1)
#SmearData.Create("bone2", "smear2", 2)
#SmearData.Create("bone3", "smear3", 3)
#SmearData.Create("bone4", "smear4", 4)

#ApplySmearSelected(0)        
#KeySmearSelected()


class SmearFrameEditor():
    _windowName = "smearFrameEditor"
    _frameCount = "smearFrameEditor|main|frameControls|controls|slider|frameCount"
    _autoUpdate = "smearFrameEditor|main|frameControls|controls|timeline|autoupdate"
    _bakeStart = "smearFrameEditor|main|bake|startFrame"
    _bakeEnd = "smearFrameEditor|main|bake|endFrame"
    _bakeFade = "smearFrameEditor|main|bake|fade"
    
    #Show Window
    @staticmethod
    def Show(*args):
        #Ensure single instance
        windowName = SmearFrameEditor._windowName
        if (cmds.window(windowName, exists=True)):
            cmds.deleteUI( windowName, window=True )   
        if (cmds.windowPref(windowName, exists=True)):
            cmds.windowPref(windowName, r=True)

        frameMin = cmds.playbackOptions(q=True, min=True)
        frameMax = cmds.playbackOptions(q=True, max=True)

        #Window Contents        
        cmds.window( windowName, title='Smear Frame Editor', widthHeight=(430, 190), s=False)    
        cmds.columnLayout("main")   
        cmds.text(l="  Frame Control", font="boldLabelFont", height = 25)         
        cmds.rowColumnLayout("frameControls", numberOfColumns=2)
        #Smear Frame Controls
        cmds.rowColumnLayout("controls", width = 320)
        #-Smear Frame Presets
        cmds.rowColumnLayout("presets", 
            numberOfColumns=7, 
            columnWidth=[(1, 40), (2, 38), (3, 38), (4, 38), (5, 38), (6, 38), (7,38)], 
            cs=[(1, 5), (2, 5), (3, 5), (4, 5), (5, 5), (6, 5), (7,5)], 
            width=300)
        cmds.text("Presets", align="left")
        cmds.button(l="0", c=partial(SmearFrameEditor.UISetFrameDelay, 0))
        cmds.button(l="1", c=partial(SmearFrameEditor.UISetFrameDelay, 1))
        cmds.button(l="2", c=partial(SmearFrameEditor.UISetFrameDelay, 2))
        cmds.button(l="3", c=partial(SmearFrameEditor.UISetFrameDelay, 3))
        cmds.button(l="4", c=partial(SmearFrameEditor.UISetFrameDelay, 4))
        cmds.button(l="5", c=partial(SmearFrameEditor.UISetFrameDelay, 5))
        cmds.setParent("..")
        cmds.text(l="", height=5)                
        #-Smear Frame Slider
        cmds.rowColumnLayout("slider", 
            numberOfColumns=2, 
            columnWidth=[(1,40), (2,260)], 
            cs=[(1,5),(2,5)])
        cmds.text("Frames", align="left")                
        cmds.floatSliderGrp("frameCount", 
            field=True, 
            width=260, 
            cw3=[50,50,210], 
            min=0, 
            max=5, 
            v=1, 
            dc=SmearFrameEditor.UIAutoApplySmear, 
            cc=SmearFrameEditor.UIAutoApplySmear)
        cmds.setParent("..")
        cmds.text(l="", height=5)                
        #-Timeline
        cmds.rowColumnLayout("timeline", 
            numberOfColumns=3, 
            columnWidth=[(1, 95), (2, 95), (3, 95)], 
            cs=[(1, 5), (2, 5), (3, 5)], 
            width=300)
        cmds.checkBox("autoupdate", l="Auto Update", v=True)
        cmds.button(l="<<", width=90, c=partial(SmearFrameEditor.StepTimeline, -1))       
        cmds.button(l=">>", width=90, c=partial(SmearFrameEditor.StepTimeline, 1))      
        cmds.setParent("..")
        cmds.setParent("..")
        #-Toolbar: Buttons
        cmds.rowColumnLayout("buttons", width=100)
        cmds.button(l="Reset", width=100, c=SmearFrameEditor.ResetSmear)
        cmds.text(l="", height=5)          
        cmds.button(l="Apply", width=100, c=SmearFrameEditor.ApplySmear)
        cmds.text(l="", height=5)  
        cmds.button(l="Key", width=100, c=SmearFrameEditor.KeySmear)
        cmds.setParent("..")          
        cmds.setParent("..")       
        #
        cmds.text(l="")
        cmds.separator(width = 430)
        #        
        cmds.text(l="  Bake Sequence", font="boldLabelFont", height = 25)
        cmds.rowColumnLayout("bake", numberOfColumns=8 )
        cmds.text(l="Start", width=40)
        cmds.intField("startFrame", width=65, v=frameMin)
        cmds.text(l="End", width=40)
        cmds.intField("endFrame", width=65, v=frameMax)
        cmds.text(l="Fade", width=40)
        cmds.intField("fade", width=65)
        cmds.text(l="")
        cmds.button(l="Bake", width=100, c=SmearFrameEditor.BakeSequence)                
        cmds.showWindow()

    #UI - Set Frame Delay    
    @staticmethod
    def UISetFrameDelay(frameCount, *args):
        cmds.floatSliderGrp( SmearFrameEditor._frameCount, e=True, v=frameCount)   
        SmearFrameEditor.UIAutoApplySmear()
        
    #UI - Auto Apply Smear
    @staticmethod
    def UIAutoApplySmear(*args):
        auto = cmds.checkBox( SmearFrameEditor._autoUpdate, q=True, v=True)   
        if auto:
            SmearFrameEditor.ApplySmear()            
            
    #Step Timeline
    @staticmethod
    def StepTimeline(delta, *args):
        time = cmds.currentTime(q=True)
        cmds.currentTime(time + delta)
        auto = cmds.checkBox( SmearFrameEditor._autoUpdate, q=True, v=True)   
        if auto:
            SmearFrameEditor.ApplySmear()     
            
    #Find Related Smear Data (Selected)
    @staticmethod
    def FindRelatedSmearData(*args):
        smearData = []
        objList = cmds.ls(sl=True, fl=True)
        for obj in objList:
            data = SmearData.FindRelatedSmearData(obj)
            if data != None:
                if data.valid:
                    smearData.append(data)
        return smearData        
    
    #Find Related Apply Smear (Selected)
    @staticmethod    
    def ApplySmear(*args):
        frameLength = cmds.floatSliderGrp(SmearFrameEditor._frameCount, q=True, v=True)
        smearData = SmearFrameEditor.FindRelatedSmearData()
        for smear in smearData:
            smear.apply(frameLength)
    
    #Find Related Key Smear (Selected)
    @staticmethod    
    def KeySmear(*args):
        smearData = SmearFrameEditor.FindRelatedSmearData()
        for smear in smearData:
            smear.key()  
    
    #Reset Smear
    @staticmethod
    def ResetSmear(*args):
        smearData = SmearFrameEditor.FindRelatedSmearData()
        for smear in smearData:
            smear.apply(0)

    #Bake
    @staticmethod
    def BakeSequence(*args):
        startFrame = cmds.intField(SmearFrameEditor._bakeStart, q=True, v=True)
        endFrame = cmds.intField(SmearFrameEditor._bakeEnd, q=True, v=True)
        fade = cmds.intField(SmearFrameEditor._bakeFade, q=True, v=True)    
        frameLength = cmds.floatSliderGrp(SmearFrameEditor._frameCount, q=True, v=True)
        smearData = SmearFrameEditor.FindRelatedSmearData()
        for i in range(startFrame, endFrame+1):
            cmds.currentTime(i)  
            if i > endFrame-fade and fade != 0:
                frameLength = frameLength - (frameLength / fade)
            for smear in smearData:
                smear.apply(frameLength)  
                smear.key()                     


def ShowWindow(*args):
    SmearFrameEditor.Show()



